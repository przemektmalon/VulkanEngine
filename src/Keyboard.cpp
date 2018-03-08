#include "PCH.hpp"
#include "Keyboard.hpp"

u8 Keyboard::keyState[256];

#ifdef __linux__
s32 Key::linuxScanCodeToInternal(s32 scan)
{
	switch (scan)
	{
	case 9: return KC_ESCAPE;
	case 10: return KC_1;
	case 11: return KC_2;
	case 12: return KC_3;
	case 13: return KC_4;
	case 14: return KC_5;
	case 15: return KC_6;
	case 16: return KC_7;
	case 17: return KC_8;
	case 18: return KC_9;
	case 19: return KC_0;
	case 20: return KC_MINUS;
	case 21: return KC_PLUS;
	case 22: return KC_BACKSPACE;
	case 23: return KC_TAB;
	case 24: return KC_Q;
	case 25: return KC_W;
	case 26: return KC_E;
	case 27: return KC_R;
	case 28: return KC_T;
	case 29: return KC_Y;
	case 30: return KC_U;
	case 31: return KC_I;
	case 32: return KC_O;
	case 33: return KC_P;
	case 34: return KC_RIGHT_BRACKET;
	case 35: return KC_LEFT_BRACKET;
	case 36: return KC_ENTER;
	case 37: return KC_LEFT_CTRL;
	case 38: return KC_A;
	case 39: return KC_S;
	case 40: return KC_D;
	case 41: return KC_F;
	case 42: return KC_G;
	case 43: return KC_H;
	case 44: return KC_J;
	case 45: return KC_K;
	case 46: return KC_L;
	case 47: return KC_SEMICOLON;
	case 48: return KC_APOSTROPHE;
	case 49: return KC_GRAVE;
	case 50: return KC_LEFT_SHIFT;
	case 51: return KC_BACK_SLASH;
	case 52: return KC_Z;
	case 53: return KC_X;
	case 54: return KC_C;
	case 55: return KC_V;
	case 56: return KC_B;
	case 57: return KC_N;
	case 58: return KC_M;
	case 59: return KC_COMMA;
	case 60: return KC_PERIOD;
	case 61: return KC_FORWARD_SLASH;
	case 62: return KC_RIGHT_SHIFT;
	case 63: return KC_ASTERISK;
	case 64: return KC_LEFT_ALT;
	case 65: return KC_SPACE;
	case 66: return KC_CAPS;
	case 67: return KC_F1;
	case 68: return KC_F2;
	case 69: return KC_F3;
	case 70: return KC_F4;
	case 71: return KC_F5;
	case 72: return KC_F6;
	case 73: return KC_F7;
	case 74: return KC_F8;
	case 75: return KC_F9;
	case 76: return KC_F10;
	case 77: return KC_NUMLOCK;
	case 78: return KC_SCROLL_LOCK;
	case 79: return KC_NUMPAD7;
	case 80: return KC_NUMPAD8;
	case 81: return KC_NUMPAD9;
	case 82: return KC_MINUS;
	case 83: return KC_NUMPAD4;
	case 84: return KC_NUMPAD5;
	case 85: return KC_NUMPAD6;
	case 86: return KC_PLUS;
	case 87: return KC_NUMPAD1;
	case 88: return KC_NUMPAD2;
	case 89: return KC_NUMPAD3;
	case 90: return KC_NUMPAD0;
	case 91: return KC_NUMPAD_PERIOD;
	case 95: return KC_F11;
	case 96: return KC_F12;
	case 104: return KC_RETURN;
	case 105: return KC_RIGHT_CTRL;
	case 106: return KC_DIVIDE;
	case 107: return KC_PRINTSCREEN;
	case 108: return KC_RIGHT_ALT;
	case 110: return KC_HOME;
	case 111: return KC_UP;
	case 112: return KC_PAGEUP;
	case 113: return KC_LEFT;
	case 114: return KC_RIGHT;
	case 115: return KC_END;
	case 116: return KC_DOWN;
	case 117: return KC_PAGEDOWN;
	case 118: return KC_INSERT;
	case 119: return KC_DELETE;
	case 127: return KC_PAUSE;
	case 133: return KC_LEFT_SUPER;
	case 134: return KC_RIGHT_SUPER;
	default: return KC_UNKNOWN;
	}
}
#endif
