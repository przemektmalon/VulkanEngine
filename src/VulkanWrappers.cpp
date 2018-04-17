#include "PCH.hpp"
#include "VulkanWrappers.hpp"
#include "Engine.hpp"
#include "Window.hpp"

/*
	@brief	Query vulkan for the necessary device features, queue family details, and swap chain 
*/
void PhysicalDeviceDetails::queryDetails()
{
	// Query device properties and features
	{
		VK_VALIDATE(vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties));
		VK_VALIDATE(vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &deviceFeatures));

		switch (deviceProperties.deviceType)
		{
		case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):
			suitabilityScore += 1000;
			break;
		case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU):
			suitabilityScore += 500;
			break;
		case (VK_PHYSICAL_DEVICE_TYPE_CPU):
			suitabilityScore += 100;
			break;
		default:
			suitabilityScore -= 100000;
			break;
		}
	}

	/// TODO: Other suitability measures

	// Query queue family properties
	{
		uint32_t queueFamilyCount = 0;
		VK_VALIDATE(vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr));
		queueFamilies.resize(queueFamilyCount);
		VK_VALIDATE(vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies.data()));
	}

	// Choose queue families
	{
		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 surfaceSupport = false;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, Engine::window->vkSurface, &surfaceSupport));

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsQueueFamily == -1)
				graphicsQueueFamily = i;

			if (graphicsQueueFamily == i && surfaceSupport && presentQueueFamily == -1)
				presentQueueFamily = i;

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && computeQueueFamily == -1)
				computeQueueFamily = i;

			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
				transferQueueFamily = i;

			suitabilityScore += queueFamily.queueCount;

			++i;
		}

		if (graphicsQueueFamily == -1 || presentQueueFamily == -1 || computeQueueFamily == -1 || transferQueueFamily == -1)
			suitabilityScore -= 100000;
	}

	// Query surface capabilities
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, Engine::window->vkSurface, &swapChainDetails.capabilities));

	// Query surface formats
	{
		uint32_t formatCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, Engine::window->vkSurface, &formatCount, nullptr));

		if (formatCount != 0) {
			swapChainDetails.formats.resize(formatCount);
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, Engine::window->vkSurface, &formatCount, swapChainDetails.formats.data()));
		}
		else
			suitabilityScore -= 100000;
	}

	// Query surface present modes
	{
		uint32_t presentModeCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, Engine::window->vkSurface, &presentModeCount, nullptr));

		if (presentModeCount != 0) {
			swapChainDetails.presentModes.resize(presentModeCount);
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, Engine::window->vkSurface, &presentModeCount, swapChainDetails.presentModes.data()));
		}
		else
			suitabilityScore -= 100000;
	}

	// Query device memory properties
	{
		VK_VALIDATE(vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memoryProperties));
	}
}

/*
	@brief	Get the requested memory type index
*/
u32 PhysicalDeviceDetails::getMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	DBG_SEVERE("Suitable memory type not found");
}
