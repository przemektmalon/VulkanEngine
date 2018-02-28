#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Keyboard.hpp"

#ifdef _WIN32
/*
	@brief	Window proc function for receiving Window messages (user input among others)
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif

/*
	@brief	Create a window surface for drawing to screen
*/
void Window::create(WindowCreateInfo * c)
{
#ifdef _WIN32
	memset(&win32WindowClass, 0, sizeof(WNDCLASSEX));
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

	windowName = c->title;

	DWORD windowStyle;
	if (c->borderless)
		windowStyle = WS_POPUP;
	else
		windowStyle = (WS_OVERLAPPED | WS_SYSMENU);

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

	vkCreateWin32SurfaceKHR(Engine::vkInstance, &sci, NULL, &vkSurface);
#endif
#ifdef __linux__
	int screenp = 0;
	connection = xcb_connect(NULL, &screenp);
	if (xcb_connection_has_error(connection))
		throw std::runtime_error("failed to connect to X server using XCB");

	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));

	for (int s = screenp; s > 0; s--)
		xcb_screen_next(&iter);
	
	screen = iter.data;
	window = xcb_generate_id(connection);
	uint32_t eventMask = XCB_CW_EVENT_MASK;
	uint32_t valueList[] = { 0 };
	xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, c->width, c->height,
						0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, eventMask, valueList);
	//Code to set window name
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(c->title), c->title);
	
	//Sets window destroy
	xcb_intern_atom_cookie_t wmDeleteCookie = xcb_intern_atom(connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	xcb_intern_atom_cookie_t wmProtocolsCookie = xcb_intern_atom(connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *wmDeleteReply = xcb_intern_atom_reply(connection, wmDeleteCookie, NULL);
	xcb_intern_atom_reply_t *wmProtocolsReply = xcb_intern_atom_reply(connection, wmProtocolsCookie, NULL);
	wmDeleteWin = wmDeleteReply->atom;
	wmProtocols = wmProtocolsReply->atom;

	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window,
						wmProtocolsReply->atom, 4, 32, 1, &wmDeleteReply->atom);
	
	
	xcb_map_window(connection, window);
	xcb_flush(connection);

	VkXcbSurfaceCreateInfoKHR sci = {};
	sci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	sci.pNext = NULL;
	sci.flags = 0;
	sci.connection = Window::connection;
	sci.window = Window::window;
	VkResult result = vkCreateXcbSurfaceKHR(Engine::vkInstance, &sci, NULL, &vkSurface);
#endif
}

/*
	@brief	Destroy the window
*/
void Window::destroy()
{
	vkDestroySurfaceKHR(Engine::vkInstance, vkSurface, 0);
#ifdef _WIN32
	UnregisterClass(windowName.c_str(), Engine::win32InstanceHandle);
#endif
}

/*
	@brief	Process window related messages. 
	@note	This function should detect user input (keyboard, mouse)
*/
bool Window::processMessages()
{
#ifdef _WIN32
	MSG msg;
	if (PeekMessage(&msg, win32WindowHandle, 0, 0, PM_REMOVE)) {
		//TranslateMessage(&msg);
		DispatchMessage(&msg);
		return true;
	}
	return false;
#endif
}

#ifdef _WIN32
/*
	@brief	Window proc function for receiving Window messages (user input among others)
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
	{
		Keyboard::keyState[wParam] = 1;
		auto check = [](u8 pKey) -> bool { return Keyboard::isKeyPressed(pKey); };
		Event newEvent(Event::KeyDown); 
		newEvent.constructKey(wParam, 
			check(Key::KC_RIGHT_SHIFT) || check(Key::KC_LEFT_SHIFT), 
			check(Key::KC_LEFT_ALT) || check(Key::KC_RIGHT_ALT), 
			check(Key::KC_RIGHT_SUPER) || check(Key::KC_LEFT_SUPER), 
			check(Key::KC_CAPS), 
			check(Key::KC_RIGHT_CTRL) || check(Key::KC_LEFT_CTRL));
		Engine::window->eventQ.pushEvent(newEvent);
		break;
	}
	case WM_KEYUP:
	{
		Keyboard::keyState[wParam] = 0;
		auto check = [](u8 pKey) -> bool { return Keyboard::isKeyPressed(pKey); };
		Event newEvent(Event::KeyUp);
		newEvent.constructKey(wParam,
			check(Key::KC_RIGHT_SHIFT) || check(Key::KC_LEFT_SHIFT),
			check(Key::KC_LEFT_ALT) || check(Key::KC_RIGHT_ALT),
			check(Key::KC_RIGHT_SUPER) || check(Key::KC_LEFT_SUPER),
			check(Key::KC_CAPS),
			check(Key::KC_RIGHT_CTRL) || check(Key::KC_LEFT_CTRL));
		Engine::window->eventQ.pushEvent(newEvent);
		break;
		break;
	}
	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		Engine::engineRunning = false;
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

#endif