#include "context_generic_glfw.h"
#include "window_base.h"
#include "input_buffer.h"
#include "composition_interface.h"

namespace cgb
{
	window* generic_glfw::sWindowInFocus = nullptr;
	std::mutex generic_glfw::sInputMutex;
	std::array<key_code, GLFW_KEY_LAST + 1> generic_glfw::sGlfwToKeyMapping{};
	std::thread::id generic_glfw::sMainThreadId = std::this_thread::get_id();
	std::mutex generic_glfw::sDispatchMutex;

	generic_glfw::generic_glfw()
		: mInitialized(false)

	{
		LOG_VERBOSE("Creating GLFW context...");
	
		// Setting an error callback
		glfwSetErrorCallback(glfw_error_callback);

		// Initializing GLFW
		if (GLFW_TRUE == glfwInit())
		{
			mInitialized = true;
		}
		else
		{
			LOG_ERROR("glfwInit failed");
		}

		for (auto& i : sGlfwToKeyMapping)
		{
			i = key_code::unknown;
		}
		sGlfwToKeyMapping[0] = key_code::unknown;
		sGlfwToKeyMapping[GLFW_KEY_SPACE] = key_code::space;
		sGlfwToKeyMapping[GLFW_KEY_APOSTROPHE] = key_code::apostrophe;
		sGlfwToKeyMapping[GLFW_KEY_COMMA] = key_code::comma;
		sGlfwToKeyMapping[GLFW_KEY_MINUS] = key_code::minus;
		sGlfwToKeyMapping[GLFW_KEY_PERIOD] = key_code::period;
		sGlfwToKeyMapping[GLFW_KEY_SLASH] = key_code::slash;
		sGlfwToKeyMapping[GLFW_KEY_0] = key_code::num0;
		sGlfwToKeyMapping[GLFW_KEY_1] = key_code::num1;
		sGlfwToKeyMapping[GLFW_KEY_2] = key_code::num2;
		sGlfwToKeyMapping[GLFW_KEY_3] = key_code::num3;
		sGlfwToKeyMapping[GLFW_KEY_4] = key_code::num4;
		sGlfwToKeyMapping[GLFW_KEY_5] = key_code::num5;
		sGlfwToKeyMapping[GLFW_KEY_6] = key_code::num6;
		sGlfwToKeyMapping[GLFW_KEY_7] = key_code::num7;
		sGlfwToKeyMapping[GLFW_KEY_8] = key_code::num8;
		sGlfwToKeyMapping[GLFW_KEY_9] = key_code::num9;
		sGlfwToKeyMapping[GLFW_KEY_SEMICOLON] = key_code::semicolon;
		sGlfwToKeyMapping[GLFW_KEY_EQUAL] = key_code::equal;
		sGlfwToKeyMapping[GLFW_KEY_A] = key_code::a;
		sGlfwToKeyMapping[GLFW_KEY_B] = key_code::b;
		sGlfwToKeyMapping[GLFW_KEY_C] = key_code::c;
		sGlfwToKeyMapping[GLFW_KEY_D] = key_code::d;
		sGlfwToKeyMapping[GLFW_KEY_E] = key_code::e;
		sGlfwToKeyMapping[GLFW_KEY_F] = key_code::f;
		sGlfwToKeyMapping[GLFW_KEY_G] = key_code::g;
		sGlfwToKeyMapping[GLFW_KEY_H] = key_code::h;
		sGlfwToKeyMapping[GLFW_KEY_I] = key_code::i;
		sGlfwToKeyMapping[GLFW_KEY_J] = key_code::j;
		sGlfwToKeyMapping[GLFW_KEY_K] = key_code::k;
		sGlfwToKeyMapping[GLFW_KEY_L] = key_code::l;
		sGlfwToKeyMapping[GLFW_KEY_M] = key_code::m;
		sGlfwToKeyMapping[GLFW_KEY_N] = key_code::n;
		sGlfwToKeyMapping[GLFW_KEY_O] = key_code::o;
		sGlfwToKeyMapping[GLFW_KEY_P] = key_code::p;
		sGlfwToKeyMapping[GLFW_KEY_Q] = key_code::q;
		sGlfwToKeyMapping[GLFW_KEY_R] = key_code::r;
		sGlfwToKeyMapping[GLFW_KEY_S] = key_code::s;
		sGlfwToKeyMapping[GLFW_KEY_T] = key_code::t;
		sGlfwToKeyMapping[GLFW_KEY_U] = key_code::u;
		sGlfwToKeyMapping[GLFW_KEY_V] = key_code::v;
		sGlfwToKeyMapping[GLFW_KEY_W] = key_code::w;
		sGlfwToKeyMapping[GLFW_KEY_X] = key_code::x;
		sGlfwToKeyMapping[GLFW_KEY_Y] = key_code::y;
		sGlfwToKeyMapping[GLFW_KEY_Z] = key_code::z;
		sGlfwToKeyMapping[GLFW_KEY_LEFT_BRACKET] = key_code::left_bracket;
		sGlfwToKeyMapping[GLFW_KEY_BACKSLASH] = key_code::backslash;
		sGlfwToKeyMapping[GLFW_KEY_RIGHT_BRACKET] = key_code::right_bracket;
		sGlfwToKeyMapping[GLFW_KEY_GRAVE_ACCENT] = key_code::grave_accent;
		sGlfwToKeyMapping[GLFW_KEY_WORLD_1] = key_code::world_1;
		sGlfwToKeyMapping[GLFW_KEY_WORLD_2] = key_code::world_2;
		sGlfwToKeyMapping[GLFW_KEY_ESCAPE] = key_code::escape;
		sGlfwToKeyMapping[GLFW_KEY_ENTER] = key_code::enter;
		sGlfwToKeyMapping[GLFW_KEY_TAB] = key_code::tab;
		sGlfwToKeyMapping[GLFW_KEY_BACKSPACE] = key_code::backspace;
		sGlfwToKeyMapping[GLFW_KEY_INSERT] = key_code::insert;
		sGlfwToKeyMapping[GLFW_KEY_DELETE] = key_code::del;
		sGlfwToKeyMapping[GLFW_KEY_RIGHT] = key_code::right;
		sGlfwToKeyMapping[GLFW_KEY_LEFT] = key_code::left;
		sGlfwToKeyMapping[GLFW_KEY_DOWN] = key_code::down;
		sGlfwToKeyMapping[GLFW_KEY_UP] = key_code::up;
		sGlfwToKeyMapping[GLFW_KEY_PAGE_UP] = key_code::page_up;
		sGlfwToKeyMapping[GLFW_KEY_PAGE_DOWN] = key_code::page_down;
		sGlfwToKeyMapping[GLFW_KEY_HOME] = key_code::home;
		sGlfwToKeyMapping[GLFW_KEY_END] = key_code::end;
		sGlfwToKeyMapping[GLFW_KEY_CAPS_LOCK] = key_code::caps_lock;
		sGlfwToKeyMapping[GLFW_KEY_SCROLL_LOCK] = key_code::scroll_lock;
		sGlfwToKeyMapping[GLFW_KEY_NUM_LOCK] = key_code::num_lock;
		sGlfwToKeyMapping[GLFW_KEY_PRINT_SCREEN] = key_code::print_screen;
		sGlfwToKeyMapping[GLFW_KEY_PAUSE] = key_code::pause;
		sGlfwToKeyMapping[GLFW_KEY_F1] = key_code::f1;
		sGlfwToKeyMapping[GLFW_KEY_F2] = key_code::f2;
		sGlfwToKeyMapping[GLFW_KEY_F3] = key_code::f3;
		sGlfwToKeyMapping[GLFW_KEY_F4] = key_code::f4;
		sGlfwToKeyMapping[GLFW_KEY_F5] = key_code::f5;
		sGlfwToKeyMapping[GLFW_KEY_F6] = key_code::f6;
		sGlfwToKeyMapping[GLFW_KEY_F7] = key_code::f7;
		sGlfwToKeyMapping[GLFW_KEY_F8] = key_code::f8;
		sGlfwToKeyMapping[GLFW_KEY_F9] = key_code::f9;
		sGlfwToKeyMapping[GLFW_KEY_F10] = key_code::f10;
		sGlfwToKeyMapping[GLFW_KEY_F11] = key_code::f11;
		sGlfwToKeyMapping[GLFW_KEY_F12] = key_code::f12;
		sGlfwToKeyMapping[GLFW_KEY_F13] = key_code::f13;
		sGlfwToKeyMapping[GLFW_KEY_F14] = key_code::f14;
		sGlfwToKeyMapping[GLFW_KEY_F15] = key_code::f15;
		sGlfwToKeyMapping[GLFW_KEY_F16] = key_code::f16;
		sGlfwToKeyMapping[GLFW_KEY_F17] = key_code::f17;
		sGlfwToKeyMapping[GLFW_KEY_F18] = key_code::f18;
		sGlfwToKeyMapping[GLFW_KEY_F19] = key_code::f19;
		sGlfwToKeyMapping[GLFW_KEY_F20] = key_code::f20;
		sGlfwToKeyMapping[GLFW_KEY_F21] = key_code::f21;
		sGlfwToKeyMapping[GLFW_KEY_F22] = key_code::f22;
		sGlfwToKeyMapping[GLFW_KEY_F23] = key_code::f23;
		sGlfwToKeyMapping[GLFW_KEY_F24] = key_code::f24;
		sGlfwToKeyMapping[GLFW_KEY_F25] = key_code::f25;
		sGlfwToKeyMapping[GLFW_KEY_KP_0] = key_code::numpad_0;
		sGlfwToKeyMapping[GLFW_KEY_KP_1] = key_code::numpad_1;
		sGlfwToKeyMapping[GLFW_KEY_KP_2] = key_code::numpad_2;
		sGlfwToKeyMapping[GLFW_KEY_KP_3] = key_code::numpad_3;
		sGlfwToKeyMapping[GLFW_KEY_KP_4] = key_code::numpad_4;
		sGlfwToKeyMapping[GLFW_KEY_KP_5] = key_code::numpad_5;
		sGlfwToKeyMapping[GLFW_KEY_KP_6] = key_code::numpad_6;
		sGlfwToKeyMapping[GLFW_KEY_KP_7] = key_code::numpad_7;
		sGlfwToKeyMapping[GLFW_KEY_KP_8] = key_code::numpad_8;
		sGlfwToKeyMapping[GLFW_KEY_KP_9] = key_code::numpad_9;
		sGlfwToKeyMapping[GLFW_KEY_KP_DECIMAL] = key_code::numpad_decimal;
		sGlfwToKeyMapping[GLFW_KEY_KP_DIVIDE] = key_code::numpad_divide;
		sGlfwToKeyMapping[GLFW_KEY_KP_MULTIPLY] = key_code::numpad_multiply;
		sGlfwToKeyMapping[GLFW_KEY_KP_SUBTRACT] = key_code::numpad_subtract;
		sGlfwToKeyMapping[GLFW_KEY_KP_ADD] = key_code::numpad_add;
		sGlfwToKeyMapping[GLFW_KEY_KP_ENTER] = key_code::numpad_enter;
		sGlfwToKeyMapping[GLFW_KEY_KP_EQUAL] = key_code::numpad_equal;
		sGlfwToKeyMapping[GLFW_KEY_LEFT_SHIFT] = key_code::left_shift;
		sGlfwToKeyMapping[GLFW_KEY_LEFT_CONTROL] = key_code::left_control;
		sGlfwToKeyMapping[GLFW_KEY_LEFT_ALT] = key_code::left_alt;
		sGlfwToKeyMapping[GLFW_KEY_LEFT_SUPER] = key_code::left_super;
		sGlfwToKeyMapping[GLFW_KEY_RIGHT_SHIFT] = key_code::right_shift;
		sGlfwToKeyMapping[GLFW_KEY_RIGHT_CONTROL] = key_code::right_control;
		sGlfwToKeyMapping[GLFW_KEY_RIGHT_ALT] = key_code::right_alt;
		sGlfwToKeyMapping[GLFW_KEY_RIGHT_SUPER] = key_code::right_super;
		sGlfwToKeyMapping[GLFW_KEY_MENU] = key_code::menu;
	}

	generic_glfw::~generic_glfw()
	{
		if (mInitialized)
		{
			mWindows.clear();
			glfwTerminate();
			// context has been destroyed by glfwTerminate
			mInitialized = false;
		}
	}

	generic_glfw::operator bool() const
	{
		return mInitialized;
	}

	window* generic_glfw::prepare_window()
	{
		assert(are_we_on_the_main_thread());

		// Insert in the back and return the newly created window
		auto& back = mWindows.emplace_back(std::make_unique<window>());

		// Continue initialization later, after this window has gotten a handle:
		context().add_event_handler(context_state::anytime, // execute handler asap
			[wnd = back.get()]() -> bool {
				LOG_INFO("Running event handler which sets up windows focus callbacks");
				// We can't be sure whether it still exists
				auto* exists = context().find_window([wnd](const auto* w) -> bool {
					return wnd == w;
				});

				// but it does!
				if (wnd->handle().has_value()) {
					auto handle = wnd->handle()->mHandle;

					// finish initialization of this window
					glfwSetWindowFocusCallback(handle, glfw_window_focus_callback);
					if (nullptr == sWindowInFocus) {
						sWindowInFocus = wnd;
					}

					int width, height;
					glfwGetWindowSize(handle, &width, &height);
					wnd->mResultion = glm::uvec2(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
					glfwSetWindowSizeCallback(handle, glfw_window_size_callback);

					wnd->mIsCursorDisabled = glfwGetInputMode(handle, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

					return true; // done
				}
				return false; // not done yet
			}); 

		// Also add an event handler which will run at the end of the application for cleanup:
		context().add_event_handler(context_state::about_to_finalize,
			[wnd = back.get()]() -> bool {
				LOG_INFO("Running window cleanup event handler");
				// We can't be sure whether it still exists
				auto* exists = context().find_window([wnd](const auto* w) -> bool {
					return wnd == w;
				});

				if (nullptr == exists) { // Doesn't exist anymore
					return true;
				}

				// Okay, it exists => close it!
				context().close_window(*wnd);
			});
		
		return back.get();
	}

	void generic_glfw::close_window(window& wnd)
	{
		if (!wnd.handle())
		{
			LOG_WARNING("The passed window has no valid handle. Has it already been destroyed?");
			return;
		}

		if (wnd.is_in_use()) {
			throw new std::logic_error("This window is in use and can not be closed at the moment.");
		}

		context().dispatch_to_main_thread([&wnd]() {
			auto handle = wnd.handle()->mHandle;
			wnd.mHandle = std::nullopt;

			// unregister callbacks:
			glfwSetWindowFocusCallback(handle, nullptr);
			glfwSetWindowSizeCallback(handle, nullptr);

			context().mWindows.erase(
				std::remove_if(
					std::begin(context().mWindows), std::end(context().mWindows),
					[&wnd](const auto& inQuestion) -> bool {
						return inQuestion.get() == &wnd;
					}),
				std::end(context().mWindows));

			glfwDestroyWindow(handle);
		});
	}

	double generic_glfw::get_time()
	{
		assert(are_we_on_the_main_thread());
		return glfwGetTime();
	}

	void generic_glfw::glfw_error_callback(int error, const char* description)
	{
		LOG_ERROR(fmt::format("GLFW-Error: hex[0x{0:x}] int[{0}] description[{1}]", error, description));
	}

	void generic_glfw::start_receiving_input_from_window(const window& pWindow, input_buffer& pInputBuffer)
	{
		assert(are_we_on_the_main_thread());
		glfwSetMouseButtonCallback(pWindow.handle()->mHandle, glfw_mouse_button_callback);
		glfwSetCursorPosCallback(pWindow.handle()->mHandle, glfw_cursor_pos_callback);
		glfwSetScrollCallback(pWindow.handle()->mHandle, glfw_scroll_callback);
		glfwSetKeyCallback(pWindow.handle()->mHandle, glfw_key_callback);
	}

	void generic_glfw::stop_receiving_input_from_window(const window& pWindow)
	{
		assert(are_we_on_the_main_thread());
		glfwSetMouseButtonCallback(pWindow.handle()->mHandle, nullptr);
		glfwSetCursorPosCallback(pWindow.handle()->mHandle, nullptr);
		glfwSetScrollCallback(pWindow.handle()->mHandle, nullptr);
		glfwSetKeyCallback(pWindow.handle()->mHandle, nullptr);
	}

	window* generic_glfw::main_window() const
	{
		if (mWindows.size() > 0) {
			return mWindows[0].get();
		}
		return nullptr;
	}

	void generic_glfw::set_main_window(window* pMainWindowToBe) 
	{
		context().dispatch_to_main_thread([this, pMainWindowToBe]() {
			auto position = std::find_if(std::begin(mWindows), std::end(mWindows), [pMainWindowToBe](const window_ptr & w) -> bool {
				return w.get() == pMainWindowToBe;
				});

			if (position == mWindows.end()) {
				throw std::runtime_error(fmt::format("Window[{}] not found in collection of windows", fmt::ptr(pMainWindowToBe)));
			}

			std::rotate(std::begin(mWindows), position, position + 1); // Move ONE element to the beginning, not the rest of the vector => hence, not std::end(mWindows)
		});
	}

	window* generic_glfw::window_by_title(const std::string& pTitle) const
	{
		auto it = std::find_if(std::begin(mWindows), std::end(mWindows),
							   [&pTitle](const auto& w) {
								   return w->title() == pTitle;
							   });
		return it != mWindows.end() ? it->get() : nullptr;
	}

	window* generic_glfw::window_by_id(uint32_t pId) const
	{
		auto it = std::find_if(std::begin(mWindows), std::end(mWindows),
							   [&pId](const auto & w) {
								   return w->id() == pId;
							   });
		return it != mWindows.end() ? it->get() : nullptr;
	}

	void generic_glfw::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		assert(are_we_on_the_main_thread());
		std::scoped_lock<std::mutex> guard(sInputMutex);
		button = glm::clamp(button, 0, 7);

		auto& inputBackBuffer = composition_interface::current()->background_input_buffer();
		switch (action)
		{
		case GLFW_PRESS:
			inputBackBuffer.mMouseKeys[button] |= key_state::pressed;
			inputBackBuffer.mMouseKeys[button] |= key_state::down;
			break;
		case GLFW_RELEASE:
			inputBackBuffer.mMouseKeys[button] |= key_state::released;
			break;
		case GLFW_REPEAT:
			inputBackBuffer.mMouseKeys[button] |= key_state::down;
			break;
		}
	}

	void generic_glfw::glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
	{
		assert(are_we_on_the_main_thread());
		auto* wnd = context().window_for_handle(window);
		assert(wnd);
		wnd->mCursorPosition = glm::dvec2(xpos, ypos);
	}

	void generic_glfw::glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		//assert(sTargetInputBuffer);
		//std::scoped_lock<std::mutex> guard(sInputMutex);
		//sTargetInputBuffer->mScrollPosition += glm::dvec2(xoffset, yoffset);
	}

	void generic_glfw::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		std::scoped_lock<std::mutex> guard(sInputMutex);

		// TODO: Do something with the window-parameter? Or is it okay?

		key = glm::clamp(key, 0, GLFW_KEY_LAST);

		auto& inputBackBuffer = composition_interface::current()->background_input_buffer();
		switch (action)
		{
		case GLFW_PRESS:
			inputBackBuffer.mKeyboardKeys[static_cast<size_t>(sGlfwToKeyMapping[key])] |= key_state::pressed;
			inputBackBuffer.mKeyboardKeys[static_cast<size_t>(sGlfwToKeyMapping[key])] |= key_state::down;
			break;
		case GLFW_RELEASE:
			inputBackBuffer.mKeyboardKeys[static_cast<size_t>(sGlfwToKeyMapping[key])] |= key_state::released;
			inputBackBuffer.mKeyboardKeys[static_cast<size_t>(sGlfwToKeyMapping[key])] &= ~key_state::down;
			break;
		case GLFW_REPEAT:
			// just ignore
			break;
		}
	}

	void generic_glfw::glfw_window_focus_callback(GLFWwindow* window, int focused)
	{
		if (focused) {
			sWindowInFocus = context().window_for_handle(window);
		}
	}

	void generic_glfw::glfw_window_size_callback(GLFWwindow* window, int width, int height)
	{
		auto* wnd = context().window_for_handle(window);
		assert(wnd);
		wnd->mResultion = glm::uvec2(width, height);
	}

	GLFWwindow* generic_glfw::get_window_for_shared_context()
	{
		for (const auto& w : mWindows) {
			if (w->handle().has_value()) {
				return w->handle()->mHandle;
			}
		}
		return nullptr;
	}

	bool generic_glfw::are_we_on_the_main_thread()
	{
		return sMainThreadId == std::this_thread::get_id();
	}

	void generic_glfw::dispatch_to_main_thread(std::function<dispatcher_action> pAction)
	{
		// Are we on the main thread?
		if (are_we_on_the_main_thread()) {
			pAction();
		}
		else {
			std::scoped_lock<std::mutex> guard(sDispatchMutex);
			mDispatchQueue.push_back(std::move(pAction));
		}
	}

	void generic_glfw::work_off_all_pending_main_thread_actions()
	{
		assert(are_we_on_the_main_thread());
		std::scoped_lock<std::mutex> guard(sDispatchMutex);
		for (auto& action : mDispatchQueue) {
			action();
		}
		mDispatchQueue.clear();
	}

	void generic_glfw::add_event_handler(context_state pStage, event_handler_func pHandler)
	{
		dispatch_to_main_thread([handler = std::move(pHandler), stage = pStage]() {
			// No need to lock anything here, everything is happening on the main thread only
			context().mEventHandlers.emplace_back(std::move(handler), stage);

			context().work_off_event_handlers();
		});
	}

	void generic_glfw::work_off_event_handlers()
	{
		assert(are_we_on_the_main_thread());
		size_t numHandled = 0;
		do {
			// No need to lock anything here, everything is happening on the main thread only
			auto numBefore = mEventHandlers.size();
			mEventHandlers.erase(
				std::remove_if(
					std::begin(mEventHandlers), std::end(mEventHandlers),
					[curState = mContextState](const auto& tpl) {
				auto targetStates = std::get<cgb::context_state>(tpl);
				if ((curState & targetStates) != curState) {
					return false; // false => not done yet! Handler shall remain, because handler has not been invoked.
				}
				// Invoke the handler:
				return std::get<event_handler_func>(tpl)();	// true => done, i.e. remove
															// false => not done yet, i.e. shall remain
			}),
				std::end(mEventHandlers));
			auto numAfter = mEventHandlers.size();
			numHandled = numAfter - numBefore;
		} while (numHandled > 0);
	}
}
