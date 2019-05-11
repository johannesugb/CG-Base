#include "window_base.h"

namespace cgb
{
	uint32_t window_base::mNextWindowId = 0u;

	window_base::window_base()
		: mIsInUse(false)
		, mWindowId(mNextWindowId++)
		, mTitle()
		, mMonitor()
		, mIsInputEnabled(true)
		, mRequestedSize{ 512, 512 }
		, mCursorPosition{ 0.0, 0.0 }
		, mResultion{ 0, 0 }
		, mIsCursorDisabled{ false }
	{
	}

	window_base::~window_base()
	{ }

	window_base::window_base(window_base&& other) noexcept
		: mIsInUse(std::move(other.mIsInUse))
		, mWindowId(std::move(other.mWindowId))
		, mHandle(std::move(other.mHandle))
		, mTitle(std::move(other.mTitle))
		, mMonitor(std::move(other.mMonitor))
		, mIsInputEnabled(std::move(other.mIsInputEnabled))
		, mRequestedSize(std::move(other.mRequestedSize))
		, mCursorPosition(std::move(other.mCursorPosition))
		, mResultion(std::move(other.mResultion))
		, mIsCursorDisabled(std::move(other.mIsCursorDisabled))
	{
		other.mIsInUse = false;
		other.mWindowId = 0u;
		other.mHandle = std::nullopt;
		other.mTitle = "moved from";
		other.mMonitor = std::nullopt;
		other.mIsInputEnabled = false;
		other.mCursorPosition = { 0.0, 0.0 };
		other.mResultion = { 0, 0 };
		other.mIsCursorDisabled = false;
	}

	window_base& window_base::operator= (window_base&& other) noexcept
	{
		mIsInUse = std::move(other.mIsInUse);
		mWindowId = std::move(other.mWindowId);
		mHandle = std::move(other.mHandle);
		mTitle = std::move(other.mTitle);
		mMonitor = std::move(other.mMonitor);
		mIsInputEnabled = std::move(other.mIsInputEnabled);
		mRequestedSize = std::move(other.mRequestedSize);
		mCursorPosition = std::move(other.mCursorPosition);
		mResultion = std::move(other.mResultion);
		mIsCursorDisabled = std::move(other.mIsCursorDisabled);

		other.mIsInUse = false;
		other.mWindowId = 0u;
		other.mHandle = std::nullopt;
		other.mTitle = "moved from";
		other.mMonitor = std::nullopt;
		other.mIsInputEnabled = false;
		other.mCursorPosition = { 0.0, 0.0 };
		other.mResultion = { 0, 0 };
		other.mIsCursorDisabled = false;

		return *this;
	}

	void window_base::set_is_in_use(bool value)
	{
		mIsInUse = value;
	}

	float window_base::aspect_ratio() const
	{
		auto res = resolution();
		return static_cast<float>(res.x) / static_cast<float>(res.y);
	}

	void window_base::set_resolution(window_size pExtent)
	{
		if (is_alive()) {
			context().dispatch_to_main_thread([this, pExtent]() {
				glfwSetWindowSize(handle()->mHandle, pExtent.mWidth, pExtent.mHeight);
			});
		}
		else {
			mRequestedSize = pExtent;
		}
	}

	void window_base::set_title(std::string pTitle)
	{
		mTitle = pTitle;
		if (mHandle.has_value()) {
			context().dispatch_to_main_thread([this, pTitle]() {
				glfwSetWindowTitle(mHandle->mHandle, pTitle.c_str());
			});
		}
	}

	void window_base::set_is_input_enabled(bool pValue)
	{
		mIsInputEnabled = pValue; 
	}

	void window_base::switch_to_fullscreen_mode(monitor_handle pOnWhichMonitor)
	{
		if (is_alive()) {
			context().dispatch_to_main_thread([this, pOnWhichMonitor]() {
				glfwSetWindowMonitor(handle()->mHandle, pOnWhichMonitor.mHandle,
									 0, 0,
									 mRequestedSize.mWidth, mRequestedSize.mHeight, // TODO: Support different resolutions or query the current resolution
									 GLFW_DONT_CARE); // TODO: Support different refresh rates
			});
		}
		else {
			mMonitor = std::nullopt;
		}
	}

	void window_base::switch_to_windowed_mode()
	{
		if (is_alive()) {
			context().dispatch_to_main_thread([this]() {
				int xpos = 10, ypos = 10;
				glfwGetWindowPos(handle()->mHandle, &xpos, &ypos);
				glfwSetWindowMonitor(handle()->mHandle, nullptr,
									 xpos, ypos,
									 mRequestedSize.mWidth, mRequestedSize.mHeight, // TODO: Support different resolutions or query the current resolution
									 GLFW_DONT_CARE); // TODO: Support different refresh rates
			});
		}
		else {
			mMonitor = std::nullopt;
		}
	}

	glm::dvec2 window_base::cursor_position() const
	{
		return mCursorPosition;
	}

	glm::uvec2 window_base::resolution() const
	{
		return mResultion;
	}

	void window_base::disable_cursor(bool pDisable)
	{
		assert(handle());
		context().dispatch_to_main_thread([this, pDisable]() {
			if (pDisable) {

				// RAAAAW!
				if (glfwRawMouseMotionSupported()) {
					glfwSetInputMode(handle()->mHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
				}

				glfwSetInputMode(handle()->mHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			} 
			else {
				glfwSetInputMode(handle()->mHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}



			mIsCursorDisabled = pDisable;
		});
	}

	bool window_base::is_cursor_disabled() const
	{
		assert(context().are_we_on_the_main_thread());
		return glfwGetInputMode(handle()->mHandle, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
	}

	void window_base::set_cursor_pos(glm::dvec2 pCursorPos)
	{
		assert(handle());
		context().dispatch_to_main_thread([this, newCursorPos = pCursorPos]() {
			assert(handle());
			glfwSetCursorPos(handle()->mHandle, newCursorPos.x, newCursorPos.y);
			mCursorPosition = newCursorPos;
		});
	}

}