#pragma once
#include "cg_base.h"

namespace cgb
{
	class window;

	class window_base
	{
		friend class generic_glfw;
	public:
		window_base();
		~window_base();
		window_base(const window_base&) = delete;
		window_base(window_base&&) noexcept;
		window_base& operator =(const window_base&) = delete;
		window_base& operator =(window_base&&) noexcept;

		/** Returns whether or not this window is currently in use and hence, may not be closed. */
		bool is_in_use() const { return mIsInUse; }

		/** @brief Consecutive ID, identifying a window.
		 *	First window will get the ID=0, second one ID=1, etc.  */
		uint32_t id() const { return mWindowId; }

		/** Returns the window handle or std::nullopt if
		 *	it wasn't constructed successfully, has been moved from,
		 *	or has been destroyed. */
		std::optional<window_handle> handle() const { return mHandle; }

		/** Returns the aspect ratio of the window, which is width/height */
		float aspect_ratio() const;

		/** The window title */
		const std::string& title() const { return mTitle; }

		/** Returns the monitor handle or std::nullopt if there is 
		 *  no monitor assigned to this window (e.g. not running
		 *  in full-screen mode.
		 */
		std::optional<monitor_handle> monitor() const { return mMonitor; }

		/**	Returns true if the input of this window will be regarded,
		 *	false if the input of this window will be ignored.
		 */
		bool is_input_enabled() const { return mIsInputEnabled; }

		/** Sets whether or not the window is in use. Setting this to true has the
		 *	effect that the window can not be closed for the time being.
		 */
		void set_is_in_use(bool value);

		/** Set a new resolution for this window. This will also update
		 *  this window's underlying framebuffer 
		 *  TODO: Resize underlying framebuffer!
		 */
		void set_resolution(window_size pExtent);

		/** Set a new title */
		void set_title(std::string pTitle);

		/** Enable or disable input handling of this window */
		void set_is_input_enabled(bool pValue);

		/** Indicates whether or not this window has already been created. */
		bool is_alive() const { return mHandle.has_value(); }

		/** Indicates whether or not this window must be recreated (because parameters have changed or so). */
		bool must_be_recreated() const { return mRecreationRequired; }

		/** Sets this window to fullscreen mode 
		 *	@param	pOnWhichMonitor	Handle to the monitor where to present the window in full screen mode
		 */
		void switch_to_fullscreen_mode(monitor_handle pOnWhichMonitor = monitor_handle::primary_monitor());

		/** Switches to windowed mode by removing this window's monitor assignment */
		void switch_to_windowed_mode();

		/** Hides or shows the cursor */
		void hide_cursor(bool pHide);

		/** Sets the cursor to the given coordinates */
		void set_cursor_pos(glm::dvec2 pCursorPos);


		/** Get the cursor position w.r.t. the given window 
		 */
		glm::dvec2 cursor_position() const;

		glm::dvec2 scroll_position() const;

		/** Determine the window's extent 
		 */
		glm::uvec2 resolution() const;

		/** Returns whether or not the cursor is hidden 
		 *	Threading: This method must always be called from the main thread
		 */
		bool is_cursor_hidden() const;

	protected:
		/** Static variable which holds the ID that the next window will get assigned */
		static uint32_t mNextWindowId;

		/** A flag indicating if this window is currently in use and hence, may not be closed */
		bool mIsInUse;

		/** Unique window id */
		uint32_t mWindowId; 

		/** Handle of this window */
		std::optional<window_handle> mHandle;

		/** This window's title */
		std::string mTitle;

		/** Monitor this window is attached to, if set */
		std::optional<monitor_handle> mMonitor;

		/** A flag which tells if this window is enabled for receiving input (w.r.t. a running composition) */
		bool mIsInputEnabled;
		
		// A flag to indicate that window recreation is required in order to apply new parameters 
		bool mRecreationRequired;

		// The requested window size which only has effect BEFORE the window was created 
		window_size mRequestedSize;

		glm::dvec2 mCursorPosition;

		glm::dvec2 mScrollPosition;

		glm::uvec2 mResultion;

		bool mIsCursorHidden;

		// Actions to be executed after the actual window (re-)creation
		std::vector<std::function<void(window&)>> mPostCreateActions;

		// Cleanup actions which are executed before the window will be destroyed
		std::vector<std::function<void(window&)>> mCleanupActions;
	};
}