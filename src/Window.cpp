#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Keyboard.hpp"
#include "Profiler.hpp"

#ifdef _WIN32
/*
	@brief	Window proc function for receiving Window messages (user input among others)
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
#ifdef __linux__
void processEvent(xcb_generic_event_t *event);
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

	VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(Engine::vkInstance, &sci, NULL, &vkSurface));

	registerRawMouseDevice();
#endif
#ifdef __linux__
	connection = c->connection;
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
	
	screen = iter.data;
	window = xcb_generate_id(connection);
 	uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  	uint32_t valueList[] = {screen->black_pixel, 	XCB_EVENT_MASK_EXPOSURE       | XCB_EVENT_MASK_BUTTON_PRESS   |
													XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
													XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW   |
													XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE};
													
	xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, c->width, c->height,
						0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, eventMask, valueList);
	
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
	VK_CHECK_RESULT(vkCreateXcbSurfaceKHR(Engine::vkInstance, &sci, NULL, &vkSurface));
#endif

	resX = c->width;
	resY = c->height;
}

void Window::resize(u32 newResX, u32 newResY)
{
	resX = newResX;
	resY = newResY;
#ifdef _WIN32
	SetWindowPos(win32WindowHandle, 0, 0, 0, newResX, newResY, SWP_NOOWNERZORDER | SWP_NOZORDER);
#endif
	Event we(Event::Type::WindowResized); // Window event
	we.constructWindow(glm::ivec2(posX, posY), glm::ivec2(resX, resY));
	eventQ.pushEvent(we);
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
#ifdef __linux__
	xcb_destroy_window(connection, window);
#endif
}

/*
	@brief	Process window related messages. 
	@note	This function should detect user input (keyboard, mouse)
*/
void Window::processMessages()
{
#ifdef _WIN32
	MSG msg;
	while (PeekMessage(&msg, win32WindowHandle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
#ifdef __linux__

	xcb_generic_event_t *event;
	while( event = xcb_poll_for_event(connection)){
		processEvent(event);
		free(event);
	}
#endif
}

Event Window::constructKeyEvent(u8 keyCode, Event::Type eventType)
{
	auto check = [](u8 pKey) -> bool { return Keyboard::isKeyPressed((Key)pKey); };
	Event newEvent(eventType);
	newEvent.constructKey(keyCode,
		check(Key::KC_RIGHT_SHIFT) || check(Key::KC_LEFT_SHIFT), 
		check(Key::KC_LEFT_ALT) || check(Key::KC_RIGHT_ALT), 
		check(Key::KC_RIGHT_SUPER) || check(Key::KC_LEFT_SUPER), 
		check(Key::KC_CAPS), 
		check(Key::KC_RIGHT_CTRL) || check(Key::KC_LEFT_CTRL));

	return newEvent;
}

#ifdef _WIN32
void Window::registerRawMouseDevice()
{
	RAWINPUTDEVICE Rid[1];

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = 0;
	Rid[0].hwndTarget = 0;

	if (!RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]))) {
		DBG_SEVERE("Could not register raw/hardware mouse");
	}
}
#endif

#ifdef __linux__
void processEvent(xcb_generic_event_t *event){

	switch (event->response_type & ~0x80) {
		
		case XCB_CLIENT_MESSAGE: {
			if (Engine::window->isWmDeleteWin(((xcb_client_message_event_t *)event)->data.data32[0]))
				Engine::engineRunning = false;
			break;
		}
		
		case XCB_BUTTON_PRESS: {
			xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
			switch (bp->detail) {
			case 1:
				Mouse::state |= Mouse::M_LEFT;
				break;
			case 2:
				Mouse::state |= Mouse::M_MIDDLE;
				break;
			case 3:
				Mouse::state |= Mouse::M_RIGHT;
				break;
			case 4:
				//DBG_INFO("D Scroll Up");
				break;
			case 5:
				//DBG_INFO("D Scroll Down");
				break;
			default:
				//Button pressed in window
				break;
			}
			Event mouseEvent(Event::MouseDown);
			glm::ivec2 mPos(bp->event_x, bp->event_y);
			mouseEvent.constructMouse(Mouse::state, mPos, glm::ivec2(0), 0);
			Engine::window->eventQ.pushEvent(mouseEvent);
			break;
		}
		case XCB_BUTTON_RELEASE: {
			//Button released in window
			xcb_button_release_event_t *br = (xcb_button_release_event_t *)event;
			switch (br->detail) {
			case 1:
				Mouse::state &= ~Mouse::M_LEFT;
				break;
			case 2:
				Mouse::state &= ~Mouse::M_MIDDLE;
				break;
			case 3:
				Mouse::state &= ~Mouse::M_RIGHT;
				break;
			case 4:
				//DBG_INFO("U Scroll Up");
				break;
			case 5:
				//DBG_INFO("U Scroll Down");
				break;
			default:
				//Button pressed in window
				break;
			}
			Event mouseEvent(Event::MouseUp);
			glm::ivec2 mPos(br->event_x, br->event_y);
			mouseEvent.constructMouse(Mouse::state, mPos, glm::ivec2(0), 0);
			Engine::window->eventQ.pushEvent(mouseEvent);
			break;
		}
		case XCB_MOTION_NOTIFY: {
			//Mouse moved in window
			xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
			glm::ivec2 mPos(motion->event_x, motion->event_y);
			glm::ivec2 move = mPos-Mouse::getWindowPosition();
			
			Event mouseEvent(Event::MouseMove);
			mouseEvent.constructMouse(Mouse::state, mPos, move, 0);
			Engine::window->eventQ.pushEvent(mouseEvent);
			Mouse::setWindowPosition(mPos);
			break;
		}
		case XCB_ENTER_NOTIFY: {
			//Mouse entered window
			xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)event;

			break;
		}
		case XCB_LEAVE_NOTIFY: {
			//Mouse left window
			xcb_leave_notify_event_t *leave = (xcb_leave_notify_event_t *)event;

			break;
		}
		case XCB_KEY_PRESS: {
			DBG_INFO("Key pressed");
			
			//Key pressed in window
			xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
			int internalKeyCode = Key::linuxScanCodeToInternal(kp->detail);
			Keyboard::keyState[internalKeyCode] = 1;
			Event keyEvent = Engine::window->constructKeyEvent(internalKeyCode, Event::KeyDown);
			Engine::window->eventQ.pushEvent(keyEvent);

			break;
		}
		case XCB_KEY_RELEASE: {
			DBG_INFO("Key released");
			//Key released in window
			xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
			int internalKeyCode = Key::linuxScanCodeToInternal(kr->detail);
			Keyboard::keyState[internalKeyCode] = 0;
			Event keyEvent = Engine::window->constructKeyEvent(internalKeyCode, Event::KeyUp);
			Engine::window->eventQ.pushEvent(keyEvent);
			break;
		}
		default:
			break;
		}
}


#endif
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
		Event keyEvent = Engine::window->constructKeyEvent(wParam, Event::KeyDown);
		Engine::window->eventQ.pushEvent(keyEvent);

		Event textEvent(Event::TextInput);
		textEvent.constructText(keyEvent.eventUnion.keyEvent.key, keyEvent.eventUnion.keyEvent.shift);
		if (textEvent.eventUnion.textInputEvent.character != 0)
			Engine::window->eventQ.pushEvent(textEvent);

		
		break;
	}
	case WM_KEYUP:
	{
		Event keyEvent = Engine::window->constructKeyEvent(wParam, Event::KeyUp);
		Engine::window->eventQ.pushEvent(keyEvent);
		break;
	}
	case WM_INPUT:
	{
		
		UINT dwSize = 0;

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

		//LPBYTE lpb = new BYTE[dwSize];
		LPBYTE lpb[48];

		/// WATCH: 48 hard-coded. Is it always 48 ?
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)lpb;

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			const auto buttonFlag = raw->data.mouse.usButtonFlags;

			if (buttonFlag == RI_MOUSE_WHEEL)
			{
				Event::Type eventType = Event::MouseWheel;
				Event mouseEvent(eventType);
				mouseEvent.constructMouse(0, glm::ivec2(Mouse::getWindowPosition(Engine::window)), glm::ivec2(0, 0), raw->data.mouse.usButtonData);
				Engine::window->eventQ.pushEvent(mouseEvent);
				//delete[] lpb;
				//PROFILE_END("msgevent");
				break;
			}

			const auto anyMouseClick = (RI_MOUSE_LEFT_BUTTON_DOWN | RI_MOUSE_RIGHT_BUTTON_DOWN | RI_MOUSE_MIDDLE_BUTTON_DOWN) & buttonFlag;
			const auto lMouseClick = RI_MOUSE_LEFT_BUTTON_DOWN & buttonFlag;
			const auto rMouseClick = RI_MOUSE_RIGHT_BUTTON_DOWN & buttonFlag;
			const auto mMouseClick = RI_MOUSE_MIDDLE_BUTTON_DOWN & buttonFlag;

			const auto anyMouseRelease = (RI_MOUSE_LEFT_BUTTON_UP | RI_MOUSE_RIGHT_BUTTON_UP | RI_MOUSE_MIDDLE_BUTTON_UP) & buttonFlag;
			const auto lMouseRelease = RI_MOUSE_LEFT_BUTTON_UP & buttonFlag;
			const auto rMouseRelease = RI_MOUSE_RIGHT_BUTTON_UP & buttonFlag;
			const auto mMouseRelease = RI_MOUSE_MIDDLE_BUTTON_UP & buttonFlag;

			if (true) /// TODO: Window has focus ?
			{
				glm::ivec2 mPos; mPos = Mouse::getWindowPosition(Engine::window);

				if (lMouseClick)
					Mouse::state |= Mouse::M_LEFT;
				else if (lMouseRelease)
					Mouse::state &= ~Mouse::M_LEFT;

				if (rMouseClick)
					Mouse::state |= Mouse::M_RIGHT;
				else if (rMouseRelease)
					Mouse::state &= ~Mouse::M_RIGHT;

				if (mMouseClick)
					Mouse::state |= Mouse::M_MIDDLE;
				else if (mMouseRelease)
					Mouse::state &= ~Mouse::M_MIDDLE;

				MouseCode mouseCode = Mouse::state;

				Event::Type eventType;

				if (anyMouseClick)
				{
					eventType = Event::MouseDown;
					Event mouseEvent(eventType);
					if (lMouseClick)
						mouseEvent.constructMouse(Mouse::M_LEFT, mPos, glm::ivec2(0), 0);
					else if (rMouseClick)
						mouseEvent.constructMouse(Mouse::M_RIGHT, mPos, glm::ivec2(0), 0);
					else
						mouseEvent.constructMouse(Mouse::M_MIDDLE, mPos, glm::ivec2(0), 0);
					Engine::window->eventQ.pushEvent(mouseEvent);		// Send mouse down event
				}
				else if (anyMouseRelease)
				{
					eventType = Event::MouseUp;
					Event mouseEvent(eventType);
					if (lMouseRelease)
						mouseEvent.constructMouse(Mouse::M_LEFT, mPos, glm::ivec2(0), 0);
					else if (rMouseRelease)
						mouseEvent.constructMouse(Mouse::M_RIGHT, mPos, glm::ivec2(0), 0);
					else
						mouseEvent.constructMouse(Mouse::M_MIDDLE, mPos, glm::ivec2(0), 0);
					Engine::window->eventQ.pushEvent(mouseEvent);		// Send mouse up event
				}

				glm::ivec2 mouseMove;
				mouseMove.x = raw->data.mouse.lLastX;
				mouseMove.y = raw->data.mouse.lLastY;

				if (mouseMove != glm::ivec2(0, 0))
				{
					Event moveEvent(Event::MouseMove);
					moveEvent.constructMouse(mouseCode, mPos, mouseMove, 0);
					Engine::window->eventQ.pushEvent(moveEvent);		// Send mouse move event
				}
			}
			else
			{
				//if (anyMouseClick || engineWindow->isMouseInClientArea())
				//	engineWindow->captureMouseFocus();
			}
		}

		//delete[] lpb;
		//PROFILE_END("msgevent");
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
		auto defRet = DefWindowProc(hWnd, message, wParam, lParam);
		return defRet;
	}
	return 0;
}

#endif

#ifdef __linux__
bool Window::isWmDeleteWin(xcb_atom_t message){
	return wmDeleteWin == message;
}
#endif
