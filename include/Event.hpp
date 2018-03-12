#pragma once
#include "PCH.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"

/*
	@brief	Event superclass. Unions all types of events and their metadata.
*/
struct Event
{
	enum Type { MouseMove, MouseDown, MouseUp, MouseWheel, KeyDown, KeyUp, WindowResized, WindowMoved };
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
		struct mouse_struct {
			MouseCode code;
			glm::ivec2 position;
			glm::ivec2 move;
			int delta;
		};
		mouse_struct mouseEvent;
	} eventUnion;

	void constructKey(Key pKey, bool pShift, bool pAlt, bool pSys, bool pCtrl, bool pCaps)
	{
		eventUnion.keyEvent.key = pKey; eventUnion.keyEvent.shift = pShift; eventUnion.keyEvent.alt = pAlt; eventUnion.keyEvent.sys = pSys; eventUnion.keyEvent.ctrl = pCtrl; eventUnion.keyEvent.caps = pCaps;
	}

	void constructMouse(MouseCode pCode, glm::ivec2 pPos, glm::ivec2 pMove, int pDelta)
	{
		eventUnion.mouseEvent.code = pCode; eventUnion.mouseEvent.position = pPos; eventUnion.mouseEvent.move = pMove; eventUnion.mouseEvent.delta = pDelta;
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
	int getDelta() { return eventUnion.mouseEvent.delta; }

private:
	void setPosition(glm::ivec2 pPos) { eventUnion.mouseEvent.position = pPos; }
};