#pragma once
#include "PCH.hpp"
#include "Clock.hpp"
#include "Time.hpp"
#include "Camera.hpp"
#include "World.hpp"
#include "AssetStore.hpp"
#include "PhysicsWorld.hpp"
#include "Console.hpp"
#include "Scripting.hpp"
#include "EngineConfig.hpp"
#include "Filesystem.hpp"

class Threading;
class UIElementGroup;
class Renderer;
namespace os {
	class Window;
}

/*
	@brief Main engine class that collates the engine sub-classes and sub-systems
*/
class Engine
{
public:
	Engine() {}
	~Engine() {}

	static void start();
	static void engineLoop();
	static void eventLoop();
	static void processNextMainThreadJob();
	static void createVulkanInstance();
	static void queryVulkanPhysicalDeviceDetails();
	static void createWindow();
	static void quit();

	static void updatePerformanceStatsDisplay();

	static void vduVkDebugCallback(vdu::Instance::DebugReportLevel level, vdu::Instance::DebugObjectType objectType, uint64_t objectHandle, const std::string& objectName, const std::string& message);
	static void vduVkErrorCallback(VkResult result, const std::string& message);
	static void vduDebugCallback(vdu::LogicalDevice::VduDebugLevel level, const std::string& message);

	static Filesystem fs;
	static EngineConfig config;
	static AssetStore assets;
	static World world;
	static Clock clock;
	static os::Window* window;
	static Renderer* renderer;
	static Camera camera;
	static PhysicsWorld physicsWorld;
	static Console* console;
	static ScriptEnv scriptEnv;
	static Threading* threading;
	
	static vdu::Instance vulkanInstance;
	static std::vector<vdu::PhysicalDevice> allPhysicalDevices;
	static vdu::PhysicalDevice* physicalDevice;

	static Time scriptTickTime;
	static Time frameTime;
	static bool engineRunning;
	static Time engineStartTime;
	static std::mt19937_64 rand;
	static float maxDepth;
	static UIElementGroup* uiGroup;
	static u64 gpuTimeStamps[12];
	static std::atomic_char initialised;
	static std::mutex waitForProfilerInitMutex;
	static std::string workingDirectory;
	
	// Used to display stats every X seconds
	static double timeSinceLastStatsUpdate;
};