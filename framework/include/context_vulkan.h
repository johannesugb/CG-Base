#pragma once

// DEFINES:
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define VK_USE_PLATFORM_WIN32_KHR

// INCLUDES:
#include <vulkan/vulkan.hpp>
#include "context_vulkan_types.h"
#include "context_generic_glfw.h"

namespace cgb
{
	// =============================== type aliases =================================
	using swap_chain_data_ptr = std::unique_ptr<swap_chain_data>;

	// ============================== VULKAN CONTEXT ================================
	/**	@brief Context for Vulkan
	 *
	 *	This context abstracts calls to the Vulkan API, for environment-related
	 *	stuff, like window creation etc.,  it relies on GLFW and inherits
	 *	@ref generic_glfw.
	 */
	class vulkan : public generic_glfw
	{
		friend struct texture_handle;
		friend struct image_format;
		friend struct swap_chain_data;
		friend struct shader_handle;
		friend struct pipeline;
		friend struct framebuffer;
		friend struct command_pool;
		friend struct command_buffer;
	public:
		static size_t sSettingMaxFramesInFlight;

		vulkan();
		vulkan(const vulkan&) = delete;
		vulkan(vulkan&&) = delete;
		vulkan& operator=(const vulkan&) = delete;
		vulkan& operator=(vulkan&&) = delete;
		virtual ~vulkan();

		vk::Instance& vulkan_instance() { return mInstance; }
		vk::PhysicalDevice& physical_device() { return mPhysicalDevice; }
		vk::Device& logical_device() { return mLogicalDevice; }
		vk::Queue& graphics_queue() { return mGraphicsQueue; }
		vk::Queue& presentation_queue() { return mPresentQueue; }

		window* create_window(const window_params&, const swap_chain_params&);

		texture_handle create_texture()
		{
			return texture_handle();
		}

		void destroy_texture(const texture_handle& pHandle)
		{
		}

		void draw_triangle(const pipeline& pPipeline, const command_buffer& pCommandBuffer);

		void draw_vertices(const pipeline& pPipeline, const command_buffer& pCommandBuffer, vk::ArrayProxy<const vk::Buffer> pBuffers, uint32_t pVertexCount);

		/** Completes all pending work on the device, blocks the current thread until then. */
		void finish_pending_work();

		/** Used to signal the context about the beginning of a composition */
		void begin_composition();

		/** Used to signal the context about the end of a composition */
		void end_composition();

		/** Used to signal the context about the beginning of a new frame */
		void begin_frame();

		/** Used to signal the context about the end of a frame */
		void end_frame();

	public: // TODO: private
		/** Queries the instance layer properties for validation layers 
		 *  and returns true if a layer with the given name could be found.
		 *  Returns false if not found. 
		 */
		static bool is_validation_layer_supported(const char* pName);

		/**	Compiles all those entries from @ref settings::gValidationLayersToBeActivated into
		 *	an array which are supported by the instance. A warning will be issued for those
		 *	entries which are not supported.
		 */
		auto assemble_validation_layers();

		/** Create a new vulkan instance with all the application information and
		 *	also set the required instance extensions which GLFW demands.
		 */
		void create_instance();

		/** Create semaphores and fences according to the sActualMaxFramesInFlight parameter,
		 *	(which is set to sSettingMaxFramesInFlight in the constructor)
		 */
		void create_sync_objects();

		/** Cleans up all the semaphores and fences
		 */
		void cleanup_sync_objects();

		/** Method which handles debug callbacks from the validation layers */
		static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

		/** Set up the debug callbacks, i.e. hook into vk to have @ref vk_debug_callback called */
		void setup_vk_debug_callback();

		/** Creates a surface for the given window */
		vk::SurfaceKHR create_surface_for_window(const window* pWindow);

		/** 
		 *	@return Pointer to the tuple or nullptr if not found
		 */
		swap_chain_data* get_surf_swap_tuple_for_window(const window* pWindow);
		
		/**
		 *	@return Pointer to the tuple or nullptr if not found
		 */
		swap_chain_data* get_surf_swap_tuple_for_surface(const vk::SurfaceKHR& pSurface);

		/**
		 *	@return Pointer to the tuple or nullptr if not found
		 */
		swap_chain_data* get_surf_swap_tuple_for_swap_chain(const vk::SwapchainKHR& pSwapChain);

		/** Returns a vector containing all elements from @ref sRequiredDeviceExtensions
		 *  and settings::gRequiredDeviceExtensions
		 */
		static std::vector<const char*> get_all_required_device_extensions();
		
		/** Checks whether the given physical device supports all the required extensions,
		 *	namely those stored in @ref settings::gRequiredDeviceExtensions. 
		 *	Returns true if it does, false otherwise.
		 */
		static bool supports_all_required_extensions(const vk::PhysicalDevice& device);

		/** Pick the physical device which looks to be the most promising one */
		void pick_physical_device();


		/**	Finds all queue families which support certain criteria which are defined by the parameters.
		 *	@param pRequiredFlags	If set, a queue family must support the set flags
		 *	@param pSurface			If set, the queue family must support the given surface
		 *	@return		All which support them are returned as a vector of tuples of indices and data.
		 *				The index is important for further vk-calls and is stored in the first element
		 *				of the tuple, i.e. use @ref std::get<0>() to get the index, @ref std::get<1>() 
		 *				for the data
		 */
		auto find_queue_families_for_criteria(std::optional<vk::QueueFlagBits> pRequiredFlags, std::optional<vk::SurfaceKHR> pSurface);

		/**
		 *
		 */
		void create_and_assign_logical_device(vk::SurfaceKHR pSurface);

		/** Creates the swap chain for the given window and surface with the given parameters
		 *	@param pWindow		[in] The window to create the swap chain for
		 *	@param pSurface		[in] the surface to create the swap chain for
		 *	@param pParams		[in] swap chain creation parameters
		 *	@return				A newly created swap chain
		 */
		swap_chain_data create_swap_chain(const window* pWindow, const vk::SurfaceKHR& pSurface, const swap_chain_params& pParams);

		/** TODO: TBD */
		vk::RenderPass create_render_pass(image_format pImageFormat);

		/** TODO: TBD */
		pipeline create_graphics_pipeline_for_window(const std::vector<std::tuple<shader_type, shader_handle*>>& pShaderInfos, const window* pWindow, const vk::VertexInputBindingDescription& pBindingDesc, const std::array<vk::VertexInputAttributeDescription, 2>& pAttributeDesc);
		/** TODO: TBD */
		pipeline create_graphics_pipeline_for_swap_chain(const std::vector<std::tuple<shader_type, shader_handle*>>& pShaderInfos, const swap_chain_data& pSwapChainData, const vk::VertexInputBindingDescription& pBindingDesc, const std::array<vk::VertexInputAttributeDescription, 2>& pAttributeDesc);

		std::vector<framebuffer> create_framebuffers(const vk::RenderPass& renderPass, const window* pWindow);
		std::vector<framebuffer> create_framebuffers(const vk::RenderPass& renderPass, const swap_chain_data& pSwapChainData);

		command_pool create_command_pool();

		std::vector<command_buffer> create_command_buffers(uint32_t pCount, const command_pool& pCommandPool);
		
		/** Calculates the semaphore index of the current frame */
		size_t sync_index_curr_frame() const { return mFrameCounter % sActualMaxFramesInFlight; }

		/** Calculates the semaphore index of the previous frame */
		size_t sync_index_prev_frame() const { return (mFrameCounter - 1) % sActualMaxFramesInFlight; }

		vk::Semaphore& image_available_semaphore_current_frame() { return mImageAvailableSemaphores[sync_index_curr_frame()]; }

		vk::Semaphore& render_finished_semaphore_current_frame() { return mRenderFinishedSemaphores[sync_index_curr_frame()]; }

		vk::Fence& fence_current_frame() { return mInFlightFences[sync_index_curr_frame()]; }

		/** Find (index of) memory with parameters
		 *	@param pMemoryTypeBits		Bit field of the memory types that are suitable for the buffer. [9]
		 *	@param pMemoryProperties	Special features of the memory, like being able to map it so we can write to it from the CPU. [9]
		 */
		uint32_t find_memory_type_index(uint32_t pMemoryTypeBits, vk::MemoryPropertyFlags pMemoryProperties);

	private:
		static std::vector<const char*> sRequiredDeviceExtensions;
		static size_t sActualMaxFramesInFlight;
		size_t mFrameCounter;
		std::vector<vk::Semaphore> mImageAvailableSemaphores; // GPU-GPU synchronization
		std::vector<vk::Semaphore> mRenderFinishedSemaphores; // GPU-GPU synchronization
		std::vector<vk::Fence> mInFlightFences;  // CPU-GPU synchronization
		vk::Instance mInstance;
		VkDebugUtilsMessengerEXT mDebugCallbackHandle;
		std::vector<swap_chain_data_ptr> mSurfSwap;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mLogicalDevice;
		vk::Queue mGraphicsQueue;
		vk::Queue mPresentQueue;
	};
}
