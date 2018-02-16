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

		KC_0 = 0x30,
		KC_1,
		KC_2,
		KC_3,
		KC_4,
		KC_5,
		KC_6,
		KC_7,
		KC_8,

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