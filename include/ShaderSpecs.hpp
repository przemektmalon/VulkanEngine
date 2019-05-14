#pragma once
#include "PCH.hpp"
#include "Engine.hpp"

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

class GBufferNoTexShader : public vdu::ShaderProgram
{
public:
	GBufferNoTexShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/gBufferNoTex.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/gBufferNoTex.glsl");
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

class CombineSceneShader : public vdu::ShaderProgram
{
public:
	CombineSceneShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/combineScene.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/combineScene.glsl");
	}
};

class SSAOShader : public vdu::ShaderProgram
{
public:
	SSAOShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/ssao.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/ssao.glsl");
	}
};

class SSAOBlurShader : public vdu::ShaderProgram
{
public:
	SSAOBlurShader()
	{
		addModule(vdu::ShaderStage::Vertex, Engine::workingDirectory + "/res/shaders/ssaoBlur.glsl");
		addModule(vdu::ShaderStage::Fragment, Engine::workingDirectory + "/res/shaders/ssaoBlur.glsl");
	}
};