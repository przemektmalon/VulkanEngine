#pragma once
#include "PCH.hpp"

namespace os {

class Window;
typedef s32 MouseCode;

class Mouse
{
public:
	Mouse() {}
	~Mouse() {}

	static glm::ivec2 getScreenPosition();
	static glm::ivec2 getWindowPosition(const Window* pWnd);

	static void setWheelDelta(glm::ivec2 pD) { wheelDelta = pD; }
	static glm::ivec2 getWheelDelta() { return wheelDelta; }
	enum Code
	{
		M_NONE = 0,
		M_LEFT = 1,
		M_RIGHT = 2,
		M_MIDDLE = 4
	};

	static glm::ivec2 windowPosition;
	static MouseCode state;
	static glm::ivec2 wheelDelta;
};

}