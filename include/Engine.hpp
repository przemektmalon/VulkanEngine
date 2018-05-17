#pragma once
#include "PCH.hpp"
#include "VulkanWrappers.hpp"
#include "Clock.hpp"
#include "Time.hpp"
#include "Camera.hpp"
#include "World.hpp"
#include "AssetStore.hpp"
#include "PhysicsWorld.hpp"
#include "Console.hpp"
#include "Scripting.hpp"

class Threading;
class OLayer;
class Window;
class Renderer;

/*
	@brief Main engine class that collates the engine sub-classes and sub-systems
*/
class Engine
{
public:
	Engine() {}
	~Engine() {}

	static void start();
	static void createVulkanInstance();
	static void queryVulkanPhysicalDeviceDetails();
	static void createWindow();
	static void quit();

	static VkPhysicalDevice getPhysicalDevice() { return vkPhysicalDevice; }
	static PhysicalDeviceDetails& getPhysicalDeviceDetails() { return physicalDevicesDetails[physicalDeviceIndex]; }

#ifdef ENABLE_VULKAN_VALIDATION
	static VkDebugReportCallbackEXT debugCallbackInfo;
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
#endif

#ifdef _WIN32
	static HINSTANCE win32InstanceHandle;
#endif
#ifdef __linux__
	static xcb_connection_t * connection;
#endif

	static AssetStore assets;
	static World world;
	static Clock clock;
	static Window* window;
	static Renderer* renderer;
	static Camera camera;
	static PhysicsWorld physicsWorld;
	static Console* console;
	static ScriptEnv scriptEnv;
	static Threading* threading;

	static VkInstance vkInstance;
	static VkPhysicalDevice vkPhysicalDevice;
	static std::vector<PhysicalDeviceDetails> physicalDevicesDetails;
	static int physicalDeviceIndex;
	
	static Time frameTime;
	static bool engineRunning;
	static Time engineStartTime;
	static std::mt19937_64 rand;
	static float maxDepth;
	static bool validationWarning;
	static std::string validationMessage;
	static OLayer* uiLayer;
	static u64 gpuTimeStamps[12];
};