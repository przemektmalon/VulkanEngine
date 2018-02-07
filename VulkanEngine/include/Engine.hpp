#pragma once
#include "PCH.hpp"

class Window;

/*

	App launch.
	! Construct win32 window.
	! Construct vulkan instance, devices, surfaces etc

*/

class Engine
{
public:
	Engine() {}
	~Engine() {}

#ifdef _WIN32
	static HINSTANCE win32InstanceHandle;
#endif
	static Window* window;

	static void start();
	static void createWindow();
	static void quit();



};