#include "PCH.hpp"
#include "Polygon.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void UIPolygon::reserveBuffer(int numVerts)
{
	vertsBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vertsBuffer.setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	vertsBuffer.create(&Engine::renderer->logicalDevice, numVerts * sizeof(Vertex2D));
}

void UIPolygon::render(VkCommandBuffer cmd)
{
	if (drawable)
	{
		VkDeviceSize offsets[] = { 0 };
		VkBuffer buffer[] = { vertsBuffer.getHandle() };
		VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, buffer, offsets));
		VK_VALIDATE(vkCmdDraw(cmd, verts.size(), 1, 0, 0));
	}
}

void UIPolygon::cleanup()
{
	vertsBuffer.destroy();
}

void UIPolygon::setTexture(Texture * tex)
{
	texture = tex;
	updateDescriptorSet();
}

void UIPolygon::setVerts(std::vector<Vertex2D>& v)
{
	verts = v;
	memcpy(vertsBuffer.getMemory()->map(), verts.data(), verts.size() * sizeof(Vertex2D));
	vertsBuffer.getMemory()->unmap();
	drawable = true;
}

void UIPolygon::setColour(glm::fvec4 colour)
{
	memcpy(pushConstData, &colour, sizeof(colour));
}

void UIPolygon::updateDescriptorSet()
{
	VkDescriptorImageInfo fontInfo = {};
	fontInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	fontInfo.imageView = texture->getView();
	fontInfo.sampler = Engine::renderer->textureSampler;

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descSet.getHandle();
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &fontInfo;

	VK_VALIDATE(vkUpdateDescriptorSets(Engine::renderer->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr));
}