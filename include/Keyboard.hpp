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
	Key() : code(KC_UNKNOWN) {}
#ifdef __linux__
	static KeyCode linuxScanCodeToInternal(s32 scan);
#endif
	enum
	{
		KC_UNKNOWN = 0,

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


		KC_BACKSPACE = 0x08,
		KC_TAB = 0x09,
		KC_DELETE = 0x2E,
		KC_ENTER = 0x0D,
		KC_LEFT = 0x25,
		KC_RIGHT = 0x27,
		KC_UP = 0x26,
		KC_DOWN = 0x28,

		KC_LEFT_SUPER = 0x5B,
		KC_RIGHT_SUPER = 0x5C,
		KC_RETURN = 0x0D,
		KC_LEFT_SHIFT = 0xA0,
		KC_RIGHT_SHIFT = 0xA1,
		KC_LEFT_CTRL = 0xA2,
		KC_RIGHT_CTRL = 0xA3,
		KC_LEFT_ALT = 0xA4,
		KC_RIGHT_ALT = 0xA5,
		KC_CAPS = 0x14,
		KC_ESCAPE = 0x1B,
		KC_MINUS = 0xBD,
		KC_PLUS = 0xBB,
		KC_LEFT_BRACKET = 0xDB,
		KC_RIGHT_BRACKET = 0xDD,
		KC_SEMICOLON = 0xBA,
		KC_APOSTROPHE = 0xDE,
		KC_GRAVE = 0xC0,
		KC_BACK_SLASH = 0xDC,
		KC_COMMA = 0xBC,
		KC_PERIOD = 0xBE,
		KC_ASTERISK = 0x6A,
		KC_FORWARD_SLASH = 0xBF,
		KC_SPACE = 0x20,
		KC_TILDE = 0xDF,

		KC_F1 = 0x70,
		KC_F2,
		KC_F3,
		KC_F4,
		KC_F5,
		KC_F6,
		KC_F7,
		KC_F8,
		KC_F9,
		KC_F10,
		KC_F11,
		KC_F12,
		KC_NUMLOCK = 0x90,
		KC_SCROLL_LOCK = 0x91,
		KC_DIVIDE = 0x6F,
		KC_NUMPAD_PERIOD = 0x6B,
		KC_PRINTSCREEN = 0x2C,
		KC_HOME = 0x24,
		KC_PAGEUP = 0x21,
		KC_PAGEDOWN = 0x22,
		KC_END = 0x23,
		KC_INSERT = 0x2D,
		KC_PAUSE = 0x13
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
#ifdef _WIN32
		return keyState[key.code] & 0b10000000;
#endif
		return keyState[key.code] != 0;
	}

	static bool isKeyPressed(char key)
	{
#ifdef _WIN32
		return keyState[key] & 0b10000000;
#endif
		return keyState[key] != 0;
	}

	static u8 keyState[256];
};