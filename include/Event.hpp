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
			Key key;
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
			MouseCode code;
			glm::ivec2 position;
			glm::ivec2 move;
			s16 wheelDelta;
		};
		mouse_struct mouseEvent;
	} eventUnion;

	void constructKey(Key pKey, bool pShift, bool pAlt, bool pSys, bool pCtrl, bool pCaps)
	{
		eventUnion.keyEvent.key = pKey; eventUnion.keyEvent.shift = pShift; eventUnion.keyEvent.alt = pAlt; eventUnion.keyEvent.sys = pSys; eventUnion.keyEvent.ctrl = pCtrl; eventUnion.keyEvent.caps = pCaps;
	}

	void constructMouse(MouseCode pCode, glm::ivec2 pPos, glm::ivec2 pMove, s16 pDelta)
	{
		eventUnion.mouseEvent.code = pCode; eventUnion.mouseEvent.position = pPos; eventUnion.mouseEvent.move = pMove; eventUnion.mouseEvent.wheelDelta = pDelta;
	}

	void constructText(Key pKey, bool pShift)
	{
		unsigned char c = pKey.code;
		if (c == Key::KC_BACKSPACE || c == Key::KC_ENTER)
		{
			eventUnion.textInputEvent.character = c;
			return;
		}
		else if (c == Key::KC_DELETE)
		{
			eventUnion.textInputEvent.character = 127;
			return;
		}
		else if (c >= Key::KC_A && c <= Key::KC_Z)
		{
			if (!pShift)
				c += 32;
			eventUnion.textInputEvent.character = c;
			return;
		}
		else if (c == Key::KC_SPACE)
		{
			eventUnion.textInputEvent.character = 32;
			return;
		}
		else if ((c >= Key::KC_0 && c <= Key::KC_9) ||
			c == Key::KC_MINUS ||
			c == Key::KC_PLUS ||
			c == Key::KC_LEFT_BRACKET ||
			c == Key::KC_RIGHT_BRACKET ||
			c == Key::KC_SEMICOLON ||
			c == Key::KC_APOSTROPHE ||
			c == Key::KC_BACK_SLASH ||
			c == Key::KC_COMMA ||
			c == Key::KC_PERIOD ||
			c == Key::KC_FORWARD_SLASH ||
			c == Key::KC_GRAVE)
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
				case Key::KC_MINUS:
					c = '_';
					break;
				case Key::KC_PLUS:
					c = '+';
					break;
				case Key::KC_LEFT_BRACKET:
					c = '{';
					break;
				case Key::KC_RIGHT_BRACKET:
					c = '}';
					break;
				case Key::KC_SEMICOLON:
					c = ':';
					break;
				case Key::KC_APOSTROPHE:
					c = '"';
					break;
				case Key::KC_FORWARD_SLASH:
					c = '|';
					break;
				case Key::KC_COMMA:
					c = '<';
					break;
				case Key::KC_PERIOD:
					c = '>';
					break;
				case Key::KC_BACK_SLASH:
					c = '?';
					break;
				case Key::KC_GRAVE:
					c = '~';
					break;
				}
			}
			else
			{
				switch (c) {
				case Key::KC_MINUS:
					c = '-';
					break;
				case Key::KC_PLUS:
					c = '=';
					break;
				case Key::KC_LEFT_BRACKET:
					c = '[';
					break;
				case Key::KC_RIGHT_BRACKET:
					c = ']';
					break;
				case Key::KC_SEMICOLON:
					c = ';';
					break;
				case Key::KC_APOSTROPHE:
					c = '\'';
					break;
				case Key::KC_FORWARD_SLASH:
					c = '/';
					break;
				case Key::KC_COMMA:
					c = ',';
					break;
				case Key::KC_PERIOD:
					c = '.';
					break;
				case Key::KC_BACK_SLASH:
					c = '\\';
					break;
				case Key::KC_GRAVE:
					c = '`';
					break;
				}
			}
			eventUnion.textInputEvent.character = c;
			return;
		}
		eventUnion.textInputEvent.character = 0;
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
			events.pop();
			return true;
		} else
		{
			return false;
		}
	}

	void pushEvent(Event& pEvent)
	{
		events.push(pEvent);
	}
};

/*
	@brief	Specialised key event class. Can be passed to functions expecting a key event.
*/
class KeyEvent : private Event
{
public:
	Key getKey() { return eventUnion.keyEvent.key; }
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
	MouseCode getCode() { return eventUnion.mouseEvent.code; }
	glm::ivec2 getPosition() { return eventUnion.mouseEvent.position; }
	glm::fvec2 getMove() { return eventUnion.mouseEvent.move; }
	int getDelta() { return eventUnion.mouseEvent.wheelDelta; }

private:
	void setPosition(glm::ivec2 pPos) { eventUnion.mouseEvent.position = pPos; }
};

/*
	@brief	Specialised text input event class. Can be passed to functions expecting text input.
*/
class TextInputEvent : private Event
{
public:
	char getCharacter() { return eventUnion.textInputEvent.character; }
};