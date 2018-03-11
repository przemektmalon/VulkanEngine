#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Model.hpp"

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

/*
	@brief	Collates Vulkan objects and controls rendering pipeline
*/
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
	VkDescriptorSetLayout vkDescriptorSetLayout;
	VkDescriptorPool vkDescriptorPool;
	VkDescriptorSet vkDescriptorSet;
	VkRenderPass vkRenderPass;
	VkCommandPool vkCommandPool;
	std::vector<VkCommandBuffer> vkCommandBuffers;
	std::vector<VkFramebuffer> vkFramebuffers;
	std::vector<VkImage> vkSwapChainImages;
	std::vector<VkImageView> vkSwapChainImageViews;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkBuffer vkStagingBuffer;
	VkDeviceMemory vkStagingBufferMemory;

	Model chalet;

	VkBuffer vkUniformBuffer;
	VkDeviceMemory vkUniformBufferMemory;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	UniformBufferObject ubo;

	void initVulkanLogicalDevice();
	void initVulkanSwapChain();
	void initVulkanImageViews();
	void initVulkanRenderPass();
	void initVulkanDescriptorSetLayout();
	void initVulkanGraphicsPipeline();
	void initVulkanFramebuffers();
	void initVulkanCommandPool();

	void initVulkanDepthResources();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	void createTextureImage();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, int mipLevels);
	void transitionImageLayout(VkImage image, VkFormat format,VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int mipLevel);
	void createTextureImageView();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels);
	void createTextureSampler();

	void initVulkanUniformBuffer();
	void initVulkanDescriptorPool();
	void initVulkanDescriptorSet();
	void initVulkanCommandBuffers();
	void initVulkanSemaphores();

	void createVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyVulkanBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void updateUniformBuffer();

	void cleanupVulkanSwapChain();
	void recreateVulkanSwapChain();
};
