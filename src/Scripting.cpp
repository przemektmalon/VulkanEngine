#include "PCH.hpp"
#include "Scripting.hpp"
#include "File.hpp"
#include "Transform.hpp"
#include "Time.hpp"
#include "Camera.hpp"
#include "World.hpp"
#include "Keyboard.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "CommonJobs.hpp"
#include "EngineConfig.hpp"

using namespace chaiscript;

void ScriptEnv::initChai()
{
	glm::fvec2(*add2Func)(const glm::fvec2&, const glm::fvec2&) = glm::operator+;
	glm::fvec2(*sub2Func)(const glm::fvec2&, const glm::fvec2&) = glm::operator-;
	glm::fvec2(*mul2Func)(const glm::fvec2&, const glm::fvec2&) = glm::operator*;
	//glm::fvec2(*mul2aFunc)(const glm::fvec2&, const float&) = glm::operator*;

	glm::fvec3(*add3Func)(const glm::fvec3&, const glm::fvec3&) = glm::operator+;
	glm::fvec3(*sub3Func)(const glm::fvec3&, const glm::fvec3&) = glm::operator-;
	glm::fvec3(*mul3Func)(const glm::fvec3&, const glm::fvec3&) = glm::operator*;
	//glm::fvec3(*mul3aFunc)(const glm::fvec3&, const float&) = glm::operator*;

	glm::fvec4(*add4Func)(const glm::fvec4&, const glm::fvec4&) = glm::operator+;
	glm::fvec4(*sub4Func)(const glm::fvec4&, const glm::fvec4&) = glm::operator-;
	glm::fvec4(*mul4Func)(const glm::fvec4&, const glm::fvec4&) = glm::operator*;
	//glm::fvec4(*mul4aFunc)(const glm::fvec4&, const float) = glm::operator*;

	glm::fvec4(*mul4ByMatFunc)(const glm::fvec4&, const glm::fmat4&) = glm::operator*;
	glm::fvec3(*mul3ByMatFunc)(const glm::fvec3&, const glm::fmat3&) = glm::operator*;

	auto negateVec3 = [](glm::fvec3& v) -> glm::fvec3 { return glm::fvec3(0) - v; };
	auto negateVec4 = [](glm::fvec4& v) -> glm::fvec4 { return glm::fvec4(0) - v; };

	/// TODO: 0 <= row <= 3 check ?
	//auto mat4GetRowFunc = [](const glm::fmat4& mat, int row) -> glm::fvec4 { return mat[row]; };
	//auto mat4SetRowFunc = [](glm::fmat4& mat, int row, glm::fvec4 set) -> void { mat[row] = set; };

	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<glm::fvec2>(*m,
			"fvec2",
			{ constructor<glm::fvec2()>(),
			constructor<glm::fvec2(const glm::fvec2 &)>(),
			constructor<glm::fvec2(float,float)>() },
			{ { fun(&glm::fvec2::x), "x" },
			{ fun(&glm::fvec2::y), "y" },
			{ fun(add2Func), "+" },
			{ fun(sub2Func), "-" },
			{ fun(mul2Func), "*" },
			/*{ fun(mul2aFunc), "*"},*/ }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<glm::ivec2>(*m,
			"ivec2",
			{ constructor<glm::ivec2()>(),
			constructor<glm::ivec2(const glm::fvec2 &)>(),
			constructor<glm::ivec2(float,float)>() },
			{ { fun(&glm::ivec2::x), "x" },
			{ fun(&glm::ivec2::y), "y" } }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<glm::fvec3>(*m,
			"fvec3",
			{ constructor<glm::fvec3()>(),
			constructor<glm::fvec3(const glm::fvec3 &)>(),
			constructor<glm::fvec3(const glm::fvec4 &)>(),
			constructor<glm::fvec3(float,float,float)>(), },
			{ { fun(&glm::fvec3::x), "x" },
			{ fun(&glm::fvec3::y), "y" },
			{ fun(&glm::fvec3::z), "z" },
			{ fun(add3Func), "+" },
			{ fun(sub3Func), "-" },
			{ fun(mul3Func), "*" },
			/*{ fun(mul3aFunc), "*" },*/
			{ fun(mul3ByMatFunc), "*" },
			{ fun(negateVec3), "negate" } }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<glm::fvec4>(*m,
			"fvec4",
			{ constructor<glm::fvec4()>(),
			constructor<glm::fvec4(const glm::fvec4 &)>(),
			constructor<glm::fvec4(float,float,float,float)>() },
			{ { fun(&glm::fvec4::x), "x" },
			{ fun(&glm::fvec4::y), "y" },
			{ fun(&glm::fvec4::z), "z" },
			{ fun(&glm::fvec4::w), "w" },
			{ fun(add4Func), "+" },
			{ fun(sub4Func), "-" },
			{ fun(mul4Func), "*" },
			/*{ fun(mul4aFunc), "*" },*/
			{ fun(mul4ByMatFunc), "*" },
			{ fun(negateVec4), "negate" } }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<glm::fmat4>(*m,
			"fmat4",
			{ constructor<glm::fmat4()>(),
			constructor<glm::fmat4(const glm::fmat4 &)>() }, {
				//{ { fun(&mat4GetRowFunc), "getRow" },
				//{ fun(&mat4SetRowFunc), "setRow" }, 
			}
			);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<Camera>(*m,
			"Camera",
			{ constructor<Camera()>() },
			{ { fun(&Camera::move), "move" },
			{ fun(&Camera::update), "update" },
			{ fun(&Camera::getMatYaw), "getMatYaw", },
			{ fun(&Camera::setFOV), "setFOV" },
			{ fun(&Camera::setPosition), "setPosition" },
			{ fun(&Camera::lockView), "lockView"},
			{ fun(&Camera::freeView), "freeView"}, }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<Transform>(*m,
			"Transform",
			{ constructor<Transform()>() },
			{ { fun(&Transform::getTransformMat), "getTransformMat" },
			{ fun(&Transform::setTransformMat), "setTransformMat" },
			{ fun(&Transform::getInverseTransformMat), "getInverseTransformMat" },
			{ fun(&Transform::setTranslation), "setTranslation" },
			{ fun(&Transform::getTranslation), "getTranslation" },
			{ fun(&Transform::translate), "translate" },
			{ fun(&Transform::setQuat), "setQuat" },
			{ fun(&Transform::getQuat), "getQuat" },
			{ fun(&Transform::setScale), "setScale" },
			{ fun(&Transform::getScale), "getScale" },
			{ fun(&Transform::setOrigin), "setOrigin" },
			{ fun(&Transform::getOrigin), "getOrigin" },
			{ fun(&Transform::updateMatrix), "updateMatrix" } }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<World>(*m,
			"World",
			{ constructor<World()>() },
			{ { fun(&World::addModelInstance), "addModel" },
			{ fun(&World::removeModelInstance), "removeModel" },
			{ fun(&World::getModelInstance), "getModel" } }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<ModelInstance>(*m,
			"Model",
			{ constructor<ModelInstance()>() },
			{ { fun(&ModelInstance::getTransform), "getTransform" },
			{ fun(&ModelInstance::setTransform), "setTransform" },
			{ fun(&ModelInstance::makePhysicsObject), "makePhysical" },
			{ fun(&ModelInstance::setMaterial), "setMaterial" } }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<Material>(*m,
			"Material",
			{ constructor<Material()>() },
			{  }
		);
		chai.add(m);
	}
	{
		void(AssetStore::*addMaterialFlat)(std::string, glm::fvec3, float, float) = &AssetStore::addMaterial;

		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<AssetStore>(*m,
			"AssetStore",
			{ constructor<AssetStore()>() },
			{ { fun(&AssetStore::getMaterial), "getMaterial" },
			{ fun(&AssetStore::getModel), "getModel" },
			{ fun(addMaterialFlat), "addMaterial" } }
		);
		chai.add(m);
		}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<Event>(*m,
			"Event",
			{ constructor<Event()>() },
			{ { fun(&Event::type), "type" },
			{ fun(&Event::getKeyEvent), "keyEvent" },
			{ fun(&Event::getMouseEvent), "mouseEvent" }, 
			{ fun(&Event::getTextEvent), "textEvent" }, }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		chaiscript::utility::add_class<Event::Type>(*m,
			"EventType",
			{ { Event::MouseMove, "MouseMove" },
			{ Event::MouseDown, "MouseDown" },
			{ Event::MouseUp, "MouseUp" },
			{ Event::MouseWheel, "MouseWheel" },
			{ Event::KeyDown, "KeyDown" },
			{ Event::KeyUp, "KeyUp" },
			{ Event::WindowResized, "WindowResized" },
			{ Event::WindowMoved, "WindowMoved" },
			{ Event::TextInput, "TextInput" }
			}
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new chaiscript::Module());
		utility::add_class<Console>(*m,
			"Console",
			{ constructor<Console()>() },
			{ { fun(&Console::postMessage), "postMessage" },
			  { fun(&Console::isActive), "isActive" },
			  { fun(&Console::toggle), "toggle" } }
		);
		chai.add(m);
	}

	glm::fvec3(*crossFunc)(const glm::fvec3&, const glm::fvec3&) = glm::cross;

	chai.add(fun(crossFunc), "cross");

	bool(*isKeyPressedFunc)(char) = Keyboard::isKeyPressed;

#define ADD_TO_STRING_OVERLOAD(TYPE) \
	std::string( *to_string_##TYPE## )( ##TYPE## ) = std::to_string; \
	chai.add(fun( to_string_##TYPE## ), "to_string");
	
	ADD_TO_STRING_OVERLOAD(double);
	ADD_TO_STRING_OVERLOAD(float);
	ADD_TO_STRING_OVERLOAD(u64);
	ADD_TO_STRING_OVERLOAD(s64);
	ADD_TO_STRING_OVERLOAD(u32);
	ADD_TO_STRING_OVERLOAD(s32);

	chai.add(fun(isKeyPressedFunc), "isKeyPressed");
	chai.add(fun([](Event& ev)->bool { return Engine::window->eventQ.pollEvent(ev); }), "pollEvent");
	chai.add(fun([]()->float { return Engine::frameTime.getSecondsf(); }), "getFrameTime");
	chai.add(fun([]()->float { return Engine::scriptTickTime.getSecondsf(); }), "getScriptTickTime");
	chai.add(fun([]()->u64 { return Engine::clock.now(); }), "getCurrentTime");
	chai.add(fun([]()->Camera& { return Engine::camera; }), "getCamera");
	chai.add(fun([]()->World& { return Engine::world; }), "getWorld");

	{
		ModulePtr m = ModulePtr(new Module());
		utility::add_class<EngineConfig::Render::SSAO>(*m,
			"EngineConfig::Render::SSAO",
			{  },
			{ { fun(&EngineConfig::Render::SSAO::setRadius), "setRadius" },
			  { fun(&EngineConfig::Render::SSAO::setSamples), "setSamples" },
			  { fun(&EngineConfig::Render::SSAO::setProjScale), "setProjScale" }, 
			  { fun(&EngineConfig::Render::SSAO::setSpiralTurns), "setSpiralTurns" }, 
			  { fun(&EngineConfig::Render::SSAO::setBias), "setBias" }, 
			  { fun(&EngineConfig::Render::SSAO::setIntensity), "setIntensity" }, }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new Module());
		utility::add_class<EngineConfig::Render>(*m,
			"EngineConfig::Render",
			{ },
			{ { fun(&EngineConfig::Render::ssao), "ssao" },
			  { fun(&EngineConfig::Render::setResolution), "setResolution"} }
		);
		chai.add(m);
	}
	{
		ModulePtr m = ModulePtr(new Module());
		utility::add_class<EngineConfig>(*m,
			"EngineConfig",
			{ constructor<EngineConfig()>() },
			{ { fun(&EngineConfig::render), "render" } }
		);
		chai.add(m);
	}



}

Boxed_Value ScriptEnv::evalFile(std::string path)
{
	try {
		auto ret = chai.eval_file(path);
		return ret;
	}
	catch (const chaiscript::exception::eval_error &e) {
		DBG_WARNING("Chaiscript eval error: " << e.pretty_print() << '\n');
	}
}

Boxed_Value ScriptEnv::evalString(std::string script)
{
	try {
		auto ret = chai.eval(script);
		return ret;
	}
	catch (const chaiscript::exception::eval_error &e) {
		DBG_WARNING("Chaiscript eval error: " << e.pretty_print() << '\n');
	}
}
