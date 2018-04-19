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
	shadowTex->create(&ci);

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
	shadowTex->create(&ci);

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