#include "PCH.hpp"
#include "Lights.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void PointLight::initTexture(int resolution)
{
	TextureCreateInfo ci = {};
	ci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	ci.bpp = 32;
	ci.format = Engine::getPhysicalDeviceDetails().findDepthFormat();
	ci.genMipMaps = false;
	ci.components = 1;
	ci.height = resolution;
	ci.width = resolution;
	ci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	ci.numLayers = 6;
	ci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	shadowTex = new Texture();
	shadowTex->loadToGPU(&ci);

	std::array<VkImageView, 1> attachments = {
		shadowTex->getImageViewHandle()
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = Engine::renderer->pointShadowRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = resolution;
	framebufferInfo.height = resolution;
	framebufferInfo.layers = 6;

	VK_CHECK_RESULT(vkCreateFramebuffer(Engine::renderer->device, &framebufferInfo, nullptr, &shadowFBO));
}

void PointLight::destroy()
{
	vkDestroyFramebuffer(Engine::renderer->device, shadowFBO, 0);
	shadowTex->destroy();
}

void PointLight::updateRadius()
{
	matNeedsUpdate = true;
	gpuData->setRadius(calculateRadius(gpuData->getLinear(), gpuData->getQuadratic()));
	proj = glm::perspective<float>(glm::radians(90.f), 1.f, 0.1f, gpuData->getRadius());
}

inline float PointLight::calculateRadius(float linear, float quad)
{
	return (0.5 * (std::sqrt(linear*linear + (200 * quad) - linear))) / quad;
}

void SpotLight::destroy()
{
	vkDestroyFramebuffer(Engine::renderer->device, shadowFBO, 0);
	shadowTex->destroy();
}

void SpotLight::initTexture(int resolution)
{
	TextureCreateInfo ci = {};
	ci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	ci.bpp = 32;
	ci.format = Engine::getPhysicalDeviceDetails().findDepthFormat();
	ci.genMipMaps = false;
	ci.components = 1;
	ci.height = resolution;
	ci.width = resolution;
	ci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	ci.numLayers = 1;
	ci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	shadowTex = new Texture();
	shadowTex->loadToGPU(&ci);

	std::array<VkImageView, 1> attachments = {
		shadowTex->getImageViewHandle()
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = Engine::renderer->spotShadowRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = resolution;
	framebufferInfo.height = resolution;
	framebufferInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(Engine::renderer->device, &framebufferInfo, nullptr, &shadowFBO));
}

void SpotLight::updateRadius()
{
	matNeedsUpdate = true;
	gpuData->setRadius(calculateRadius(gpuData->getLinear(), gpuData->getQuadratic()));
}

inline float SpotLight::calculateRadius(float linear, float quad)
{
	return (0.5 * (std::sqrt(linear*linear + (2000 * quad) - linear))) / (quad * 2.f);
}

PointLight & LightManager::addPointLight(PointLight::GPUData& data)
{
	pointLights.push_back(PointLight());
	auto& light = pointLights.back();
	pointLightsGPUData.push_back(data);
	light.gpuData = &pointLightsGPUData.back();

	return light;
}

SpotLight & LightManager::addSpotLight(SpotLight::GPUData& data)
{
	spotLights.push_back(SpotLight());
	auto& light = spotLights.back();
	spotLightsGPUData.push_back(data);
	light.gpuData = &spotLightsGPUData.back();

	return light;
}

void SunLight::initTexture(glm::ivec2 resolution)
{
	TextureCreateInfo ci = {};
	ci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	ci.bpp = 32;
	ci.format = Engine::getPhysicalDeviceDetails().findDepthFormat();
	ci.genMipMaps = false;
	ci.components = 1;
	ci.height = resolution.y;
	ci.width = resolution.x;
	ci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	ci.numLayers = 1;
	ci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	shadowTex = new Texture();
	shadowTex->loadToGPU(&ci);

	std::array<VkImageView, 1> attachments = {
		shadowTex->getImageViewHandle()
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = Engine::renderer->sunShadowRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = resolution.x;
	framebufferInfo.height = resolution.y;
	framebufferInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(Engine::renderer->device, &framebufferInfo, nullptr, &shadowFBO));
}

void SunLight::calcProjs()
{
	glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
		+0.0f, -1.0f, 0.0f, 0.0f,
		+0.0f, 0.0f, 0.5f, 0.0f,
		+0.0f, 0.0f, 0.5f, 1.0f);

	glm::fmat4 camView = Engine::camera.getView();
	glm::fmat4 invView = glm::inverse(camView);

	auto dir = getDirection();
	glm::fmat4 lightView = glm::lookAt(glm::fvec3(0, 0, 0), dir, glm::fvec3(0, 1, 0));

	float ar = Engine::camera.getAspect();
	float tanHalfVFOV = tanf(Engine::camera.getFOV() / 2.f);
	float tanHalfHFOV = tanf((Engine::camera.getFOV() * ar) / 2.f);

	const int NUM_CASCADES = 1;

	for (u32 i = 0; i < NUM_CASCADES; i++) {
		float xn = -gpuData->cascadeEnds[i] * tanHalfHFOV;
		float xf = -gpuData->cascadeEnds[i + 1] * tanHalfHFOV;
		float yn = -gpuData->cascadeEnds[i] * tanHalfVFOV;
		float yf = -gpuData->cascadeEnds[i + 1] * tanHalfVFOV;

		glm::fvec4 frustumCorners[8] = {
			// near face
			glm::fvec4(xn, yn, -gpuData->cascadeEnds[i], 1.0),
			glm::fvec4(-xn, yn, -gpuData->cascadeEnds[i], 1.0),
			glm::fvec4(xn, -yn, -gpuData->cascadeEnds[i], 1.0),
			glm::fvec4(-xn, -yn, -gpuData->cascadeEnds[i], 1.0),

			// far face
			glm::fvec4(xf, yf, -gpuData->cascadeEnds[i + 1], 1.0),
			glm::fvec4(-xf, yf, -gpuData->cascadeEnds[i + 1], 1.0),
			glm::fvec4(xf, -yf, -gpuData->cascadeEnds[i + 1], 1.0),
			glm::fvec4(-xf, -yf, -gpuData->cascadeEnds[i + 1], 1.0)
		};

		glm::fvec4 frustumCornersL[8];

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();

		for (u32 j = 0; j < 8; j++) {

			// Transform the frustum coordinate from view to world space
			glm::fvec4 vW = invView * frustumCorners[j];

			// Transform the frustum coordinate from world to light space
			frustumCornersL[j] = lightView * vW;
			
			minX = std::min(minX, frustumCornersL[j].x);
			maxX = std::max(maxX, frustumCornersL[j].x);
			minY = std::min(minY, frustumCornersL[j].y);
			maxY = std::max(maxY, frustumCornersL[j].y);
			minZ = std::min(minZ, frustumCornersL[j].z);
			maxZ = std::max(maxZ, frustumCornersL[j].z);
		}

		float right = maxX;
		float left = minX;
		float bot = minY;
		float top = maxY;
		float farp = maxZ;
		float nearp = minZ;

		/*orthoData[0] = maxX;
		orthoData[1] = minX;
		orthoData[2] = minY;
		orthoData[3] = maxY;
		orthoData[4] = maxZ;
		orthoData[5] = minZ;*/



		gpuData->projView[i] = clip * glm::ortho<float>(left, right, bot, top, nearp, farp) * lightView;
	}
}