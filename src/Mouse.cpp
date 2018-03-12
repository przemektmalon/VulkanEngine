#include "Mouse.hpp"
#include "Window.hpp"

glm::ivec2 Mouse::getScreenPosition()
{
#ifdef _WIN32
	POINT m;
	GetCursorPos(&m);
	return glm::ivec2(m.x, m.y);
#endif
}

glm::ivec2 Mouse::getWindowPosition(const Window* pWnd)
{
#ifdef _WIN32
	POINT m;
	GetCursorPos(&m);
	ScreenToClient(pWnd->win32WindowHandle, &m);
	return glm::ivec2(m.x, m.y);
#endif
}

glm::ivec2 Mouse::windowPosition;
MouseCode Mouse::state;
glm::ivec2 Mouse::wheelDelta;