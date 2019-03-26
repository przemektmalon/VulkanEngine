#pragma once
#include "PCH.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"

/*
	@brief	Event superclass. Unions all types of events and their metadata.
*/
struct Event
{
	enum Type { MouseMove, MouseDown, MouseUp, MouseWheel, KeyDown, KeyUp, WindowResized, WindowMoved, TextInput };
	Type type;

	Event() {}
	Event(Type pType) : type(pType) {}

	union EventUnion{
		EventUnion(){}
		
		struct key_struct {
			os::Key key;
			bool shift;
			bool alt;
			bool sys;
			bool ctrl;
			bool caps;
		};
		key_struct keyEvent;
		struct text_struct {
			char character;
		};
		text_struct textInputEvent;
		struct mouse_struct {
			os::MouseCode code;
			glm::ivec2 position;
			glm::ivec2 move;
			s16 wheelDelta;
		};
		mouse_struct mouseEvent;
		struct window_struct {
			glm::ivec2 position;
			glm::ivec2 resolution;
		};
		window_struct windowEvent;
	} eventUnion;

	EventUnion::key_struct getKeyEvent() { return eventUnion.keyEvent; }
	EventUnion::mouse_struct getMouseEvent() { return eventUnion.mouseEvent; }
	EventUnion::text_struct getTextEvent() { return eventUnion.textInputEvent; }
	EventUnion::window_struct getWindowEvent() { return eventUnion.windowEvent; }

	void constructKey(os::Key pKey, bool pShift, bool pAlt, bool pSys, bool pCtrl, bool pCaps)
	{
		eventUnion.keyEvent.key = pKey; eventUnion.keyEvent.shift = pShift; eventUnion.keyEvent.alt = pAlt; eventUnion.keyEvent.sys = pSys; eventUnion.keyEvent.ctrl = pCtrl; eventUnion.keyEvent.caps = pCaps;
	}

	void constructMouse(os::MouseCode pCode, glm::ivec2 pPos, glm::ivec2 pMove, s16 pDelta)
	{
		eventUnion.mouseEvent.code = pCode; eventUnion.mouseEvent.position = pPos; eventUnion.mouseEvent.move = pMove; eventUnion.mouseEvent.wheelDelta = pDelta;
	}

	void constructText(os::Key pKey, bool pShift)
	{
		unsigned char c = pKey.code;
		if (c == os::Key::KC_BACKSPACE || c == os::Key::KC_ENTER)
		{
			eventUnion.textInputEvent.character = c;
			return;
		}
		else if (c == os::Key::KC_DELETE)
		{
			eventUnion.textInputEvent.character = 127;
			return;
		}
		else if (c >= os::Key::KC_A && c <= os::Key::KC_Z)
		{
			if (!pShift)
				c += 32;
			eventUnion.textInputEvent.character = c;
			return;
		}
		else if (c == os::Key::KC_SPACE)
		{
			eventUnion.textInputEvent.character = 32;
			return;
		}
		else if ((c >= os::Key::KC_0 && c <= os::Key::KC_9) ||
			c == os::Key::KC_MINUS ||
			c == os::Key::KC_PLUS ||
			c == os::Key::KC_LEFT_BRACKET ||
			c == os::Key::KC_RIGHT_BRACKET ||
			c == os::Key::KC_SEMICOLON ||
			c == os::Key::KC_APOSTROPHE ||
			c == os::Key::KC_BACK_SLASH ||
			c == os::Key::KC_COMMA ||
			c == os::Key::KC_PERIOD ||
			c == os::Key::KC_FORWARD_SLASH ||
			c == os::Key::KC_GRAVE)
		{
			if (pShift)
			{
				switch (c)
				{
				case '1':
					c = '!';
					break;
				case '2':
					c = '@';
					break;
				case '3':
					c = '#';
					break;
				case '4':
					c = '$';
					break;
				case '5':
					c = '%';
					break;
				case '6':
					c = '^';
					break;
				case '7':
					c = '&';
					break;
				case '8':
					c = '*';
					break;
				case '9':
					c = '(';
					break;
				case '0':
					c = ')';
					break;
				case os::Key::KC_MINUS:
					c = '_';
					break;
				case os::Key::KC_PLUS:
					c = '+';
					break;
				case os::Key::KC_LEFT_BRACKET:
					c = '{';
					break;
				case os::Key::KC_RIGHT_BRACKET:
					c = '}';
					break;
				case os::Key::KC_SEMICOLON:
					c = ':';
					break;
				case os::Key::KC_APOSTROPHE:
					c = '"';
					break;
				case os::Key::KC_FORWARD_SLASH:
					c = '|';
					break;
				case os::Key::KC_COMMA:
					c = '<';
					break;
				case os::Key::KC_PERIOD:
					c = '>';
					break;
				case os::Key::KC_BACK_SLASH:
					c = '?';
					break;
				case os::Key::KC_GRAVE:
					c = '~';
					break;
				}
			}
			else
			{
				switch (c) {
				case os::Key::KC_MINUS:
					c = '-';
					break;
				case os::Key::KC_PLUS:
					c = '=';
					break;
				case os::Key::KC_LEFT_BRACKET:
					c = '[';
					break;
				case os::Key::KC_RIGHT_BRACKET:
					c = ']';
					break;
				case os::Key::KC_SEMICOLON:
					c = ';';
					break;
				case os::Key::KC_APOSTROPHE:
					c = '\'';
					break;
				case os::Key::KC_FORWARD_SLASH:
					c = '/';
					break;
				case os::Key::KC_COMMA:
					c = ',';
					break;
				case os::Key::KC_PERIOD:
					c = '.';
					break;
				case os::Key::KC_BACK_SLASH:
					c = '\\';
					break;
				case os::Key::KC_GRAVE:
					c = '`';
					break;
				}
			}
			eventUnion.textInputEvent.character = c;
			return;
		}
		eventUnion.textInputEvent.character = 0;
	}

	void constructWindow(glm::ivec2 pPosition, glm::ivec2 pResolution)
	{
		eventUnion.windowEvent.position = pPosition;
		eventUnion.windowEvent.resolution = pResolution;
	}
};

/*
	@brief	Event queue. Used for pushing and polling events (user input)
*/
class EventQ
{
private:
	std::queue<Event> events;

public:
	bool pollEvent(Event& pEvent)
	{
		if (events.size() > 0)
		{
			pEvent = events.front();
			return true;
		} else
		{
			return false;
		}
	}

	bool popEvent(Event& pEvent)
	{
		if (events.size() > 0)
		{
			pEvent = events.front();
			events.pop();
			return true;
		}
		else
		{
			return false;
		}
	}

	bool popEvent()
	{
		if (events.size() > 0)
		{
			events.pop();
			return true;
		}
		else
		{
			return false;
		}
	}

	void pushEvent(Event& pEvent)
	{
		pushEventMutex.lock();
		events.push(pEvent);
		pushEventMutex.unlock();
	}

	std::mutex pushEventMutex;
};

/*
	@brief	Specialised key event class. Can be passed to functions expecting a key event.
*/
class KeyEvent : private Event
{
public:
	os::Key getKey() { return eventUnion.keyEvent.key; }
	bool getShift() { return eventUnion.keyEvent.shift; }
	bool getAlt() { return eventUnion.keyEvent.alt; }
	bool getSys() { return eventUnion.keyEvent.sys; }
	bool getCtrl() { return eventUnion.keyEvent.ctrl; }
	bool getCaps() { return eventUnion.keyEvent.caps; }
};

/*
	@brief	Specialised mouse event class. Can be passed to functions expecting a mouse event.
*/
class MouseEvent : private Event
{
public:
	os::MouseCode getCode() { return eventUnion.mouseEvent.code; }
	glm::ivec2 getPosition() { return eventUnion.mouseEvent.position; }
	glm::fvec2 getMove() { return eventUnion.mouseEvent.move; }
	int getDelta() { return eventUnion.mouseEvent.wheelDelta; }
};

/*
	@brief	Specialised text input event class. Can be passed to functions expecting text input.
*/
class TextInputEvent : private Event
{
public:
	char getCharacter() { return eventUnion.textInputEvent.character; }
};

/*
	@brief  Specialised window event class.
*/
class WindowEvent : private Event
{
public:
	glm::ivec2 getPosition() { return eventUnion.windowEvent.position; }
	glm::ivec2 getResolution() { return eventUnion.windowEvent.resolution; }
};