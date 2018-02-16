#include "PCH.hpp"
#include "Engine.hpp"

/*
	@brief	Entry point
*/
int main(int argc, char* argv[])
{
#ifdef _WIN32
	char workingDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, workingDir);
	DBG_INFO("Working Directory: " << workingDir);
#endif

	Engine::start();
}