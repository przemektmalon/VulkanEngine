#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "GBufferShader.hpp"
#include "ScreenShader.hpp"
#include "PBRShader.hpp"
#include "Buffer.hpp"
#include "Lights.hpp"

struct CameraUBOData {
	glm::fmat4 view;
	glm::fmat4 proj;
	glm::fvec3 pos;
	float unused;
	glm::fvec4 viewRays;
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
	Renderer() : vertexInputByteOffset(0), indexInputByteOffset(0) {}

	// Top level
	void initialise();
	void reInitialise();
	void cleanup();
	void cleanupForReInit();
	void render();

	// Device, queues, swap chain
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D renderResolution;

	// Memory pools
	VkDescriptorPool descriptorPool;
	VkCommandPool commandPool;

	void createLogicalDevice();
	void createDescriptorPool();
	void createCommandPool();
	void createTextureSampler();

	// Shaders
	GBufferShader gBufferShader;
	PBRShader pbrShader;
	ScreenShader screenShader;

	/// --------------------
	/// GBuffer pipeline
	/// --------------------

	// Functions
	void createGBufferAttachments();
	void createGBufferRenderPass();
	void createGBufferDescriptorSetLayouts();
	void createGBufferPipeline();
	void createGBufferFramebuffers();
	void createGBufferDescriptorSets();
	void createGBufferCommands();

	void updateGBufferDescriptorSets();
	void updateGBufferCommands();

	void destroyGBufferAttachments();
	void destroyGBufferRenderPass();
	void destroyGBufferDescriptorSetLayouts();
	void destroyGBufferPipeline();
	void destroyGBufferFramebuffers();
	void destroyGBufferDescriptorSets();
	void destroyGBufferCommands();

	// Pipeline objets
	VkPipeline gBufferPipeline;
	VkPipelineLayout gBufferPipelineLayout;
	
	// Descriptors
	VkDescriptorSetLayout gBufferDescriptorSetLayout;
	VkDescriptorSet gBufferDescriptorSet;

	// Framebuffer and attachments
	VkFramebuffer gBufferFramebuffer;
	Texture gBufferColourAttachment;
	Texture gBufferNormalAttachment;
	Texture gBufferPBRAttachment;
	Texture gBufferDepthAttachment;

	// Render pass
	VkRenderPass gBufferRenderPass;

	// Command buffer
	VkCommandBuffer gBufferCommandBuffer;

	/// --------------------
	/// PBR shading pipeline
	/// --------------------

	// Functions
	void createPBRAttachments();
	void createPBRDescriptorSetLayouts();
	void createPBRPipeline();
	void createPBRDescriptorSets();
	void createPBRCommands();
	
	void updatePBRDescriptorSets();
	void updatePBRCommands();

	void destroyPBRAttachments();
	void destroyPBRDescriptorSetLayouts();
	void destroyPBRPipeline();
	void destroyPBRDescriptorSets();
	void destroyPBRCommands();

	// Pipeline objects
	VkPipeline pbrPipeline;
	VkPipelineLayout pbrPipelineLayout;

	// Descriptors
	VkDescriptorSetLayout pbrDescriptorSetLayout;
	VkDescriptorSet pbrDescriptorSet;

	// Framebuffer and attachments
	Texture pbrOutput;

	// Command buffer
	VkCommandBuffer pbrCommandBuffer;

	/// --------------------
	/// Screen pipeline
	/// --------------------

	// Functions
	void createScreenSwapChain();
	void createScreenAttachments();
	void createScreenRenderPass();
	void createScreenDescriptorSetLayouts();
	void createScreenPipeline();
	void createScreenFramebuffers();
	void createScreenDescriptorSets();
	void createScreenCommands();

	void updateScreenDescriptorSets();
	void updateScreenCommands();

	void destroyScreenSwapChain();
	void destroyScreenAttachments();
	void destroyScreenRenderPass();
	void destroyScreenDescriptorSetLayouts();
	void destroyScreenPipeline();
	void destroyScreenFramebuffers();
	void destroyScreenDescriptorSets();
	void destroyScreenCommands();
	
	// Pipeline objects
	VkPipeline screenPipeline;
	VkPipelineLayout screenPipelineLayout;
	
	// Descriptors
	VkDescriptorSetLayout screenDescriptorSetLayout;
	VkDescriptorSet screenDescriptorSet;

	// Framebuffer and attachments
	std::vector<VkFramebuffer> screenFramebuffers;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	// Render pass
	VkRenderPass screenRenderPass;

	// Command buffers
	std::vector<VkCommandBuffer> screenCommandBuffers;
	

	// GPU Memory management
	// Joint vertex/index buffer
	/// TODO: We need a better allocator/deallocator
	// It should keep track of free memory which may be "holes" (after removing memory from middle of buffer) and allocate if new additions fit
	// If we want to compact data (not sure if this will be worth the effort) we'd have to keep track of which memory regions are used by which models

	Buffer stagingBuffer;
	
	Buffer screenQuadBuffer;
	
	std::vector<Vertex2D> quad;

	Buffer vertexIndexBuffer;
	u64 vertexInputByteOffset;
	u64 indexInputByteOffset;
	void createVertexIndexBuffers();
	void pushModelDataToGPU(Model& model);
	void createDataBuffers();

	// End GPU mem management

	// Draw buffers
	Buffer drawCmdBuffer;
	void populateDrawCmdBuffer();

	// Uniform buffers

	CameraUBOData cameraUBOData;
	Buffer cameraUBO;
	Buffer transformUBO;
	LightManager lightManager;
	
	// Semaphores
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkSemaphore screenFinishedSemaphore;
	VkSemaphore pbrFinishedSemaphore;

	// Samplers
	VkSampler textureSampler;


	
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, int mipLevels);
	void transitionImageLayout(VkImage image, VkFormat format,VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int mipLevel);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels);
	

	void createUBOs();

	void createSemaphores();

	// Copies to optimal (efficient) device local buffer
	void copyToDeviceLocalBuffer(void* srcData, VkDeviceSize size, VkBuffer dstBuffer, VkDeviceSize dstOffset = 0);
	void copyToDeviceLocalBuffer(void* srcData, VkDeviceSize size, Buffer* dstBuffer, VkDeviceSize dstOffset = 0);
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

	//void cleanupVulkanSwapChain();
	void recreateVulkanSwapChain();
};
