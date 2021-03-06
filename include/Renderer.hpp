#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "Lights.hpp"
#include "ShaderSpecs.hpp"
#include "UIText.hpp"
#include "UIElement.hpp"
#include "UIRenderer.hpp"

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

#define USED_GBUFFER texturedGBufferAttachments
//#define USED_GBUFFER flatGBufferAttachments

/*
	@brief	Collates Vulkan objects and controls rendering pipeline
*/
class Renderer
{
public:
	Renderer() : vertexInputByteOffset(0), indexInputByteOffset(0), gBufferDescriptorSetNeedsUpdate(true), gBufferNoTexDescriptorSetNeedsUpdate(true) {}

	// Top level
	void initialiseDevice();
	void initialise();
	void cleanup();
	void render();

	static void renderJob();

	void createShaders();
	void compileShaders();
	void reloadShaders();

	void updateConfigs();

	enum GPUTimeStamps { 
		BEGIN_GBUFFER,			END_GBUFFER, 
		BEGIN_SHADOW,			END_SHADOW, 
		BEGIN_SSAO,				END_SSAO,
		BEGIN_PBR,				END_PBR, 
		BEGIN_UI,				END_UI,
		BEGIN_SCREEN,			END_SCREEN, 
		NUM_GPU_TIMESTAMPS };

	UIRenderer uiRenderer;

	// Device, queues, swap chain
	vdu::LogicalDevice logicalDevice;
	vdu::Queue lTransferQueue;
	vdu::Queue lGraphicsQueue;

	VkDevice device;
	VkQueue transferQueue; // Queue for memory transfers (submitted to by the main thread)
	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;
	VkExtent2D renderResolution;

	vdu::VertexInputState defaultVertexInputState;

	std::mutex bufferFreeMutex;

	// Thread safe command pools
	static thread_local vdu::CommandPool commandPool;
	// Memory pools
	vdu::DescriptorPool descriptorPool;
	vdu::DescriptorPool freeableDescriptorPool;
	vdu::QueryPool queryPool;

	// Semaphores
	vdu::Semaphore imageAvailableSemaphore;
	vdu::Semaphore gBufferFinishedSemaphore;
	vdu::Semaphore screenFinishedSemaphore;
	vdu::Semaphore pbrFinishedSemaphore;
	vdu::Semaphore shadowFinishedSemaphore;
	vdu::Semaphore overlayFinishedSemaphore;
	vdu::Semaphore overlayCombineFinishedSemaphore;
	vdu::Semaphore ssaoFinishedSemaphore;

	// Fences
	vdu::Fence gBufferGroupFence;
	vdu::Fence pbrGroupFence;

	// Samplers
	VkSampler textureSampler;
	VkSampler skySampler;
	VkSampler shadowSampler;

	void createLogicalDevice();
	void createDescriptorPool();
	static void createPerThreadCommandPools();
	void createQueryPool();
	void createTextureSampler();
	void createSynchroObjects();
	void createUBOs();

	// Shaders
	GBufferShader gBufferShader;
	GBufferNoTexShader gBufferNoTexShader;
	PBRShader pbrShader;
	ScreenShader screenShader;
	PointShadowShader pointShadowShader;
	SpotShadowShader spotShadowShader;
	OverlayShader overlayShader;
	CombineOverlaysShader combineOverlaysShader;
	SunShadowShader sunShadowShader;
	CombineSceneShader combineSceneShader;
	SSAOShader ssaoShader;
	SSAOBlurShader ssaoBlurShader;

	/// There are probably more elegant solutions to this
	/// A generic full rendering pipeline class would be useful
	struct GBufferAttachments {
		vdu::Texture* albedo;
		vdu::Texture* normal;
		vdu::Texture* pbr;
		vdu::Texture* depth;
	};

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
	vdu::GraphicsPipeline gBufferPipeline;
	vdu::PipelineLayout gBufferPipelineLayout;
	
	// Descriptors
	vdu::DescriptorSetLayout gBufferDescriptorSetLayout;
	vdu::DescriptorSet gBufferDescriptorSet;
	bool gBufferDescriptorSetNeedsUpdate;

	// Framebuffer and attachments
	vdu::Framebuffer gBufferFramebuffer;
	Texture gBufferColourAttachment;
	Texture gBufferNormalAttachment;
	Texture gBufferPBRAttachment;
	Texture gBufferDepthAttachment;
	//Texture gBufferDepthLinearAttachment;

	GBufferAttachments texturedGBufferAttachments = { &gBufferColourAttachment, &gBufferNormalAttachment, &gBufferPBRAttachment, &gBufferDepthAttachment };

	// Render pass
	vdu::RenderPass gBufferRenderPass;

	// Command buffer
	vdu::CommandBuffer gBufferCommandBuffer;
	bool gBufferCmdsNeedUpdate;

	/// --------------------
	/// GBuffer no texture pipeline
	/// --------------------

	// Functions
	void createGBufferNoTexAttachments();
	void createGBufferNoTexRenderPass();
	void createGBufferNoTexDescriptorSetLayouts();
	void createGBufferNoTexPipeline();
	void createGBufferNoTexFramebuffers();
	void createGBufferNoTexDescriptorSets();
	void createGBufferNoTexCommands();

	void updateGBufferNoTexDescriptorSets();
	void updateGBufferNoTexCommands();

	void destroyGBufferNoTexAttachments();
	void destroyGBufferNoTexRenderPass();
	void destroyGBufferNoTexDescriptorSetLayouts();
	void destroyGBufferNoTexPipeline();
	void destroyGBufferNoTexFramebuffers();
	void destroyGBufferNoTexDescriptorSets();
	void destroyGBufferNoTexCommands();

	// Pipeline objets
	vdu::GraphicsPipeline gBufferNoTexPipeline;
	vdu::PipelineLayout gBufferNoTexPipelineLayout;

	// Descriptors
	vdu::DescriptorSetLayout gBufferNoTexDescriptorSetLayout;
	vdu::DescriptorSet gBufferNoTexDescriptorSet;
	bool gBufferNoTexDescriptorSetNeedsUpdate;

	// Framebuffer and attachments
	vdu::Framebuffer gBufferNoTexFramebuffer;
	Texture gBufferNoTexColourAttachment;
	Texture gBufferNoTexNormalAttachment;
	Texture gBufferNoTexPBRAttachment;
	Texture gBufferNoTexDepthAttachment;

	GBufferAttachments flatGBufferAttachments = { &gBufferNoTexColourAttachment, &gBufferNoTexNormalAttachment, &gBufferNoTexPBRAttachment, &gBufferNoTexDepthAttachment };

	// Render pass
	vdu::RenderPass gBufferNoTexRenderPass;

	// Command buffer
	vdu::CommandBuffer gBufferNoTexCommandBuffer;
	bool gBufferNoTexCmdsNeedUpdate;

	/// TODO: temp
	GBufferAttachments usedGBuffer = USED_GBUFFER;

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
	vdu::GraphicsPipeline pointShadowPipeline;
	vdu::PipelineLayout pointShadowPipelineLayout;

	vdu::GraphicsPipeline spotShadowPipeline;
	vdu::PipelineLayout spotShadowPipelineLayout;

	vdu::GraphicsPipeline sunShadowPipeline;
	vdu::PipelineLayout sunShadowPipelineLayout;

	// Render pass
	vdu::RenderPass shadowRenderPass;

	// Descriptors
	vdu::DescriptorSetLayout shadowDescriptorSetLayout;
	vdu::DescriptorSet shadowDescriptorSet;

	vdu::DescriptorSetLayout spotShadowDescriptorSetLayout;
	vdu::DescriptorSet spotShadowDescriptorSet;

	// Command buffer
	vdu::CommandBuffer shadowCommandBuffer;
	VkFence shadowFence;

	/// --------------------
	/// SSAO pipeline
	/// --------------------

	// Functions
	void createSSAOAttachments();
	void createSSAORenderPass();
	void createSSAODescriptorSetLayouts();
	void createSSAOPipeline();
	void createSSAOFramebuffer();
	void createSSAODescriptorSets();
	void createSSAOCommands();

	void updateSSAODescriptorSets();
	void updateSSAOCommands();

	void destroySSAOAttachments();
	void destroySSAORenderPass();
	void destroySSAODescriptorSetLayouts();
	void destroySSAOPipeline();
	void destroySSAOFramebuffer();
	void destroySSAODescriptorSets();
	void destroySSAOCommands();

	// Pipeline objets
	vdu::GraphicsPipeline ssaoPipeline;
	vdu::PipelineLayout ssaoPipelineLayout;
	vdu::VertexInputState ssaoVertexInputState;

	vdu::GraphicsPipeline ssaoBlurPipeline;
	vdu::PipelineLayout ssaoBlurPipelineLayout;

	// Descriptors
	vdu::DescriptorSetLayout ssaoDescriptorSetLayout;
	vdu::DescriptorSet ssaoDescriptorSet;

	vdu::DescriptorSetLayout ssaoBlurDescriptorSetLayout;
	vdu::DescriptorSet ssaoBlurDescriptorSet;

	vdu::DescriptorSetLayout ssaoFinalDescriptorSetLayout;
	vdu::DescriptorSet ssaoFinalDescriptorSet;

	// Descriptor data
	struct SSAOConfig
	{
		int samples;
		int spiralTurns;
		float projScale;
		float radius;
		float bias;
		float intensity;
		glm::fvec2 PADDING;
		glm::fvec4 projInfo;
	} ssaoConfigData;

	vdu::Buffer ssaoConfigBuffer;

	vdu::Framebuffer ssaoFramebuffer;
	Texture ssaoColourAttachment;

	vdu::Framebuffer ssaoBlurFramebuffer;
	Texture ssaoBlurAttachment;

	vdu::Framebuffer ssaoFinalFramebuffer;
	Texture ssaoFinalAttachment;

	// Render pass
	vdu::RenderPass ssaoRenderPass;
	vdu::RenderPass ssaoBlurRenderPass;

	// Command buffer
	vdu::CommandBuffer ssaoCommandBuffer;

	/// --------------------
	/// PBR shading pipeline
	/// --------------------

	// Functions
	void createPBRAttachments();
	void createPBRDescriptorSetLayouts();
	void createPBRPipeline();
	void createPBRDescriptorSets();
	void createPBRCommands();
	
	void updatePBRDescriptorSets(GBufferAttachments& gbAtt);
	void updatePBRCommands();

	void destroyPBRAttachments();
	void destroyPBRDescriptorSetLayouts();
	void destroyPBRPipeline();
	void destroyPBRDescriptorSets();
	void destroyPBRCommands();

	// Pipeline objects
	vdu::ComputePipeline pbrPipeline;
	vdu::PipelineLayout pbrPipelineLayout;

	// Descriptors
	vdu::DescriptorSetLayout pbrDescriptorSetLayout;
	vdu::DescriptorSet pbrDescriptorSet;

	// Framebuffer and attachments
	Texture pbrOutput;

	// Command buffer
	vdu::CommandBuffer pbrCommandBuffer;

	/// --------------------
	/// Screen pipeline
	/// --------------------

	// Functions
	void createScreenSwapchain();
	void createScreenDescriptorSetLayouts();
	void createScreenPipeline();
	void createScreenDescriptorSets();
	void createScreenCommands();

	void updateScreenDescriptorSets();
	void updateScreenCommands();
	void updateScreenCommandsForConsole();

	void destroyScreenSwapchain();
	void destroyScreenDescriptorSetLayouts();
	void destroyScreenPipeline();
	void destroyScreenDescriptorSets();
	void destroyScreenCommands();
	
	// Pipeline objects
	vdu::PipelineLayout screenPipelineLayout;
	vdu::GraphicsPipeline screenPipeline;
	vdu::VertexInputState screenVertexInputState;
	
	// Descriptors
	vdu::DescriptorSetLayout screenDescriptorSetLayout;
	vdu::DescriptorSet screenDescriptorSet;

	// Framebuffer and attachments
	vdu::Swapchain screenSwapchain;

	// Command buffers
	//std::vector<vdu::CommandBuffer> screenCommandBuffers;
	vdu::CommandBufferArray screenCommandBuffers;
	vdu::CommandBufferArray screenCommandBuffersForConsole;

	/// ------------------------
	/// Scene combine pipeline
	/// ------------------------





	// GPU Memory management
	// Joint vertex/index buffer
	/// TODO: We need a better allocator/deallocator
	// It should keep track of free memory which may be "holes" (after removing memory from middle of buffer) and allocate if new additions fit
	// If we want to compact data (not sure if this will be worth the effort) we'd have to keep track of which memory regions are used by which models

	vdu::Buffer screenQuadBuffer;
	
	vdu::Buffer vertexIndexBuffer;
	u64 vertexInputByteOffset;
	u64 indexInputByteOffset;
	void pushModelDataToGPU(Model& model);
	void createDataBuffers();

	void updateMaterialDescriptors();
	void updateFlatMaterialBuffer();
	void updateSkyboxDescriptor();

	void addFenceDelayedAction(vdu::Fence* fe, std::function<void(void)> action);
	void executeFenceDelayedActions();
	static thread_local std::unordered_map<vdu::Fence*, std::function<void(void)>> fenceDelayedActions;

	// End GPU mem management
	void initialiseQueryPool();

	// Draw buffers
	vdu::Buffer drawCmdBuffer;
	void populateDrawCmdBuffer();

	// Uniform buffers
	CameraUBOData cameraUBOData;
	vdu::Buffer cameraUBO;
	vdu::Buffer transformUBO;
	vdu::Buffer flatPBRUBO;

	LightManager lightManager;
	
	void setImageLayout(VkCommandBuffer cmdbuffer, Texture& tex, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
	void setImageLayout(vdu::CommandBuffer* cmdbuffer, Texture& tex, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

	void beginTransferCommands(vdu::CommandBuffer& cmd);
	void endTransferCommands(vdu::CommandBuffer& cmd, vdu::QueueSubmission& submission);
	void submitTransferCommands(vdu::QueueSubmission& submission, vdu::Fence fence = vdu::Fence());

	void updateCameraBuffer();
	void updateTransformBuffer();
	void updateSSAOConfigBuffer();
};

