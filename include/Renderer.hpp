#pragma once
#include "PCH.hpp"
#include "Engine.hpp"

class Renderer
{
public:

	void initialise();
	void cleanup();
	void render();

	VkDevice vkDevice;
	VkQueue vkGraphicsQueue;
	VkQueue vkPresentQueue;
	VkSwapchainKHR vkSwapChain;
	VkPipeline vkPipeline;
	VkPipelineLayout vkPipelineLayout;
	VkRenderPass vkRenderPass;
	VkCommandPool vkCommandPool;
	std::vector<VkCommandBuffer> vkCommandBuffers;
	std::vector<VkFramebuffer> vkFramebuffers;
	std::vector<VkImage> vkSwapChainImages;
	std::vector<VkImageView> vkSwapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	void initVulkanLogicalDevice();
	void initVulkanSwapChain();
	void initVulkanImageViews();
	void initVulkanRenderPass();
	void initVulkanGraphicsPipeline();
	void initVulkanFramebuffers();
	void initVulkanCommandPool();
	void initVulkanCommandBuffers();
	void initVulkanSemaphores();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	
	void cleanupSwapChain();
	void recreateVulkanSwapChain();
};