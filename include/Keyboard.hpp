#pragma once
#include "PCH.hpp"

typedef s32 KeyCode;

/*
	@brief	Key object. Specified key codes (modelled after Win32 virtual key codes)
*/
class Key
{
public:
	Key(KeyCode pCode) : code(pCode) {}
	Key() : code(KC_NULL) {}
	enum
	{
		KC_NULL = 0,

	#ifdef _WIN32
		KC_0 = 0x30,
		KC_1,
		KC_2,
		KC_3,
		KC_4,
		KC_5,
		KC_6,
		KC_7,
		KC_8,
		KC_9,

		KC_A = 0x41,
		KC_B,
		KC_C,
		KC_D,
		KC_E,
		KC_F,
		KC_G,
		KC_H,
		KC_I,
		KC_J,
		KC_K,
		KC_L,
		KC_M,
		KC_N,
		KC_O,
		KC_P,
		KC_Q,
		KC_R,
		KC_S,
		KC_T,
		KC_U,
		KC_V,
		KC_W,
		KC_X,
		KC_Y,
		KC_Z,

		KC_NUMPAD0 = 0x60,
		KC_NUMPAD1,
		KC_NUMPAD2,
		KC_NUMPAD3,
		KC_NUMPAD4,
		KC_NUMPAD5,
		KC_NUMPAD6,
		KC_NUMPAD7,
		KC_NUMPAD8,
		KC_NUMPAD9,
		
		
		KC_BACKSPACE = VK_BACK,
		KC_TAB = VK_TAB,
		KC_DELETE = VK_DELETE,
		KC_ENTER = VK_RETURN,
		KC_LEFT = VK_LEFT,
		KC_RIGHT = VK_RIGHT,
		KC_UP = VK_UP,
		KC_DOWN = VK_DOWN,

		KC_LEFT_SUPER = VK_LWIN,
		KC_RIGHT_SUPER = VK_RWIN,
		KC_RETURN = VK_RETURN,
		KC_LEFT_SHIFT = VK_LSHIFT,
		KC_RIGHT_SHIFT = VK_RSHIFT,
		KC_LEFT_CTRL = VK_LCONTROL,
		KC_RIGHT_CTRL = VK_RCONTROL,
		KC_LEFT_ALT = VK_LMENU,
		KC_RIGHT_ALT = VK_RMENU,
		KC_CAPS = VK_CAPITAL,
		KC_ESCAPE = VK_ESCAPE
	#endif
	#ifdef __linux__
		KC_ESCAPE = 9,
		KC_1,
		KC_2,
		KC_3,
		KC_4,
		KC_5,
		KC_6,
		KC_7,
		KC_8,
		KC_9,
		KC_0,
		KC_MINUS,
		KC_EQUAL,
		KC_BACKSPACE,
		KC_TAB,
		KC_Q,
		KC_W,
		KC_E,
		KC_R,
		KC_T,
		KC_Y,
		KC_U,
		KC_I,
		KC_O,
		KC_P,
		KC_RIGHT_BRACKET,
		KC_LEFT_BRACKET,
		KC_ENTER,
		KC_LEFT_CTRL,
		KC_A,
		KC_S,
		KC_D,
		KC_F,
		KC_G,
		KC_H,
		KC_J,
		KC_K,
		KC_L,
		KC_SEMICOLON,
		KC_APOSTROPHE,
		KC_GRAVE,
		KC_LEFT_SHIFT,
		KC_BACK_SLASH,
		KC_Z,
		KC_X,
		KC_C,
		KC_V,
		KC_B,
		KC_N,
		KC_M,
		KC_COMMA,
		KC_PERIOD,
		KC_FORWARD_SLASH,
		KC_RIGHT_SHIFT,
		KC_ASTERIX,
		KC_LEFT_ALT,
		KC_SPACE,
		KC_CAPS,
		KC_F1,
		KC_F2,
		KC_F3,
		KC_F4,
		KC_F5,
		KC_F6,
		KC_F7,
		KC_F8,
		KC_F9,
		KC_F10,
		KC_NUMLOCK,
		KC_SCROLL_LOCK,
		KC_NUMPAD7,
		KC_NUMPAD8,
		KC_NUMPAD9,
		KC_NUMPAD_MINUS,
		KC_NUMPAD4,
		KC_NUMPAD5,
		KC_NUMPAD6,
		KC_NUMPAD_PLUS,
		KC_NUMPAD1,
		KC_NUMPAD2,
		KC_NUMPAD3,
		KC_NUMPAD0,
		KC_NUMPAD_DEL,

		KC_F11=95,
		KC_F12,

		KC_RETURN=104,
		KC_RIGHT_CTRL,
		KC_SECOND_FORWARD_SLASH,
		KC_PRINTSCREEN,
		KC_RIGHT_ALT,

		KC_HOME = 110,
		KC_UP,
		KC_PAGEUP,
		KC_LEFT,
		KC_RIGHT,
		KC_END,
		KC_DOWN,
		KC_PAGEDOWN,
		KC_INSERT,
		KC_DELETE,
		KC_PAUSE = 127,

		KC_LEFT_SUPER = 133,
		KC_RIGHT_SUPER
	#endif
	};
	KeyCode code;
};

/*
	@brief	Static keyboard class. Stores keyboard state.
*/
class Keyboard
{
public:
	Keyboard() {}
	~Keyboard() {}

	static bool isKeyPressed(Key key)
	{
		return keyState[key.code] != 0;
	}

	static u8 keyState[256];
};