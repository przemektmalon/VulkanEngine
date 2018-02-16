#pragma once
#include "PCH.hpp"
#include "Keyboard.hpp"

/*
	@brief	Event superclass. Unions all types of events and their metadata.
*/
struct Event
{
	enum Type { MouseMove, MouseDown, MouseUp, MouseWheel, KeyDown, KeyUp, WindowResized, WindowMoved };
	Type type;

	Event() {}
	Event(Type pType) : type(pType) {}

	union {
		struct key_struct {
			Key key;
			bool shift;
			bool alt;
			bool sys;
			bool ctrl;
			bool caps;
		};
		key_struct keyEvent;
	};

	void constructKey(Key pKey, bool pShift, bool pAlt, bool pSys, bool pCtrl, bool pCaps)
	{
		keyEvent.key = pKey; keyEvent.shift = pShift; keyEvent.alt = pAlt; keyEvent.sys = pSys; keyEvent.ctrl = pCtrl; keyEvent.caps = pCaps;
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
	Key getKey() { return keyEvent.key; }
	bool getShift() { return keyEvent.shift; }
	bool getAlt() { return keyEvent.alt; }
	bool getSys() { return keyEvent.sys; }
	bool getCtrl() { return keyEvent.ctrl; }
	bool getCaps() { return keyEvent.caps; }
};