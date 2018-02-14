#pragma once
#include "PCH.hpp"

struct PhysicalDeviceDetails
{
	PhysicalDeviceDetails(VkPhysicalDevice device) : suitabilityScore(0), vkPhysicalDevice(device), graphicsQueueFamily(-1), presentQueueFamily(-1), computeQueueFamily(-1), transferQueueFamily(-1) {}
	s32 suitabilityScore;
	VkPhysicalDevice vkPhysicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	void queryDetails();
	std::vector<VkQueueFamilyProperties> queueFamilies;
	s32 graphicsQueueFamily;
	s32 computeQueueFamily;
	s32 presentQueueFamily;
	s32 transferQueueFamily;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails swapChainDetails;
};