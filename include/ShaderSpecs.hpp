#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "vdu/Shaders.hpp"

class CombineOverlaysShader : public vdu::ShaderProgram
{
public:
	CombineOverlaysShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/combineOverlays.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/combineOverlays.glsl");
	}
};

class SpotShadowShader : public vdu::ShaderProgram
{
public:
	SpotShadowShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/spotShadow.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/spotShadow.glsl");
	}
};

class PointShadowShader : public vdu::ShaderProgram
{
public:
	PointShadowShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/pointShadow.glsl");
		addModule(vdu::ShaderStage::Geometry, Engine::workingDirectory + "/res/shaders/pointShadow.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/pointShadow.glsl");
	}
};

class SunShadowShader : public vdu::ShaderProgram
{
public:
	SunShadowShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/sunShadow.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/sunShadow.glsl");
	}
};

class ScreenShader : public vdu::ShaderProgram
{
public:
	ScreenShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/screen.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/screen.glsl");
	}
};

class OverlayShader : public vdu::ShaderProgram
{
public:
	OverlayShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/overlay.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/overlay.glsl");
	}
};

class GBufferShader : public vdu::ShaderProgram
{
public:
	GBufferShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/gBuffer.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/gBuffer.glsl");
	}
};

class PBRShader : public vdu::ShaderProgram
{
public:
	PBRShader()
	{
		addModule(vdu::ShaderStage::Compute, Engine::workingDirectory + "/res/shaders/pbr.glsl");
	}
};


