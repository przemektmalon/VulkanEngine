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

	Boxed_Value evalFile(std::string path);

	Boxed_Value evalString(std::string script);
};
