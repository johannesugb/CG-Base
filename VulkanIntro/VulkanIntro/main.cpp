#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>

#include <stdio.h>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <fstream>
#include <array>

#include "VkRenderObject.h"
#include "VkTexture.h"
#include "VkCommandBufferManager.h"
#include "VkDrawer.h"
#include "VkImagePresenter.h"
#include "vulkan_render_queue.h"
#include "VkRenderer.h"
#include "vulkan_pipeline.h"
#include "vulkan_framebuffer.h"
#include "vrs_image_compute_drawer.h"

#include "eyetracking_interface.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string MODEL_PATH = "models/chalet.obj/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";

class HelloTriangleApplication {
private:
	GLFWwindow * window;
	

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorPool descriptorPool;

	vk::DescriptorSetLayout vrsComputeDescriptorSetLayout;
	vk::DescriptorPool vrsComputeDescriptorPool;	
	std::vector<vk::DescriptorSet> mVrsComputeDescriptorSets;

	vk::CommandPool commandPool;
	vk::CommandPool transferCommandPool;

	bool framebufferResized = false;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	vkRenderObject* renderObject;
	vkRenderObject* renderObject2;
	vkTexture* texture;
	vkCgbImage* textureImage;
	std::shared_ptr<vkCommandBufferManager> drawCommandBufferManager;
	vkCommandBufferManager* transferCommandBufferManager;
	std::unique_ptr<vkDrawer> drawer;
	std::unique_ptr<vrs_image_compute_drawer> mVrsImageComputeDrawer;

	// render target needed for MSAA
	std::shared_ptr<vkCgbImage> colorImage;
	std::shared_ptr<vkCgbImage> depthImage;
	std::shared_ptr<vkCgbImage> vrsImage;
	std::shared_ptr<vkCgbImage> vrsDebugImage;
	std::shared_ptr <vkTexture> vrsDebugTextureImage;
	std::shared_ptr<vkImagePresenter> imagePresenter;
	std::shared_ptr<vulkan_render_queue> mVulkanRenderQueue;
	std::unique_ptr<vkRenderer> mRenderer;
	std::shared_ptr<vkRenderer> mVrsRenderer;
	std::shared_ptr<vulkan_pipeline> mRenderVulkanPipeline;
	std::shared_ptr<vulkan_pipeline> mComputeVulkanPipeline;
	std::shared_ptr<vulkan_framebuffer> mVulkanFramebuffer;

	eyetracking_interface eyeInf;

public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initVulkan() {
		vkContext::instance().initVulkan(window);

		createCommandPools();

		transferCommandBufferManager = new vkCommandBufferManager(transferCommandPool, vkContext::instance().graphicsQueue);

		imagePresenter = std::make_shared<vkImagePresenter>(vkContext::instance().presentQueue, vkContext::instance().surface, vkContext::instance().findQueueFamilies());
		vkContext::instance().dynamicRessourceCount = imagePresenter->get_swap_chain_images_count();
		drawCommandBufferManager = std::make_shared<vkCommandBufferManager>(imagePresenter->get_swap_chain_images_count(), commandPool, vkContext::instance().graphicsQueue);
		mVulkanRenderQueue = std::make_shared<vulkan_render_queue>(vkContext::instance().graphicsQueue);
		mVrsRenderer = std::make_shared<vkRenderer>(nullptr, mVulkanRenderQueue, drawCommandBufferManager, std::vector<std::shared_ptr<vkRenderer>>{}, true);
		mRenderer = std::make_unique<vkRenderer>(imagePresenter, mVulkanRenderQueue, drawCommandBufferManager, std::vector<std::shared_ptr<vkRenderer>>{ mVrsRenderer });

		createColorResources();
		createDepthResources();
		createVRSImageResources();

		mVulkanFramebuffer = std::make_shared<vulkan_framebuffer>(vkContext::instance().msaaSamples, colorImage, depthImage, imagePresenter);
		createDescriptorSetLayout();
		createVrsComputeDescriptorSetLayout();

		vk::Viewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)imagePresenter->get_swap_chain_extent().width;
		viewport.height = (float)imagePresenter->get_swap_chain_extent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = imagePresenter->get_swap_chain_extent();

		// Render Drawer and Pipeline
		mRenderVulkanPipeline = std::make_shared<vulkan_pipeline>(mVulkanFramebuffer->get_render_pass(), viewport, scissor, vkContext::instance().msaaSamples, descriptorSetLayout);
		drawer = std::make_unique<vkDrawer>(drawCommandBufferManager, mRenderVulkanPipeline);
		drawer->set_vrs_image(vrsImage);

		// Compute Drawer and Pipeline
		mComputeVulkanPipeline = std::make_shared<vulkan_pipeline>("Shader/vrs_img.spv", std::vector<vk::DescriptorSetLayout> { vrsComputeDescriptorSetLayout }, sizeof(glm::vec2));
		mVrsImageComputeDrawer = std::make_unique<vrs_image_compute_drawer>(drawCommandBufferManager, mComputeVulkanPipeline, vrsDebugImage);

		createTexture();

		createDescriptorPool();
		createVrsComputeDescriptorPool();
		createVrsDescriptorSets();
		mVrsImageComputeDrawer->set_descriptor_sets(mVrsComputeDescriptorSets);
		mVrsImageComputeDrawer->set_width_height(vrsImage->get_width(), vrsImage->get_height());

		renderObject = new vkRenderObject(imagePresenter->get_swap_chain_images_count(), verticesQuad, indicesQuad, descriptorSetLayout, descriptorPool, texture, transferCommandBufferManager, vrsDebugTextureImage.get());
		renderObject2 = new vkRenderObject(imagePresenter->get_swap_chain_images_count(), verticesQuad, indicesQuad, descriptorSetLayout, descriptorPool, texture, transferCommandBufferManager, vrsDebugTextureImage.get());

		renderObject2->update_uniform_buffer(0, 0, imagePresenter->get_swap_chain_extent());
		renderObject2->update_uniform_buffer(1, 0, imagePresenter->get_swap_chain_extent());
		renderObject2->update_uniform_buffer(2, 0, imagePresenter->get_swap_chain_extent());
		//loadModel();
	}

	void mainLoop() {
		float t = float(glfwGetTime());
		float dt = 0.0f;
		float sum_t = 0.0f;

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			auto eyeData = eyeInf.get_eyetracking_data();

			drawFrame();

			dt = t;
			t = float(glfwGetTime());
			dt = t - dt;
			sum_t += dt;

			if (sum_t >= 1.0f) {
				glfwSetWindowTitle(window, std::to_string(1.0f / dt).c_str());
				sum_t -= 1.0f;

				printf("Gaze point: %f, %f\n",
					eyeData.positionX,
					eyeData.positionY);
			}
		}

		// wait for all commands to complete
		vkContext::instance().device.waitIdle();
	}

	void cleanup() {
		cleanupSwapChain();

		vkContext::instance().device.destroyDescriptorPool(descriptorPool);
		vkContext::instance().device.destroyDescriptorSetLayout(descriptorSetLayout);
		vkContext::instance().device.destroyDescriptorPool(vrsComputeDescriptorPool);
		vkContext::instance().device.destroyDescriptorSetLayout(vrsComputeDescriptorSetLayout);

		delete renderObject;
		delete renderObject2;
		delete texture;
		delete textureImage;
		delete transferCommandBufferManager;
		mVulkanRenderQueue.reset();
		drawCommandBufferManager.reset();

		vkContext::instance().device.destroyCommandPool(transferCommandPool);
		vkContext::instance().device.destroyCommandPool(commandPool);

		// destroy instance (in context)

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanStSt", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void cleanupSwapChain() {
		colorImage.reset();
		depthImage.reset();

		drawer.reset();
		mRenderVulkanPipeline.reset();
		mVrsImageComputeDrawer.reset();
		mComputeVulkanPipeline.reset();
		mVulkanFramebuffer.reset();

		imagePresenter.reset();
		mVrsRenderer.reset();
		mRenderer.reset();
	}

	void recreateSwapChain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkContext::instance().device.waitIdle();

		cleanupSwapChain();

		imagePresenter = std::make_shared<vkImagePresenter>(vkContext::instance().presentQueue, vkContext::instance().surface, vkContext::instance().findQueueFamilies());
		mRenderer = std::make_unique<vkRenderer>(imagePresenter, mVulkanRenderQueue, drawCommandBufferManager);
		createColorResources();
		createDepthResources();
		mVulkanFramebuffer = std::make_shared<vulkan_framebuffer>(vkContext::instance().msaaSamples, colorImage, depthImage, imagePresenter);
		
		vk::Viewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)imagePresenter->get_swap_chain_extent().width;
		viewport.height = (float)imagePresenter->get_swap_chain_extent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = imagePresenter->get_swap_chain_extent();

		mRenderVulkanPipeline = std::make_shared<vulkan_pipeline>(mVulkanFramebuffer->get_render_pass(), viewport, scissor, vkContext::instance().msaaSamples, descriptorSetLayout);
		drawer = std::make_unique<vkDrawer>(drawCommandBufferManager, mRenderVulkanPipeline);
	}

	void createCommandPools() {
		QueueFamilyIndices queueFamilyIndices = vkContext::instance().findQueueFamilies();

		vk::CommandPoolCreateInfo poolInfo = {};
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer; // Optional

		if (vkContext::instance().device.createCommandPool(&poolInfo, nullptr, &commandPool) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create command pool!");
		}

		// create command pool for data transfers 
		vk::CommandPoolCreateInfo transferPoolInfo = {};
		transferPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		transferPoolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient; // Optional

		if (vkContext::instance().device.createCommandPool(&transferPoolInfo, nullptr, &transferCommandPool) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create command pool for data transfers!");
		}
	}

	void drawFrame() {
		mRenderer->start_frame();
		// update states, e.g. for animation
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		renderObject->update_uniform_buffer(vkContext::instance().currentFrame, time, imagePresenter->get_swap_chain_extent());

		// start drawing, record draw commands, etc.
		vkContext::instance().vulkanFramebuffer = mVulkanFramebuffer;

		mVrsRenderer->render(std::vector<vkRenderObject*>{}, mVrsImageComputeDrawer.get());

		std::vector<vkRenderObject*> renderObjects;
		renderObjects.push_back(renderObject);
		renderObjects.push_back(renderObject2);

		mRenderer->render(renderObjects, drawer.get());
		mRenderer->end_frame();
	}

	void createDescriptorSetLayout() {
		vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		vk::DescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding samplerDebugLayoutBinding = {};
		samplerDebugLayoutBinding.binding = 2;
		samplerDebugLayoutBinding.descriptorCount = 1;
		samplerDebugLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerDebugLayoutBinding.pImmutableSamplers = nullptr;
		samplerDebugLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
		std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding,samplerDebugLayoutBinding };
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vkContext::instance().device.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void createDescriptorPool() {
		std::array<vk::DescriptorPoolSize, 3> poolSizes = {};
		poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count()) * 2;
		poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count()) * 2;
		poolSizes[2].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count()) * 2;

		vk::DescriptorPoolCreateInfo poolInfo = {};
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count() * 2);

		if (vkContext::instance().device.createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createVrsComputeDescriptorSetLayout() {
		vk::DescriptorSetLayoutBinding storageImageLayoutBinding = {};
		storageImageLayoutBinding.binding = 0;
		storageImageLayoutBinding.descriptorCount = 1;
		storageImageLayoutBinding.descriptorType = vk::DescriptorType::eStorageImage;
		storageImageLayoutBinding.pImmutableSamplers = nullptr;
		storageImageLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
		std::array<vk::DescriptorSetLayoutBinding, 1> bindings = { storageImageLayoutBinding };
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vkContext::instance().device.createDescriptorSetLayout(&layoutInfo, nullptr, &vrsComputeDescriptorSetLayout) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create vrs compute descriptor set layout!");
		}
	}

	void createVrsComputeDescriptorPool() {
		std::array<vk::DescriptorPoolSize, 1> poolSizes = {};
		poolSizes[0].type = vk::DescriptorType::eStorageImage;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count());

		vk::DescriptorPoolCreateInfo poolInfo = {};
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count());

		if (vkContext::instance().device.createDescriptorPool(&poolInfo, nullptr, &vrsComputeDescriptorPool) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create vrs compute descriptor pool!");
		}
	}

	void createVrsDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(imagePresenter->get_swap_chain_images_count(), vrsComputeDescriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.descriptorPool = vrsComputeDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count());
		allocInfo.pSetLayouts = layouts.data();

		mVrsComputeDescriptorSets.resize(imagePresenter->get_swap_chain_images_count());
		if (vkContext::instance().device.allocateDescriptorSets(&allocInfo, mVrsComputeDescriptorSets.data()) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < imagePresenter->get_swap_chain_images_count(); i++) {
			vk::DescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = vk::ImageLayout::eGeneral;
			imageInfo.imageView = vrsImage->get_image_view();

			std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {};

			descriptorWrites[0].dstSet = mVrsComputeDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = vk::DescriptorType::eStorageImage;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;

			vkContext::instance().device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	// image / texture

	void createTexture() {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
		textureImage = new vkCgbImage(transferCommandBufferManager, pixels, texWidth, texHeight, texChannels);
		texture = new vkTexture(textureImage);

		stbi_image_free(pixels);
	}

	// load model
	void loadModel() {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const auto& shape : shapes) {
			for (const auto& shape : shapes) {
				for (const auto& index : shape.mesh.indices) {
					Vertex vertex = {};
					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};

					vertex.color = { 1.0f, 1.0f, 1.0f };

					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}

					indices.push_back(uniqueVertices[vertex]);
				}
			}
		}

		renderObject = new vkRenderObject((uint32_t)imagePresenter->get_swap_chain_images_count(), vertices, indices, descriptorSetLayout, descriptorPool, texture, transferCommandBufferManager, vrsDebugTextureImage.get());
	}


	// attachments for framebuffer (color image to render to before resolve, depth image)

	void createDepthResources() {
		vk::Format depthFormat = findDepthFormat();

		depthImage = std::make_shared<vkCgbImage>(transferCommandBufferManager, imagePresenter->get_swap_chain_extent().width, imagePresenter->get_swap_chain_extent().height, 1, vkContext::instance().msaaSamples, depthFormat,
			vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);
		depthImage->transition_image_layout(depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);

	}

	vk::Format findDepthFormat() {
		return findSupportedFormat(
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment
		);
	}

	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
		for (vk::Format format : candidates) {
			vk::FormatProperties props;
			vkContext::instance().physicalDevice.getFormatProperties(format, &props);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}
	
	void createColorResources() {
		vk::Format colorFormat = imagePresenter->get_swap_chain_image_format();

		colorImage = std::make_shared<vkCgbImage>(transferCommandBufferManager, imagePresenter->get_swap_chain_extent().width, imagePresenter->get_swap_chain_extent().height, 1, vkContext::instance().msaaSamples, colorFormat,
			vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
		colorImage->transition_image_layout(colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1);
	}

	void createVRSImageResources() {
		vk::Format colorFormat = vk::Format::eR8Uint;
		auto width = imagePresenter->get_swap_chain_extent().width / vkContext::instance().shadingRateImageProperties.shadingRateTexelSize.width;
		auto height = imagePresenter->get_swap_chain_extent().height / vkContext::instance().shadingRateImageProperties.shadingRateTexelSize.height;

		vrsImage = std::make_shared<vkCgbImage>(transferCommandBufferManager, width, height, 1, vk::SampleCountFlagBits::e1, colorFormat,
			vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eShadingRateImageNV | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
		vrsImage->transition_image_layout(colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1); // vk::ImageLayout::eShadingRateOptimalNV

		// Debug image
		colorFormat = imagePresenter->get_swap_chain_image_format();

		vrsDebugImage = std::make_shared<vkCgbImage>(transferCommandBufferManager, width, height, 1, vk::SampleCountFlagBits::e1, colorFormat,
			vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
		vrsDebugImage->transition_image_layout(colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1);

		vrsDebugTextureImage = std::make_shared <vkTexture>(vrsDebugImage.get());
	}
	
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}