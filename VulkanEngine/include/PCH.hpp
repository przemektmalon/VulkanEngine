
#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif


// Debugging output
#ifdef _WIN32

/*
bit 0 - foreground blue
bit 1 - foreground green
bit 2 - foreground red
bit 3 - foreground intensity

bit 4 - background blue
bit 5 - background green
bit 6 - background red
bit 7 - background intensity
*/

#define DBG_SEVERE(msg) { \
	std::stringstream ss; \
	ss << msg; \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b11001111); \
	std::cout << "! SEVERE  -"; \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
	std::cout << " " << ss.str() << std::endl; \
	MessageBoxA(NULL, LPCSTR(ss.str().c_str()), "Severe Error!", MB_OK); }

#define DBG_WARNING(msg) { \
	std::stringstream ss; \
	ss << msg; \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b01101111); \
	std::cout << "! Warning -"; \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
	std::cout << " " << ss.str() << std::endl; }

#define DBG_INFO(msg) { \
	std::stringstream ss; \
	ss << msg; \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00011111); \
	std::cout << "Info      -"; \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
	std::cout << " " << ss.str() << std::endl; }

#endif



/// Utilities includes

#include <cstdlib>
// system, getenv, malloc, calloc, realloc, free, atof, atoi, abs

#include <csignal>
// signal handlers

#include <functional>

#include <utility>
// pair, make_pair

#include <ctime>

#include <chrono>

#include <initializer_list>

/// Numeric limits includes

#include <limits>

/// Error handling includes

#include <cassert>

/// Strings includes

#include <string>

/// Containers includes

#include <vector>

#include <list>

#include <set>

#include <unordered_set>

#include <map>

#include <unordered_map>

/// Algorithms includes

#include <algorithm>

/// Iterators includes

#include <iterator>

/// Numerics includes

#include <cmath>

#include <random>

/// IO includes

#include <ios>

#include <iostream>

#include <fstream>

#include <cstdio>

#include <sstream>

/// Regex includes

#include <regex>

///  Thread support ?