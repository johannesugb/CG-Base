#include "window_vulkan.h"

namespace cgb
{
	void window::request_srgb_framebuffer(bool pRequestSrgb)
	{
		// Which formats are supported, depends on the surface.
		mSurfaceFormatSelector = [srgbFormatRequested = pRequestSrgb](const vk::SurfaceKHR & pSurface) {
			// Get all the formats which are supported by the surface:
			auto srfFrmts = context().physical_device().getSurfaceFormatsKHR(pSurface);

			// Init with a default format...
			auto selSurfaceFormat = vk::SurfaceFormatKHR{
				vk::Format::eB8G8R8A8Unorm,
				vk::ColorSpaceKHR::eSrgbNonlinear
			};

			// ...and try to possibly find one which is definitely supported or better suited w.r.t. the surface.
			if (!(srfFrmts.size() == 1 && srfFrmts[0].format == vk::Format::eUndefined)) {
				for (const auto& e : srfFrmts) {
					if (srgbFormatRequested) {
						if (is_srgb_format(cgb::image_format(e))) {
							selSurfaceFormat = e;
							break;
						}
					}
					else {
						if (!is_srgb_format(cgb::image_format(e))) {
							selSurfaceFormat = e;
							break;
						}
					}
				}
			}

			// In any case, return a format
			return selSurfaceFormat;
		};

		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_presentaton_mode(cgb::presentation_mode pMode)
	{
		mPresentationModeSelector = [presMode = pMode](const vk::SurfaceKHR & pSurface) {
			// Supported presentation modes must be queried from a device:
			auto presModes = context().physical_device().getSurfacePresentModesKHR(pSurface);

			// Select a presentation mode:
			auto selPresModeItr = presModes.end();
			switch (presMode) {
			case cgb::presentation_mode::immediate:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eImmediate);
				break;
			case cgb::presentation_mode::double_buffering:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifoRelaxed);
				break;
			case cgb::presentation_mode::vsync:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifo);
				break;
			case cgb::presentation_mode::triple_buffering:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eMailbox);
				break;
			default:
				throw std::runtime_error("should not get here");
			}
			if (selPresModeItr == presModes.end()) {
				LOG_WARNING_EM("No presentation mode specified or desired presentation mode not available => will select any presentation mode");
				selPresModeItr = presModes.begin();
			}

			return *selPresModeItr;
		};

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_samples(int pNumSamples)
	{
		mNumberOfSamplesGetter = [samples = to_vk_sample_count(pNumSamples)]() { return samples; };

		mMultisampleCreateInfoBuilder = [this]() {
			auto samples = mNumberOfSamplesGetter();
			return vk::PipelineMultisampleStateCreateInfo()
				.setSampleShadingEnable(vk::SampleCountFlagBits::e1 == samples ? VK_FALSE : VK_TRUE) // disable/enable?
				.setRasterizationSamples(samples)
				.setMinSampleShading(1.0f) // Optional
				.setPSampleMask(nullptr) // Optional
				.setAlphaToCoverageEnable(VK_FALSE) // Optional
				.setAlphaToOneEnable(VK_FALSE); // Optional
		};

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_presentable_images(uint32_t pNumImages)
	{
		mNumberOfPresentableImagesGetter = [numImages = pNumImages]() { return numImages; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_concurrent_frames(uint32_t pNumConcurrent)
	{
		mNumberOfConcurrentFramesGetter = [numConcurrent = pNumConcurrent]() { return numConcurrent; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::open()
	{
		context().dispatch_to_main_thread([this]() {
			// Ensure, previous work is done:
			context().work_off_event_handlers();

			// Share the graphics context between all windows
			auto* sharedContex = context().get_window_for_shared_context();
			// Bring window into existance:
			auto* handle = glfwCreateWindow(mRequestedSize.mWidth, mRequestedSize.mHeight,
				mTitle.c_str(),
				mMonitor.has_value() ? mMonitor->mHandle : nullptr,
				sharedContex);
			if (nullptr == handle) {
				// No point in continuing
				throw new std::runtime_error("Failed to create window with the title '" + mTitle + "'");
			}
			mHandle = window_handle{ handle };

			// There will be some pending work regarding this newly created window stored within the
			// context's events, like creating a swap chain and so on. 
			// Why wait? Invoke them now!
			context().work_off_event_handlers();
		});
	}

	vk::SurfaceFormatKHR window::get_config_surface_format(const vk::SurfaceKHR & surface)
	{
		if (!mSurfaceFormatSelector) {
			// Set the default:
			request_srgb_framebuffer(false);
		}
		// Determine the format:
		return mSurfaceFormatSelector(surface);
	}

	vk::PresentModeKHR window::get_config_presentation_mode(const vk::SurfaceKHR & surface)
	{
		if (!mPresentationModeSelector) {
			// Set the default:
			set_presentaton_mode(cgb::presentation_mode::triple_buffering);
		}
		// Determine the presentation mode:
		return mPresentationModeSelector(surface);
	}

	vk::SampleCountFlagBits window::get_config_number_of_samples()
	{
		if (!mNumberOfSamplesGetter) {
			// Set the default:
			set_number_of_samples(1);
		}
		// Determine the number of samples:
		return mNumberOfSamplesGetter();
	}

	vk::PipelineMultisampleStateCreateInfo window::get_config_multisample_state_create_info()
	{
		if (!mMultisampleCreateInfoBuilder) {
			// Set the default:
			set_number_of_samples(1);
		}
		// Get the config struct:
		return mMultisampleCreateInfoBuilder();
	}

	uint32_t window::get_config_number_of_presentable_images()
	{
		if (!mNumberOfPresentableImagesGetter) {
			auto srfCaps = context().physical_device().getSurfaceCapabilitiesKHR(surface());
			auto imageCount = srfCaps.minImageCount + 1u;
			if (srfCaps.maxImageCount > 0) { // A value of 0 for maxImageCount means that there is no limit
				imageCount = glm::min(imageCount, srfCaps.maxImageCount);
			}
			return imageCount;
		}
		return mNumberOfPresentableImagesGetter();
	}

	uint32_t window::get_config_number_of_concurrent_frames()
	{
		if (!mNumberOfConcurrentFramesGetter) {
			return get_config_number_of_presentable_images();
		}
		return mNumberOfConcurrentFramesGetter();
	}

	void window::set_extra_semaphore_dependency_for_frame(semaphore pSemaphore, uint64_t pFrameId)
	{
		mExtraSemaphoreDependencies.emplace_back(pFrameId, std::move(pSemaphore));
	}

	std::vector<semaphore> window::remove_all_extra_semaphore_dependencies_for_frame(uint64_t pFrameId)
	{
		// Find all to remove
		auto to_remove = std::remove_if(
			std::begin(mExtraSemaphoreDependencies), std::end(mExtraSemaphoreDependencies),
			[frameId = pFrameId](const auto& tpl) {
				return std::get<uint64_t>(tpl) == frameId;
			});
		// return ownership of all the semaphores to remove to the caller
		std::vector<semaphore> moved_semaphores;
		for (decltype(to_remove) it = to_remove; it != std::end(mExtraSemaphoreDependencies); ++it) {
			moved_semaphores.push_back(std::move(std::get<semaphore>(*it)));
		}
		// Erase and return
		mExtraSemaphoreDependencies.erase(to_remove, std::end(mExtraSemaphoreDependencies));
		return moved_semaphores;
	}

	void window::fill_in_extra_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& pSemaphores, uint64_t pFrameId)
	{
		for (const auto& [frameId, sem] : mExtraSemaphoreDependencies) {
			if (frameId == pFrameId) {
				pSemaphores.push_back(sem.handle());
			}
		}
	}

	void window::fill_in_extra_render_finished_semaphores_for_frame(std::vector<vk::Semaphore>& pSemaphores, uint64_t pFrameId)
	{
		// TODO: Fill mExtraRenderFinishedSemaphores with meaningful data
		auto si = sync_index_for_frame();
		for (auto i = si; i < si + mNumExtraRenderFinishedSemaphoresPerFrame; ++i) {
			pSemaphores.push_back(mExtraRenderFinishedSemaphores[i].handle());
		}
	}

	/*std::vector<semaphore> window::set_num_extra_semaphores_to_generate_per_frame(uint32_t pNumExtraSemaphores)
	{

	}*/

	void window::render_frame(std::initializer_list<std::reference_wrapper<cgb::command_buffer>> pCommandBuffers)
	{
		// Wait for the fence before proceeding, GPU -> CPU synchronization via fence
		const auto& fence = fence_for_frame();
		cgb::context().logical_device().waitForFences(1u, fence.handle_addr(), VK_TRUE, std::numeric_limits<uint64_t>::max());
		cgb::context().logical_device().resetFences(1u, fence.handle_addr());


		//
		//
		//
		//	TODO: Recreate swap chain probably somewhere here
		//  Potential problems:
		//	 - How to handle the fences? Is waitIdle enough?
		//	 - A problem might be the multithreaded access to this function... hmm... or is it??
		//      => Now would be the perfect time to think about how to handle parallel executors
		//		   Only Command Buffer generation should be parallelized anyways, submission should 
		//		   be done on ONE thread, hence access to this method would be syncronized inherently, right?!
		//
		//	What about the following: Tie an instance of cg_element to ONE AND EXACTLY ONE window*?!
		//	 => Then, the render method would create a command_buffer, which is then gathered (per window!) and passed on to this method.
		//
		//
		//


		// Get the next image from the swap chain, GPU -> GPU sync from previous present to the following acquire
		uint32_t imageIndex;
		const auto& imgAvailableSem = image_available_semaphore_for_frame();
		cgb::context().logical_device().acquireNextImageKHR(
			swap_chain(), // the swap chain from which we wish to acquire an image 
			std::numeric_limits<uint64_t>::max(), // a timeout in nanoseconds for an image to become available. Using the maximum value of a 64 bit unsigned integer disables the timeout. [1]
			imgAvailableSem.handle(), // The next two parameters specify synchronization objects that are to be signaled when the presentation engine is finished using the image [1]
			nullptr,
			&imageIndex); // a variable to output the index of the swap chain image that has become available. The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick the right command buffer. [1]

						  // Assemble the command buffers...
						  //std::array<vk::CommandBuffer, sizeof...(pCommandBuffers) + 1> cmdBuffers = {{
						  //	pCommandBuffer.mCommandBuffer,
						  //	pCommandBuffers.mCommandBuffer... 
						  //}};
		std::vector<vk::CommandBuffer> cmdBuffers;
		for (auto cb : pCommandBuffers) {
			cmdBuffers.push_back(cb.get().handle());
		}

		// ...and submit them. But also assemble several GPU -> GPU sync objects for both, inbound and outbound sync:
		// Wait for some extra semaphores, if there are any; i.e. GPU -> GPU sync from acquire to the following submit
		std::vector<vk::Semaphore> waitBeforeExecuteSemaphores = { imgAvailableSem.handle() };
		fill_in_extra_semaphore_dependencies_for_frame(waitBeforeExecuteSemaphores, current_frame());
		// For every semaphore, also add a entry for the corresponding stage:
		std::vector<vk::PipelineStageFlags> waitBeforeExecuteStages;
		std::transform( std::begin(waitBeforeExecuteSemaphores), std::end(waitBeforeExecuteSemaphores),
						std::back_inserter(waitBeforeExecuteStages),
						[](const auto & s) { return vk::PipelineStageFlagBits::eColorAttachmentOutput; });
		// Signal at least one semaphore when done, potentially also more.
		const auto& renderFinishedSem = render_finished_semaphore_for_frame();
		std::vector<vk::Semaphore> toSignalAfterExecute = { renderFinishedSem.handle() };
		fill_in_extra_render_finished_semaphores_for_frame(toSignalAfterExecute, current_frame());
		auto submitInfo = vk::SubmitInfo()
			.setWaitSemaphoreCount(static_cast<uint32_t>(waitBeforeExecuteSemaphores.size()))
			.setPWaitSemaphores(waitBeforeExecuteSemaphores.data())
			.setPWaitDstStageMask(waitBeforeExecuteStages.data())
			.setCommandBufferCount(static_cast<uint32_t>(cmdBuffers.size()))
			.setPCommandBuffers(cmdBuffers.data())
			.setSignalSemaphoreCount(static_cast<uint32_t>(toSignalAfterExecute.size()))
			.setPSignalSemaphores(toSignalAfterExecute.data());
		// Finally, submit to the graphics queue.
		// Also provide a fence for GPU -> CPU sync which will be waited on next time we need this frame (top of this method).
		cgb::context().graphics_queue().handle().submit(1u, &submitInfo, fence.handle());

		// Present as soon as the render finished semaphore has been signalled:
		auto presentInfo = vk::PresentInfoKHR()
			.setWaitSemaphoreCount(1u)
			.setPWaitSemaphores(&renderFinishedSem.handle())
			.setSwapchainCount(1u)
			.setPSwapchains(&swap_chain())
			.setPImageIndices(&imageIndex)
			.setPResults(nullptr);
		cgb::context().presentation_queue().handle().presentKHR(presentInfo);

		// increment frame counter
		++mCurrentFrame;
	}

}