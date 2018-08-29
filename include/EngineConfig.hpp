#pragma once
#include "PCH.hpp"

struct EngineConfig
{
	struct Render
	{
		struct SSAO
		{
			float frameScale;
			int samples;
			int spiralTurns;
			float projScale;
			float radius;
			float bias;
			float intensity;
		} ssao;
	} render;
};