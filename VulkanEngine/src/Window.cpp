#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"

#ifdef _WIN32

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif

void Window::create(WindowCreateInfo * c)
{
#ifdef _WIN32
	win32WindowClass.cbSize = sizeof(WNDCLASSEX);
	win32WindowClass.lpfnWndProc = WndProc;
	win32WindowClass.lpszClassName = LPCSTR(c->title);
	win32WindowClass.hInstance = c->win32InstanceHandle;
	win32WindowClass.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	win32WindowClass.style = CS_OWNDC;
	win32WindowClass.cbClsExtra = 0;
	win32WindowClass.cbWndExtra = 0;

	if (!RegisterClassEx(&win32WindowClass))
		DBG_SEVERE("Could not register win32 window class");

	DWORD windowStyle;
	if (c->borderless)
		windowStyle = WS_POPUP;
	else
		windowStyle = (WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	win32WindowHandle = CreateWindowEx(0, win32WindowClass.lpszClassName, win32WindowClass.lpszClassName, windowStyle,
										c->posX, c->posY, c->width, c->height, 0, 0, c->win32InstanceHandle, 0);

	if (!win32WindowHandle)
		DBG_SEVERE("Could not create win32 window");

	ShowWindow(win32WindowHandle, 1);

	VkWin32SurfaceCreateInfoKHR sci;
	sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	sci.pNext = 0;
	sci.flags = 0;
	sci.hinstance = Engine::win32InstanceHandle;
	sci.hwnd = win32WindowHandle;

	vkCreateWin32SurfaceKHR(Engine::vkInstance, &sci, 0, &vkSurface);
#endif
}

void Window::destroy()
{
	vkDestroySurfaceKHR(Engine::vkInstance, vkSurface, 0);
}

bool Window::processMessages()
{
#ifdef _WIN32
	MSG msg;
	if (PeekMessage(&msg, win32WindowHandle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return true;
	}
	return false;
#endif
}

#ifdef _WIN32

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
	{
		Engine::engineRunning = false;
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

#endif