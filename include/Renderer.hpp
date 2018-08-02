#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "Lights.hpp"
#include "ShaderSpecs.hpp"
#include "Text.hpp"
#include "OverlayElement.hpp"
#include "OverlayRenderer.hpp"
#include "vdu\VDU.hpp"

struct CameraUBOData {
	glm::fmat4 view;
	glm::fmat4 proj;
	glm::fvec3 pos;
	float unused;
	glm::fvec4 viewRays;
};

#define VERTEX_BUFFER_SIZE u64(128) * u64(1024) * u64(1024) // 128 MB
#define INDEX_BUFFER_BASE VERTEX_BUFFER_SIZE
#define INDEX_BUFFER_SIZE u64(32) * u64(1024) * u64(1024) // 32 MB

/*
	@brief	Collates Vulkan objects and controls rendering pipeline
*/
class Renderer
{
public:
	Renderer() : vertexInputByteOffset(0), indexInputByteOffset(0), gBufferDescriptorSetNeedsUpdate(true) {}

	// Top level
	void initialiseDevice();
	void initialise();
	void reInitialise();
	void cleanup();
	void cleanupForReInit();
	void render();

	void reloadShaders();

	enum GPUTimeStamps { 
		BEGIN_GBUFFER,			END_GBUFFER, 
		BEGIN_SHADOW,			END_SHADOW, 
		BEGIN_PBR,				END_PBR, 
		BEGIN_OVERLAY,			END_OVERLAY, 
		BEGIN_OVERLAY_COMBINE,	END_OVERLAY_COMBINE, 
		BEGIN_SCREEN,			END_SCREEN, 
		NUM_GPU_TIMESTAMPS };

	OverlayRenderer overlayRenderer;

	// Device, queues, swap chain
	vdu::LogicalDevice logicalDevice;
	vdu::Queue lTransferQueue;
	vdu::Queue lGraphicsQueue;

	VkDevice device;
	VkQueue transferQueue; // Queue for memory transfers (submitted to by the main thread)
	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D renderResolution;

	// Thread safe command pools and fencing

	// For now just make sure each thread has its own command pool
	static thread_local vdu::CommandPool commandPool; // [3]
	std::mutex bufferFreeMutex;

	// The current pool will be toggled when all submissions are done ( after all buffers are created )
	/*int currentPool;

	struct ThreadSafeCommands
	{
		VkCommandPool commandPool;

		// Command buffers will be allocated on each thread from the current pool and added here
		std::vector<VkCommandBuffer> commandsToSubmit;

		// A pool can be reset when all fences are signalled
		std::vector<VkFence> fences;
	};

	static thread_local ThreadSafeCommands threadSafeCommands[3];*/


	struct CommandsAndFence
	{
		VkCommandBuffer commands;
		VkFence fence;
	};


	// Memory pools
	vdu::DescriptorPool descriptorPool;
	vdu::DescriptorPool freeableDescriptorPool;
	vdu::QueryPool queryPool;

	// Semaphores
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkSemaphore screenFinishedSemaphore;
	VkSemaphore pbrFinishedSemaphore;
	VkSemaphore shadowFinishedSemaphore;
	VkSemaphore overlayFinishedSemaphore;
	VkSemaphore overlayCombineFinishedSemaphore;

	// Samplers
	VkSampler textureSampler;
	VkSampler skySampler;
	VkSampler shadowSampler;

	void createLogicalDevice();
	void createDescriptorPool();
	void createCommandPool();
	static void createPerThreadCommandPools();
	void createQueryPool();
	void createTextureSampler();
	void createSemaphores();
	void createUBOs();

	// Shaders
	GBufferShader gBufferShader;
	PBRShader pbrShader;
	ScreenShader screenShader;
	PointShadowShader pointShadowShader;
	SpotShadowShader spotShadowShader;
	OverlayShader overlayShader;
	CombineOverlaysShader combineOverlaysShader;
	SunShadowShader sunShadowShader;
	CombineSceneShader combineSceneShader;

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
	vdu::DescriptorSetLayout gBufferDescriptorSetLayout;
	vdu::DescriptorSet gBufferDescriptorSet;
	bool gBufferDescriptorSetNeedsUpdate;

	// Framebuffer and attachments
	VkFramebuffer gBufferFramebuffer;
	Texture gBufferColourAttachment;
	Texture gBufferNormalAttachment;
	Texture gBufferPBRAttachment;
	Texture gBufferDepthAttachment;

	// Render pass
	VkRenderPass gBufferRenderPass;

	// Command buffer
	vdu::CommandBuffer gBufferCommands;
	bool gBufferCmdsNeedUpdate;

	/// --------------------
	/// Shadow pipeline
	/// --------------------

	// Functions
	void createShadowRenderPass();
	void createShadowDescriptorSetLayouts();
	void createShadowPipeline();
	void createShadowDescriptorSets();
	void createShadowCommands();

	void updateShadowDescriptorSets();
	void updateShadowCommands();

	void destroyShadowRenderPass();
	void destroyShadowDescriptorSetLayouts();
	void destroyShadowPipeline();
	void destroyShadowDescriptorSets();
	void destroyShadowCommands();

	// Pipeline objets
	VkPipeline pointShadowPipeline;
	VkPipelineLayout pointShadowPipelineLayout;

	VkPipeline spotShadowPipeline;
	VkPipelineLayout spotShadowPipelineLayout;

	VkPipeline sunShadowPipeline;
	VkPipelineLayout sunShadowPipelineLayout;

	// Descriptors
	vdu::DescriptorSetLayout shadowDescriptorSetLayout;
	vdu::DescriptorSet shadowDescriptorSet;

	// Render pass
	VkRenderPass pointShadowRenderPass;
	VkRenderPass spotShadowRenderPass;
	VkRenderPass sunShadowRenderPass;

	// Command buffer
	vdu::CommandBuffer shadowCommandBuffer;
	VkFence shadowFence;

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
	vdu::DescriptorSetLayout pbrDescriptorSetLayout;
	vdu::DescriptorSet pbrDescriptorSet;

	// Framebuffer and attachments
	Texture pbrOutput;

	// Command buffer
	vdu::CommandBuffer pbrCommandBuffer;
	VkFence pbrFence;

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
	void updateScreenCommandsForConsole();

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
	vdu::DescriptorSetLayout screenDescriptorSetLayout;
	vdu::DescriptorSet screenDescriptorSet;

	// Framebuffer and attachments
	std::vector<VkFramebuffer> screenFramebuffers;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	// Render pass
	VkRenderPass screenRenderPass;

	// Command buffers
	//std::vector<vdu::CommandBuffer> screenCommandBuffers;
	vdu::CommandBufferArray screenCommandBuffers;

	/// ------------------------
	/// Scene combine pipeline
	/// ------------------------





	// GPU Memory management
	// Joint vertex/index buffer
	/// TODO: We need a better allocator/deallocator
	// It should keep track of free memory which may be "holes" (after removing memory from middle of buffer) and allocate if new additions fit
	// If we want to compact data (not sure if this will be worth the effort) we'd have to keep track of which memory regions are used by which models

	vdu::Buffer stagingBuffer;
	vdu::Buffer screenQuadBuffer;
	
	vdu::Buffer vertexIndexBuffer;
	u64 vertexInputByteOffset;
	u64 indexInputByteOffset;
	void createVertexIndexBuffers();
	void pushModelDataToGPU(Model& model);
	void createDataBuffers();

	// End GPU mem management
	
	// Draw buffers
	vdu::Buffer drawCmdBuffer;
	void populateDrawCmdBuffer();

	std::vector<OverlayElement*> overlayElems;

	// Uniform buffers
	CameraUBOData cameraUBOData;
	vdu::Buffer cameraUBO;
	vdu::Buffer transformUBO;
	LightManager lightManager;
	
	void transitionImageLayout(VkImage image, VkFormat format,VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, int layerCount = 1, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
	void setImageLayout(VkCommandBuffer cmdbuffer, Texture& tex, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

	VkCommandBuffer beginTransferCommands();
	VkSubmitInfo endTransferCommands(VkCommandBuffer commandBuffer);
	void submitTransferCommands(VkSubmitInfo submitInfo, VkFence fence = VK_NULL_HANDLE);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE);
	void updateCameraBuffer();
	void updateTransformBuffer();
};

