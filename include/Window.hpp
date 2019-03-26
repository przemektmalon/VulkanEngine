#pragma once
#include "PCH.hpp"
#include "Event.hpp"

namespace os {

/*
	@brief	Data required to create a window
*/
struct WindowCreateInfo
{
	WindowCreateInfo() : posX(0), posY(0), borderless(true) {}
	int width;
	int height;
	const char* title;

	int posX;
	int posY;
	bool borderless; // _WIN32 ?
};

/*
	@brief	On screen window and its data. Owns Vulkan surface object. Owns event queue.
*/
class Window
{
public:
	Window() {}
	~Window() {}

	void create(WindowCreateInfo* c);
	void resize(u32 newResX, u32 newResY);
	void destroy();
	// Returns true if all messages have been processed
	void processMessages();
#ifdef __linux__
	bool isWmDeleteWin(xcb_atom_t message);
#endif
	Event constructKeyEvent(u8 keyCode, Event::Type eventType);

#ifdef _WIN32
	void registerRawMouseDevice();
#endif

	VkSurfaceKHR vkSurface;
	u32 resX;
	u32 resY;
	u32 posX;
	u32 posY;
	std::string windowName;
	EventQ eventQ;
#ifdef _WIN32
	HINSTANCE win32InstanceHandle;	// Instance handle for this .exe
	WNDCLASSEX win32WindowClass;	// Window class describing WndProc, icon, cursor, etc
	HWND win32WindowHandle;			// Window handle
#endif
#ifdef __linux__
	xcb_connection_t * connection;
	xcb_window_t window;
	xcb_screen_t * screen;
	xcb_atom_t wmProtocols;
	xcb_atom_t wmDeleteWin;
#endif
};

}