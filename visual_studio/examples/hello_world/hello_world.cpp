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
};

int main()
{
	auto mainWnd = cgb::context().create_window();
	auto helloBehavior = std::make_shared<hello_behavior>();
	auto hello = cgb::composition<cgb::fixed_update_timer>({
			&mainWnd 
		}, {
			helloBehavior
		});
	hello.start();

#ifdef _DEBUG
	std::cout << std::endl << "Press any key to continue ..." << std::endl;
	std::cin.get();
#endif
}


