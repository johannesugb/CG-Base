// hello_world.cpp : Defines the entry point for the console application.
//
#include "cg_base.h"

class hello_behavior : public cgb::cg_object
{
	void update() override
	{
		if (cgb::input().key_down(cgb::key_code::a))
			LOG_INFO("a pressed");
		if (cgb::input().key_down(cgb::key_code::s))
			LOG_INFO("s pressed");
		if (cgb::input().key_down(cgb::key_code::w))
			LOG_INFO("w pressed");
		if (cgb::input().key_down(cgb::key_code::d))
			LOG_INFO("d pressed");
		if (cgb::input().key_down(cgb::key_code::escape))
			cgb::current_composition().stop();
	}

	void render_gui() override
	{
		// TODO: Use IMGUI to draw something!
	}

};

int main()
{
	// Create a window which we're going to use to render to
	auto mainWnd = cgb::context().create_window();

	// Create a "behavior" which contains functionality of our program
	auto helloBehavior = hello_behavior();

	// Create a composition of all things that define the essence of 
	// our program, which there are:
	//  - a timer
	//  - a window
	//  - a behavior
	auto hello = cgb::composition<cgb::fixed_update_timer>({
			&mainWnd 
		}, {
			&helloBehavior
		});

	// Let's go:
	hello.start();

	// GG
#ifdef _DEBUG
	std::cout << std::endl << "Press any key to continue ..." << std::endl;
	std::cin.get();
#endif
}


