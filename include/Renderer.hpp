#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "GBufferShader.hpp"

struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};

#define VERTEX_BUFFER_SIZE u64(512) * u64(1024) * u64(1024) // 512 MB
#define INDEX_BUFFER_BASE VERTEX_BUFFER_SIZE
#define INDEX_BUFFER_SIZE u64(128) * u64(1024) * u64(1024) // 128 MB

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

	// GPU Memory management
	// Joint vertex/index buffer
	/// TODO: We need a better allocator/deallocator
	// It should keep track of free memory which may be "holes" (after removing memory from middle of buffer) and allocate if new additions fit
	// If we want to compact data (not sure if this will be worth the effort) we'd have to keep track of which memory regions are used by which models

	VkBuffer vkVertexIndexBuffer;
	VkDeviceMemory vkVertexIndexBufferMemory;
	u64 vertexInputByteOffset;
	u64 indexInputByteOffset;
	void createVulkanVertexIndexBuffers();
	void pushModelDataToGPU(Model& model);

	// End GPU mem management

	VkBuffer vkDrawCmdBuffer;
	VkDeviceMemory vkDrawCmdBufferMemory;
	void populateDrawCmdBuffer();

	VkBuffer vkUniformBuffer;
	VkDeviceMemory vkUniformBufferMemory;

	VkBuffer vkTransformBuffer;
	VkDeviceMemory vkTransformBufferMemory;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	VkSampler textureSampler;

	UniformBufferObject ubo;

	GBufferShader gBufferShader;

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

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, int mipLevels);
	void transitionImageLayout(VkImage image, VkFormat format,VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int mipLevel);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels);
	void createTextureSampler();

	void initVulkanUniformBuffer();
	void initVulkanDescriptorPool();
	void initVulkanDescriptorSet();
	void initVulkanCommandBuffers();
	void initVulkanSemaphores();

	// Copies to optimal (efficient) device local buffer
	void copyToDeviceLocalBuffer(void* srcData, VkDeviceSize size, VkBuffer dstBuffer, VkDeviceSize dstOffset = 0);
	// Copies to staging buffer
	void copyToStagingBuffer(void* srcData, VkDeviceSize size, VkDeviceSize dstOffset = 0);
	// Creates staging buffer with requested size
	void createStagingBuffer(VkDeviceSize size);
	// Destroys staging buffer
	void destroyStagingBuffer();

	void createVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyVulkanBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize dstOffset = 0, VkDeviceSize srcOffset = 0);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void updateUniformBuffer();

	void cleanupVulkanSwapChain();
	void recreateVulkanSwapChain();
};
