#pragma once
#include "PCH.hpp"

using namespace chaiscript;

class ScriptEnv
{
public:
	// Constructs the chaiscript environment
	ChaiScript chai;

	// Adds our c++ functions and types to chaiscript
	void initChai();

	void evalFile(std::string path);

	void evalString(std::string script);
};




