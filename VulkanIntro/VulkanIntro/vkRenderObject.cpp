#include "vkRenderObject.h"



vkRenderObject::vkRenderObject(uint32_t imageCount, std::vector<Vertex> vertices, std::vector<uint32_t> indices,
	vk::DescriptorSetLayout &descriptorSetLayout, vk::DescriptorPool &descriptorPool, 
	vkTexture* texture, vkCommandBufferManager* commandBufferManager)
	: mImageCount(imageCount), mVertices(vertices), mIndices(indices),
	mVertexBuffer(sizeof(mVertices[0]) * mVertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, commandBufferManager, mVertices.data()),
	mIndexBuffer(sizeof(mIndices[0]) * mIndices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, commandBufferManager, mIndices.data())
{
	create_uniform_buffer(commandBufferManager);
	create_descriptor_sets(descriptorSetLayout, descriptorPool, texture);
}


vkRenderObject::~vkRenderObject()
{
	for (size_t i = 0; i < mImageCount; i++) {
		delete mUniformBuffers[i];
	}
}

void vkRenderObject::create_uniform_buffer(vkCommandBufferManager* commandBufferManager) {
	vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

	mUniformBuffers.resize(mImageCount);

	for (size_t i = 0; i < mImageCount; i++) {
		mUniformBuffers[i] = new vkCgbBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, commandBufferManager);
	}
}

void vkRenderObject::create_descriptor_sets(vk::DescriptorSetLayout &descriptorSetLayout, vk::DescriptorPool &descriptorPool, vkTexture* texture) {
	std::vector<vk::DescriptorSetLayout> layouts(mImageCount, descriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(mImageCount);
	allocInfo.pSetLayouts = layouts.data();

	mDescriptorSets.resize(mImageCount);
	if (vkContext::instance().device.allocateDescriptorSets(&allocInfo, mDescriptorSets.data()) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < mImageCount; i++) {
		vk::DescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = mUniformBuffers[i]->getVkBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		vk::DescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = texture->getTextureImageView();
		imageInfo.sampler = texture->getTextureSampler();

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].dstSet = mDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].dstSet = mDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkContext::instance().device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void vkRenderObject::update_uniform_buffer(uint32_t currentImage, float time, vk::Extent2D swapChainExtent) {
	// TODO transfer code to camera
	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	ubo.mvp = ubo.proj * ubo.view * ubo.model;

	mPushUniforms.model = ubo.model;
	mPushUniforms.view = ubo.view;
	mPushUniforms.proj = ubo.proj;
	mPushUniforms.mvp = ubo.mvp;

	mUniformBuffers[currentImage]->updateBuffer(&ubo, sizeof(ubo));
}