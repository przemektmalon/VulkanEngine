#pragma once
#include "PCH.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Scripting.hpp"
#include "Threading.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

void Console::create()
{
	layer = new OLayer();
	layer->create(glm::ivec2(Engine::window->resX, 276));

	input = new Text();

	input->setName("constext");
	input->setFont(Engine::assets.getFont("consola"));
	input->setColour(glm::fvec4(0.1, 0.9, 0.1, 1.0));
	input->setCharSize(20);
	input->setString("> ");
	input->setPosition(glm::fvec2(3, 253));
	
	input->setDepth(4);

	layer->addElement(input);

	backs[0] = new UIPolygon();
	backs[1] = new UIPolygon();

	backs[0]->reserveBuffer(6);
	backs[1]->reserveBuffer(6);

	backs[0]->setTexture(Engine::assets.getTexture("blank"));
	backs[1]->setTexture(Engine::assets.getTexture("blank"));

	backs[0]->setColour(glm::fvec4(0.1, 0.1, 0.1, 0.9f));
	backs[1]->setColour(glm::fvec4(0.05, 0.05, 0.05, 1.f));

	backs[0]->setDepth(1);
	backs[1]->setDepth(3);

	layer->addElement(backs[0]);
	layer->addElement(backs[1]);

	blinker = new UIPolygon();

	blinker->reserveBuffer(6);
	blinker->setTexture(Engine::assets.getTexture("blank"));
	blinker->setColour(glm::fvec4(0.9, 0.9, 0.9, 1.f));
	blinker->setDepth(5);

	update();

	layer->addElement(blinker);

	layer->setDoDraw(active);
	
	updateBacks();
}

void Console::update()
{
	messagePostMutex.lock();
	if (input->getString() != oldText || oldBlinkerPosition != blinkerPosition)
	{
		std::vector<Vertex2D> verts;

		int x = input->getCharsPosition(blinkerPosition).x + 1;
		int y = 274;
		int w = 2;
		int h = 24;

		verts.push_back({ { x, y },{ 0, 0 } });
		verts.push_back({ { x + w, y },{ 1, 0 } });
		verts.push_back({ { x, y - h },{ 0, 1 } });
		verts.push_back({ { x, y - h },{ 0, 1 } });
		verts.push_back({ { x + w, y },{ 1, 0 } });
		verts.push_back({ { x + w, y - h },{ 1, 1 } });

		blinker->setVerts(verts);

		oldText = input->getString();
		oldBlinkerPosition = blinkerPosition;
	}

	timeSinceBlink += Engine::frameTime.getSeconds();
	if (timeSinceBlink > 0.5)
	{
		blinker->toggleDoDraw();
		timeSinceBlink = 0;
	}
	messagePostMutex.unlock();
}

void Console::inputChar(char c)
{
	if (!isActive())
		return;

	timeSinceBlink = 0;
	blinker->setDoDraw(true);

	std::string s; s = input->getString();
	
	if (c == 8) // backspace
	{
		if (blinkerPosition > 1)
		{
			s.erase(blinkerPosition, 1);
			blinkerPosition -= 1;
		}
	}
	else if (c == 127) // delete
	{
		if (blinkerPosition < input->getString().length() - 1)
		{
			s.erase(blinkerPosition + 1, 1);
		}
	}
	else if (c == 13) // enter
	{
		postMessage(std::string(input->getString().c_str() + 2), glm::fvec3(0.9, 0.9, 0.9));

		layer->removeElement(input);
		delete input;

		input = new Text();

		input->setFont(Engine::assets.getFont("consola"));
		input->setColour(glm::fvec4(0.1, 0.9, 0.1, 1.0));
		input->setCharSize(20);
		input->setString("> ");
		input->setPosition(glm::fvec2(3, 253));
		input->setDepth(4);

		layer->addElement(input);

		if (s.length() > 2)
		{
			std::string result("Done");
			glm::fvec4 col(0.1, 0.1, 0.9, 1.0);
			try {
				
				// Submit command and get boxed return value
				auto ret = Engine::scriptEnv.evalString(std::string(s.c_str() + 2));

				// Get boxed values type info
				auto& retVal = ret.get();
				auto& typeInfo = retVal.type();

				auto sh_string = std::make_shared<std::string const>();
				auto sh_string_nonconst = std::make_shared<std::string>();

				chaiscript::detail::Any any_string(sh_string);
				chaiscript::detail::Any any_string_nonconst(sh_string_nonconst);

				// Print correctly stringified value
				if (typeInfo == any_string.type())
				{
					decltype(sh_string) valPtr = retVal.cast<decltype(sh_string)>();
					result = *valPtr;
				}
				else if (typeInfo == any_string_nonconst.type())
				{
					decltype(sh_string_nonconst) valPtr = retVal.cast<decltype(sh_string_nonconst)>();
					result = *valPtr;
				}
				else
				{
					result = std::string("Done ( ") + typeInfo.name() + " )";
				}


				// Macros for checking equality of type infos
				/// TODO: Watch this for cross compiler errors !
#define CHECK_PRINT_TYPE_INFO(TYPE) \
	auto sh_##TYPE## = std::make_shared < ##TYPE## >(); \
	chaiscript::detail::Any any_##TYPE## ( sh_##TYPE## ); \
	if (typeInfo == any_##TYPE##.type() ) { \
		decltype( sh_##TYPE## ) valPtr = retVal.cast<decltype( sh_##TYPE## )>(); \
		result = std::to_string(*valPtr); }

#define CHECK_PRINT_CONST_TYPE_INFO(TYPE) \
	auto sh_const_##TYPE## = std::make_shared < ##TYPE const>(); \
	chaiscript::detail::Any any_const_##TYPE## ( sh_const_##TYPE## ); \
	if (typeInfo == any_const_##TYPE##.type() ) { \
		decltype( sh_const_##TYPE## ) valPtr = retVal.cast<decltype( sh_const_##TYPE## )>(); \
		result = std::to_string(*valPtr); }

				// Signed integers
				CHECK_PRINT_TYPE_INFO(s8);
				CHECK_PRINT_CONST_TYPE_INFO(s8);
				CHECK_PRINT_TYPE_INFO(s16);
				CHECK_PRINT_CONST_TYPE_INFO(s16);
				CHECK_PRINT_TYPE_INFO(s32);
				CHECK_PRINT_CONST_TYPE_INFO(s32);
				CHECK_PRINT_TYPE_INFO(s64);
				CHECK_PRINT_CONST_TYPE_INFO(s64);

				// Unsigned integers
				CHECK_PRINT_TYPE_INFO(u8);
				CHECK_PRINT_CONST_TYPE_INFO(u8);
				CHECK_PRINT_TYPE_INFO(u16);
				CHECK_PRINT_CONST_TYPE_INFO(u16);
				CHECK_PRINT_TYPE_INFO(u32);
				CHECK_PRINT_CONST_TYPE_INFO(u32);
				CHECK_PRINT_TYPE_INFO(u64);
				CHECK_PRINT_CONST_TYPE_INFO(u64);

				// Floating
				CHECK_PRINT_TYPE_INFO(float);
				CHECK_PRINT_CONST_TYPE_INFO(float);
				CHECK_PRINT_TYPE_INFO(double);
				CHECK_PRINT_CONST_TYPE_INFO(double);
			}
			catch (chaiscript::exception::eval_error e)
			{
				result = std::string(e.what());
				col = glm::fvec4(0.9, 0.1, 0.1, 1.0);
			}
			
			postMessage(result, glm::fvec3(col.x, col.y, col.z));
		}
		
		scrollPosition = 0;
		blinkerPosition = 1;

		return;
	}
	else
	{
		blinkerPosition += 1;
		s.insert(blinkerPosition, 1, c);
	}

	input->setString(s);
}

void Console::moveBlinker(Key k)
{
	if (k.code == Key::KC_LEFT)
	{
		blinkerPosition = std::max(1, blinkerPosition - 1);
	}
	else if (k.code == Key::KC_RIGHT)
	{
		blinkerPosition = std::min((int)input->getString().length() - 1, blinkerPosition + 1);
	}
}

void Console::scroll(s16 wheelDelta)
{
	messagePostMutex.lock();
	if (wheelDelta > 0)
	{
		scrollPosition = std::min((std::max((float)output.size(), 10.f) - 10.f) * (input->getGlyphs()->getHeight() + 2), scrollPosition + 22.f);
		updatePositions();
	}
	else
	{
		scrollPosition = std::max(0.f, scrollPosition - 22.f);
		updatePositions();
	}
	messagePostMutex.unlock();
}

void Console::postMessage(std::string msg, glm::fvec3 colour)
{
	messagePostMutex.lock();

	if (output.size() == historyLimit)
	{
		auto t = output.back();
		t->cleanup();
		layer->removeElement(t);
		delete t;
		output.pop_back();
	}
	
	Text* msgText = new Text;

	msgText->setFont(Engine::assets.getFont("consola"));
	msgText->setColour(glm::fvec4(colour.x, colour.y, colour.z, 1.0));
	msgText->setCharSize(20);
	msgText->setString("$ " + msg);
	msgText->setPosition(glm::fvec2(3, 253));
	msgText->setDepth(2);

	layer->addElement(msgText);

	output.push_front(msgText);

	updatePositions();

	messagePostMutex.unlock();
}

void Console::renderAtStartup()
{
	Engine::threading->physToGPUMutex.lock();

	Engine::renderer->overlayRenderer.updateOverlayCommands();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitDstStageMask = 0;
	submitInfo.pCommandBuffers = &Engine::renderer->overlayRenderer.elementCommandBuffer.getHandle();
	submitInfo.commandBufferCount = 1;
	submitInfo.pSignalSemaphores = &Engine::renderer->overlayFinishedSemaphore;
	submitInfo.signalSemaphoreCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(Engine::renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));


	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &Engine::renderer->overlayFinishedSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.pCommandBuffers = &Engine::renderer->overlayRenderer.combineCommandBuffer.getHandle();
	submitInfo.pSignalSemaphores = &Engine::renderer->overlayCombineFinishedSemaphore;
	submitInfo.signalSemaphoreCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(Engine::renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(Engine::renderer->device, Engine::renderer->swapChain, std::numeric_limits<uint64_t>::max(), Engine::renderer->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS)
	{
		DBG_SEVERE("Could not acquire next image");
		return;
	}

	VkPipelineStageFlags waitStages2[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore waitSems[] = { Engine::renderer->imageAvailableSemaphore, Engine::renderer->overlayCombineFinishedSemaphore };
	submitInfo.pWaitSemaphores = waitSems;
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitDstStageMask = waitStages2;
	submitInfo.pCommandBuffers = &Engine::renderer->screenCommandBuffers.getHandle(imageIndex);
	submitInfo.pSignalSemaphores = &Engine::renderer->screenFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(Engine::renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &Engine::renderer->screenFinishedSemaphore;

	VkSwapchainKHR swapChains[] = { Engine::renderer->swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	VK_CHECK_RESULT(vkQueuePresentKHR(Engine::renderer->presentQueue, &presentInfo));

	VK_CHECK_RESULT(vkQueueWaitIdle(Engine::renderer->presentQueue));

	Engine::threading->physToGPUMutex.unlock();

	if (!Engine::initialised.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
		auto renderAgainFunc = std::bind(&Console::renderAtStartup, this);
		auto renderAgainJob = new Job<decltype(renderAgainFunc)>(renderAgainFunc, defaultAsyncJobDoneFunc);
		Engine::threading->addGraphicsJob(renderAgainJob, 1);
	}
}

void Console::updatePositions()
{
	int y = 251 - input->getGlyphs()->getHeight() + scrollPosition;
	for (auto line : output)
	{
		line->setPosition(glm::fvec2(3, y));
		y -= input->getGlyphs()->getHeight() + 2;
	}
}

void Console::updateBacks()
{
	int w = Engine::window->resX, h = 250;
	std::vector<Vertex2D> verts;

	verts.push_back({ { 0, 0 },{ 0, 0 } });
	verts.push_back({ { 0, h },{ 0, 1 } });
	verts.push_back({ { w, 0 },{ 1, 0 } });
	verts.push_back({ { 0, h },{ 0, 1 } });
	verts.push_back({ { w, h },{ 1, 1 } });
	verts.push_back({ { w, 0 },{ 1, 0 } });
	backs[0]->setVerts(verts);

	verts.clear();

	verts.push_back({ { 0, h },{ 0, 0 } });
	verts.push_back({ { 0, h+26 },{ 0, 1 } });
	verts.push_back({ { w, h },{ 1, 0 } });
	verts.push_back({ { 0, h+26 },{ 0, 1 } });
	verts.push_back({ { w, h+26 },{ 1, 1 } });
	verts.push_back({ { w, h },{ 1, 0 } });
	backs[1]->setVerts(verts);

	verts.clear();
}
