// hello_world.cpp : Defines the entry point for the console application.
//
#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>

#include "cg_base.h"
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

const int WIDTH = 1920;
const int HEIGHT = 1080;

const std::string TEXTURE_PATH = "assets/chalet.jpg";

class vrs_behavior : public cgb::cg_element
{
private:
	//GLFWwindow* window;


	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorPool descriptorPool;

	vk::DescriptorSetLayout vrsComputeDescriptorSetLayout;
	vk::DescriptorPool vrsComputeDescriptorPool;
	std::vector<vk::DescriptorSet> mVrsComputeDescriptorSets;

	vk::CommandPool commandPool;
	vk::CommandPool transferCommandPool;

	bool framebufferResized = false;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<vrs_behavior*>(glfwGetWindowUserPointer(window));
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
	std::vector<std::shared_ptr<vkCgbImage>> vrsImages;
	std::vector < std::shared_ptr<vkCgbImage>> vrsDebugImages;
	std::vector < std::shared_ptr <vkTexture>> vrsDebugTextureImages;
	std::shared_ptr<vkImagePresenter> imagePresenter;
	std::shared_ptr<vulkan_render_queue> mVulkanRenderQueue;
	std::unique_ptr<vkRenderer> mRenderer;
	std::shared_ptr<vkRenderer> mVrsRenderer;
	std::shared_ptr<vulkan_pipeline> mRenderVulkanPipeline;
	std::shared_ptr<vulkan_pipeline> mComputeVulkanPipeline;
	std::shared_ptr<vulkan_framebuffer> mVulkanFramebuffer;

	std::shared_ptr<eyetracking_interface> eyeInf;

public:
	void initialize() override
	{
		eyeInf = std::make_shared<eyetracking_interface>();
		//initWindow();
		initVulkan();
	}

	void finalize() override
	{
		// wait for all commands to complete
		vkContext::instance().device.waitIdle();

		cleanup();
	}

	void update() override
	{
		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			cgb::current_composition().stop();
		}
	}

	void render() override
	{
		static float sum_t = 0.0f;

		auto eyeData = eyeInf->get_eyetracking_data();

		drawFrame();

		sum_t += cgb::time().delta_time();
		if (sum_t >= 1.0f) {
			cgb::current_composition().window_in_focus()->set_title(std::to_string(1.0f / cgb::time().delta_time()).c_str());
			sum_t -= 1.0f;

			printf("Gaze point: %f, %f\n",
				   eyeData.positionX,
				   eyeData.positionY);
		}
	}

private:
	void initVulkan()
	{
		vkContext::instance().initVulkan();

		createCommandPools();

		transferCommandBufferManager = new vkCommandBufferManager(transferCommandPool, vkContext::instance().graphicsQueue);

		imagePresenter = std::make_shared<vkImagePresenter>(vkContext::instance().presentQueue, vkContext::instance().surface, vkContext::instance().findQueueFamilies());
		vkContext::instance().dynamicRessourceCount = imagePresenter->get_swap_chain_images_count();
		drawCommandBufferManager = std::make_shared<vkCommandBufferManager>(imagePresenter->get_swap_chain_images_count(), commandPool, vkContext::instance().graphicsQueue);
		mVulkanRenderQueue = std::make_shared<vulkan_render_queue>(vkContext::instance().graphicsQueue);

		std::vector<std::shared_ptr<vkRenderer>> dependentRenderers = {};
		if (vkContext::instance().shadingRateImageSupported) {
			mVrsRenderer = std::make_shared<vkRenderer>(nullptr, mVulkanRenderQueue, drawCommandBufferManager, std::vector<std::shared_ptr<vkRenderer>>{}, true);
			dependentRenderers.push_back(mVrsRenderer);
		}
		mRenderer = std::make_unique<vkRenderer>(imagePresenter, mVulkanRenderQueue, drawCommandBufferManager, dependentRenderers);

		createColorResources();
		createDepthResources();
		if (vkContext::instance().shadingRateImageSupported) {
			createVRSImageResources();
		}

		mVulkanFramebuffer = std::make_shared<vulkan_framebuffer>(vkContext::instance().msaaSamples, colorImage, depthImage, imagePresenter);
		createDescriptorSetLayout();
		if (vkContext::instance().shadingRateImageSupported) {
			createVrsComputeDescriptorSetLayout();
		}

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

		if (vkContext::instance().shadingRateImageSupported) {
			drawer->set_vrs_images(vrsImages);

			// Compute Drawer and Pipeline
			mComputeVulkanPipeline = std::make_shared<vulkan_pipeline>("shaders/vrs_img.comp.spv", std::vector<vk::DescriptorSetLayout> { vrsComputeDescriptorSetLayout }, sizeof(vrs_eye_comp_data));
			mVrsImageComputeDrawer = std::make_unique<vrs_image_compute_drawer>(drawCommandBufferManager, mComputeVulkanPipeline, vrsDebugImages);
			mVrsImageComputeDrawer->set_vrs_images(vrsImages);
		}


		createTexture();

		createDescriptorPool();

		if (vkContext::instance().shadingRateImageSupported) {
			createVrsComputeDescriptorPool();
			createVrsDescriptorSets();
			mVrsImageComputeDrawer->set_descriptor_sets(mVrsComputeDescriptorSets);
			mVrsImageComputeDrawer->set_width_height(vrsImages[0]->get_width(), vrsImages[0]->get_height());
			mVrsImageComputeDrawer->set_eye_inf(eyeInf);
		}

		renderObject = new vkRenderObject(imagePresenter->get_swap_chain_images_count(), verticesQuad, indicesQuad, descriptorSetLayout, descriptorPool, texture, transferCommandBufferManager, vrsDebugTextureImages);
		renderObject2 = new vkRenderObject(imagePresenter->get_swap_chain_images_count(), verticesScreenQuad, indicesScreenQuad, descriptorSetLayout, descriptorPool, texture, transferCommandBufferManager, vrsDebugTextureImages);


		// TODO transfer code to camera
		UniformBufferObject ubo = {};
		ubo.model = glm::mat4(1.0f);
		//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model[1][1] *= -1;
		ubo.mvp = ubo.model;
		renderObject2->update_uniform_buffer(0, ubo);
		renderObject2->update_uniform_buffer(1, ubo);
		renderObject2->update_uniform_buffer(2, ubo);
		//loadModel();
	}

	void cleanup()
	{
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

	}

	//void initWindow()
	//{
	//	glfwInit();

	//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//	window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanStSt", nullptr, nullptr);
	//	glfwSetWindowUserPointer(window, this);
	//	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	//}

	void cleanupSwapChain()
	{
		colorImage.reset();
		depthImage.reset();

		drawer.reset();
		mRenderVulkanPipeline.reset();
		if (vkContext::instance().shadingRateImageSupported) {
			mVrsImageComputeDrawer.reset();
		}
		mComputeVulkanPipeline.reset();
		mVulkanFramebuffer.reset();

		imagePresenter.reset();
		if (vkContext::instance().shadingRateImageSupported) {
			mVrsRenderer.reset();
		}
		mRenderer.reset();
	}

	//void recreateSwapChain()
	//{
	//	int width = 0, height = 0;
	//	while (width == 0 || height == 0) {
	//		glfwGetFramebufferSize(cgb::current_composition().window_in_focus()->handle()->mHandle, &width, &height);
	//		glfwWaitEvents();
	//	}

	//	vkContext::instance().device.waitIdle();

	//	cleanupSwapChain();

	//	imagePresenter = std::make_shared<vkImagePresenter>(vkContext::instance().presentQueue, vkContext::instance().surface, vkContext::instance().findQueueFamilies());
	//	mRenderer = std::make_unique<vkRenderer>(imagePresenter, mVulkanRenderQueue, drawCommandBufferManager);
	//	createColorResources();
	//	createDepthResources();
	//	mVulkanFramebuffer = std::make_shared<vulkan_framebuffer>(vkContext::instance().msaaSamples, colorImage, depthImage, imagePresenter);

	//	vk::Viewport viewport = {};
	//	viewport.x = 0.0f;
	//	viewport.y = 0.0f;
	//	viewport.width = (float)imagePresenter->get_swap_chain_extent().width;
	//	viewport.height = (float)imagePresenter->get_swap_chain_extent().height;
	//	viewport.minDepth = 0.0f;
	//	viewport.maxDepth = 1.0f;

	//	vk::Rect2D scissor = {};
	//	scissor.offset = { 0, 0 };
	//	scissor.extent = imagePresenter->get_swap_chain_extent();

	//	mRenderVulkanPipeline = std::make_shared<vulkan_pipeline>(mVulkanFramebuffer->get_render_pass(), viewport, scissor, vkContext::instance().msaaSamples, descriptorSetLayout);
	//	drawer = std::make_unique<vkDrawer>(drawCommandBufferManager, mRenderVulkanPipeline);
	//}

	void createCommandPools()
	{
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

	void drawFrame()
	{
		mRenderer->start_frame();
		// update states, e.g. for animation
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		UniformBufferObject ubo = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), imagePresenter->get_swap_chain_extent().width / (float)imagePresenter->get_swap_chain_extent().height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		ubo.mvp = ubo.proj * ubo.view * ubo.model;
		renderObject->update_uniform_buffer(vkContext::instance().currentFrame, ubo);

		// start drawing, record draw commands, etc.
		vkContext::instance().vulkanFramebuffer = mVulkanFramebuffer;

		if (vkContext::instance().shadingRateImageSupported) {
			mVrsRenderer->render(std::vector<vkRenderObject*>{}, mVrsImageComputeDrawer.get());
		}

		std::vector<vkRenderObject*> renderObjects;
		//renderObjects.push_back(renderObject);
		renderObjects.push_back(renderObject2);
		//for (int i = 0; i < 1000; i++) {
		//	renderObjects.push_back(renderObject2);
		//}

		mRenderer->render(renderObjects, drawer.get());
		mRenderer->end_frame();
	}

	void createDescriptorSetLayout()
	{
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

		std::vector<vk::DescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };
		if (vkContext::instance().shadingRateImageSupported) {
			vk::DescriptorSetLayoutBinding samplerDebugLayoutBinding = {};
			samplerDebugLayoutBinding.binding = 2;
			samplerDebugLayoutBinding.descriptorCount = 1;
			samplerDebugLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			samplerDebugLayoutBinding.pImmutableSamplers = nullptr;
			samplerDebugLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

			bindings.push_back(samplerDebugLayoutBinding);
		}

		vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vkContext::instance().device.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void createDescriptorPool()
	{
		std::vector<vk::DescriptorPoolSize> poolSizes(2);
		poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count()) * 2;
		poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count()) * 2;
		if (vkContext::instance().shadingRateImageSupported) {
			poolSizes.resize(3);
			poolSizes[2].type = vk::DescriptorType::eCombinedImageSampler;
			poolSizes[2].descriptorCount = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count()) * 2;
		}

		vk::DescriptorPoolCreateInfo poolInfo = {};
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(imagePresenter->get_swap_chain_images_count() * 2);

		if (vkContext::instance().device.createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createVrsComputeDescriptorSetLayout()
	{
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

	void createVrsComputeDescriptorPool()
	{
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

	void createVrsDescriptorSets()
	{
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
			imageInfo.imageView = vrsImages[i]->get_image_view();

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

	void createTexture()
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
		textureImage = new vkCgbImage(transferCommandBufferManager, pixels, texWidth, texHeight, texChannels);
		texture = new vkTexture(textureImage);

		stbi_image_free(pixels);
	}

	// attachments for framebuffer (color image to render to before resolve, depth image)

	void createDepthResources()
	{
		vk::Format depthFormat = findDepthFormat();

		depthImage = std::make_shared<vkCgbImage>(transferCommandBufferManager, imagePresenter->get_swap_chain_extent().width, imagePresenter->get_swap_chain_extent().height, 1, vkContext::instance().msaaSamples, depthFormat,
												  vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);
		depthImage->transition_image_layout(depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);

	}

	vk::Format findDepthFormat()
	{
		return findSupportedFormat(
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment
		);
	}

	vk::Format findSupportedFormat(const std::vector<vk::Format> & candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
	{
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

	void createColorResources()
	{
		vk::Format colorFormat = imagePresenter->get_swap_chain_image_format();

		colorImage = std::make_shared<vkCgbImage>(transferCommandBufferManager, imagePresenter->get_swap_chain_extent().width, imagePresenter->get_swap_chain_extent().height, 1, vkContext::instance().msaaSamples, colorFormat,
												  vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
		colorImage->transition_image_layout(colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1);
	}

	void createVRSImageResources()
	{
		vk::Format colorFormat = vk::Format::eR8Uint;
		auto width = imagePresenter->get_swap_chain_extent().width / vkContext::instance().shadingRateImageProperties.shadingRateTexelSize.width;
		auto height = imagePresenter->get_swap_chain_extent().height / vkContext::instance().shadingRateImageProperties.shadingRateTexelSize.height;

		vk::Format colorFormatDebug = imagePresenter->get_swap_chain_image_format();

		vrsImages.resize(vkContext::instance().vkContext::instance().dynamicRessourceCount);
		vrsDebugImages.resize(vkContext::instance().vkContext::instance().dynamicRessourceCount);
		vrsDebugTextureImages.resize(vkContext::instance().vkContext::instance().dynamicRessourceCount);
		for (int i = 0; i < vkContext::instance().vkContext::instance().dynamicRessourceCount; i++) {
			vrsImages[i] = std::make_shared<vkCgbImage>(transferCommandBufferManager, width, height, 1, vk::SampleCountFlagBits::e1, colorFormat,
														vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eShadingRateImageNV | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
			vrsImages[i]->transition_image_layout(colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eShadingRateOptimalNV, 1); // vk::ImageLayout::eShadingRateOptimalNV

			// Debug image
			vrsDebugImages[i] = std::make_shared<vkCgbImage>(transferCommandBufferManager, width, height, 1, vk::SampleCountFlagBits::e1, colorFormatDebug,
															 vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
			vrsDebugImages[i]->transition_image_layout(colorFormatDebug, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, 1);

			vrsDebugTextureImages[i] = std::make_shared <vkTexture>(vrsDebugImages[i].get());
		}
	}
};


int main()
{
	try {
		auto selectImageFormat = cgb::context_specific_function<cgb::image_format()>{}
		.SET_VULKAN_FUNCTION([]() { return cgb::image_format(vk::Format::eR8G8B8Unorm); })
			.SET_OPENGL46_FUNCTION([]() { return cgb::image_format{ GL_RGB };  });

		cgb::settings::gApplicationName = "Hello VRS";
		cgb::settings::gApplicationVersion = cgb::make_version(1, 0, 0);
		cgb::settings::gRequiredInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME);

		// Create a window which we're going to use to render to
		auto windowParams = cgb::window_params{
			std::nullopt,
			std::nullopt,
			"Hello VRS World!"
		};
		auto mainWnd = cgb::context().create_window(windowParams, cgb::swap_chain_params{});

		// Create a "behavior" which contains functionality of our program
		auto vrsBehavior = vrs_behavior();

		// Create a composition of all things that define the essence of 
		// our program, which there are:
		//  - a timer
		//  - an executor
		//  - a window
		//  - a behavior
		auto hello = cgb::composition<cgb::varying_update_only_timer, cgb::sequential_executor>({
				mainWnd
				}, {
					&vrsBehavior
				});

		// Let's go:
		hello.start();
	}
	catch (std::runtime_error & re) {
		LOG_ERROR_EM(re.what());
	}
}

