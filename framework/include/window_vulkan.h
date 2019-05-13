#pragma once
#include <vulkan/vulkan.hpp>
#include "window_vulkan.h"
#include "context_vulkan_types.h"
#include "context_generic_glfw.h"
#include "window_base.h"

namespace cgb
{
	class window : public window_base
	{
		friend class generic_glfw;
		friend class vulkan;
	public:
		window();
		~window();
		window(const window&) = delete;
		window(window&&) noexcept;
		window& operator =(const window&) = delete;
		window& operator =(window&&) noexcept;

		/** Request a framebuffer for this window which is capable of sRGB formats */
		void request_srgb_framebuffer(bool pRequestSrgb);

		/** Sets the presentation mode for this window's swap chain. */
		void set_presentaton_mode(cgb::presentation_mode pMode);

		/** Sets the number of samples for MSAA */
		void set_number_of_samples(int pNumSamples);

		/** Sets the number of presentable images for a swap chain */
		void set_number_of_presentable_images(uint32_t pNumImages);

		/** Sets the number of images which can be rendered into concurrently,
		 *	i.e. the number of "frames in flight"
		 */
		void set_number_of_concurrent_frames(uint32_t pNumConcurrent);

		/** Creates or opens the window */
		void open();

		/** Gets the requested surface format for the given surface.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::SurfaceFormatKHR get_config_surface_format(const vk::SurfaceKHR& surface);

		/** Gets the requested presentation mode for the given surface.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::PresentModeKHR get_config_presentation_mode(const vk::SurfaceKHR& surface);

		/**	Gets the number of samples that has been configured.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::SampleCountFlagBits get_config_number_of_samples();

		/** Gets the multisampling-related config info struct for the Vk-pipeline config.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::PipelineMultisampleStateCreateInfo get_config_multisample_state_create_info();

		/** Get the minimum number of concurrent/presentable images for a swap chain.
		*	If no value is set, the surfaces minimum number + 1 will be returned.
		*/
		uint32_t get_config_number_of_presentable_images();

		/** Get the number of concurrent frames.
		*	If no value is explicitely set, the same number as the number of presentable images will be returned.
		*/
		uint32_t get_config_number_of_concurrent_frames();

		/** Gets this window's surface */
		const auto& surface() const { 
			return mSurface; 
		}
		/** Gets this window's swap chain */
		const auto& swap_chain() const { 
			return mSwapChain; 
		}
		/** Gets this window's swap chain's image format */
		const auto& swap_chain_image_format() const { 
			return mSwapChainImageFormat; 
		}
		/** Gets this window's swap chain's dimensions */
		auto swap_chain_extent() const {
			return mSwapChainExtent; 
		}
		/** Gets a collection containing all this window's swap chain images. */
		const auto& swap_chain_images() { 
			return mSwapChainImages;
		}
		/** Gets this window's swap chain's image at the specified index. */
		const auto& swap_chain_image_at_index(size_t pIdx) { 
			return mSwapChainImages[pIdx]; 
		}
		/** Gets a collection containing all this window's swap chain image views. */
		const auto& swap_chain_image_views() { 
			return mSwapChainImageViews; 
		}
		/** Gets this window's swap chain's image view at the specified index. */
		const auto& swap_chain_image_view_at_index(size_t pIdx) { 
			return mSwapChainImageViews[pIdx]; 
		}

		/** Gets the number of how many images there are in the swap chain. */
		auto number_of_swapchain_images() const {
			return mSwapChainImageViews.size(); 
		}
		/** Gets the number of how many frames are (potentially) concurrently rendered into. */
		auto number_of_concurrent_frames() const { 
			return mFences.size(); 
		}

		/** Increments the internal frame counter and returns the new frame index.
		 *	@return Returns the already incremented current frame index (i.e. the "index of the next frame")
		 */
		auto increment_current_frame() { 
			return ++mCurrentFrame; 
		}
		/** Gets the current frame index. */
		auto current_frame() const { 
			return mCurrentFrame; 
		}
		/** Helper function which calculates the image index for the given frame index. 
		 *	@param pFrameIndex Frame index which to calculate the corresponding image index for.
		 *	@return Returns the image index for the given frame index.
		 */
		auto calculate_image_index_for_frame(int64_t pFrameIndex) const { 
			return pFrameIndex % number_of_swapchain_images(); 
		}
		/** Helper function which calculates the sync index for the given frame index. 
		*	@param pFrameIndex Frame index which to calculate the corresponding image index for.
		*	@return Returns the sync index for the given frame index.
		*/
		auto calculate_sync_index_for_frame(int64_t pFrameIndex) const {
			return pFrameIndex % number_of_concurrent_frames(); 
		}

		/** Returns the image index for the requested frame.
		 *	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		 *								Specify a negative offset to refer to a previous frame.
		 */
		auto image_index_for_frame(int64_t pCurrentFrameOffset = 0) const { 
			return calculate_image_index_for_frame(static_cast<int64_t>(current_frame()) + pCurrentFrameOffset); 
		}
		/** Returns the sync index for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		auto sync_index_for_frame(int64_t pCurrentFrameOffset = 0) const { 
			return calculate_sync_index_for_frame(static_cast<int64_t>(current_frame()) + pCurrentFrameOffset); 
		}
		
		/** Returns the swap chain image for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& image_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mSwapChainImages[image_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the swap chain image view for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& image_view_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mSwapChainImageViews[image_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the fence for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& fence_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mFences[sync_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the "image available"-semaphore for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& image_available_semaphore_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mImageAvailableSemaphores[sync_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the "render finished"-semaphore for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& render_finished_semaphore_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mRenderFinishedSemaphores[sync_index_for_frame(pCurrentFrameOffset)];
		}

	protected:
		// A function which returns the surface format for this window's surface
		std::function<vk::SurfaceFormatKHR(const vk::SurfaceKHR&)> mSurfaceFormatSelector;

		// A function which returns the desired presentation mode for this window's surface
		std::function<vk::PresentModeKHR(const vk::SurfaceKHR&)> mPresentationModeSelector;

		// A function which returns the MSAA sample count for this window's surface
		std::function<vk::SampleCountFlagBits()> mNumberOfSamplesGetter;

		// A function which returns the MSAA state for this window's surface
		std::function<vk::PipelineMultisampleStateCreateInfo()> mMultisampleCreateInfoBuilder;

		// A function which returns the desired number of presentable images in the swap chain
		std::function<uint32_t()> mNumberOfPresentableImagesGetter;

		// A function which returns the number of images which can be rendered into concurrently
		// According to this number, the number of semaphores and fences will be determined.
		std::function<uint32_t()> mNumberOfConcurrentFramesGetter;

	protected: /* Data for this window */

		// The window's surface
		vk::SurfaceKHR mSurface;
		// The swap chain for this surface
		vk::SwapchainKHR mSwapChain;
		// The swap chain's image format
		image_format mSwapChainImageFormat;
		// The swap chain's extent
		vk::Extent2D mSwapChainExtent;
		// All the images of the swap chain
		std::vector<vk::Image> mSwapChainImages;
		// All the image views of the swap chain
		std::vector<vk::ImageView> mSwapChainImageViews;
		// The frame counter/frame id/frame index/current frame number
		size_t mCurrentFrame;
		// Fences to synchronize between frames (CPU-GPU synchronization)
		std::vector<vk::Fence> mFences; 
		// Semaphores to wait for an image to become available (GPU-GPU synchronization) // TODO: true?
		std::vector<vk::Semaphore> mImageAvailableSemaphores; 
		// Semaphores to wait for rendering to finish (GPU-GPU synchronization) // TODO: true?
		std::vector<vk::Semaphore> mRenderFinishedSemaphores; 

		// The backbuffer of this window
		framebuffer mBackBuffer;

		// The render pass for this window's UI calls
		vk::RenderPass mUiRenderPass;
	};
}