#include "PCH.hpp"
#include "Engine.hpp"

int main(int argc, char* argv[])
{
	auto pathStr = getenv("PATH");
	DBG_INFO("PATH variables: " << pathStr << std::endl);

	char workingDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, workingDir);
	DBG_INFO("Working Directory: " << workingDir);

	Engine::start();
}