#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"

void Engine::start()
{
#ifdef _WIN32
	win32InstanceHandle = GetModuleHandle(NULL);
#endif

	createWindow();

	while (true)
	{
		if (!window->processMessages())
		{

		}
	}

	return;
}

void Engine::createWindow()
{
	WindowCreateInfo wci;
#ifdef _WIN32
	wci.win32InstanceHandle = win32InstanceHandle;
#endif
	wci.height = 720;
	wci.width = 1280;
	wci.posX = 100;
	wci.posY = 100;
	wci.title = "Vulkan Engine";
	wci.borderless = false;

	window = new Window;
	window->create(&wci);
}

void Engine::quit()
{
}

#ifdef _WIN32
HINSTANCE Engine::win32InstanceHandle;
#endif
Window* Engine::window;