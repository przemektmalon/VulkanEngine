#pragma once
#include "PCH.hpp"

struct WindowCreateInfo
{
	WindowCreateInfo() : posX(0), posY(0), borderless(true) {}
#ifdef _WIN32
	HINSTANCE win32InstanceHandle;
#endif
	int width;
	int height;
	const char* title;

	int posX;
	int posY;
	bool borderless; // _WIN32 ?
};

class Window
{
public:
	Window() {}
	~Window() {}

	void create(WindowCreateInfo* c);
	void destroy();

	// Returns true if all messages have been processed
	bool processMessages();

	VkSurfaceKHR vkSurface;
#ifdef _WIN32
	HINSTANCE win32InstanceHandle;	// Instance handle for this .exe
	WNDCLASSEX win32WindowClass;	// Window class describing WndProc, icon, cursor, etc
	HWND win32WindowHandle;			// Window handle
	
#endif

};