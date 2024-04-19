#pragma once

#include <string>
#include <stack>
#include <memory>

#include "Event.hpp"
#include "Clock.hpp"

struct GLFWwindow;
class Layer;

struct WindowSpec
{
	int32_t Width = 1280;
	int32_t Height = 720;
	std::string Title = "FE";
};

class Application
{
public:
	Application(const WindowSpec& spec = {});
	~Application();

	void Run();
	void Shutdown();
	
	void PushLayer(std::unique_ptr<Layer>&& layer);
	void PopLayer();

	inline GLFWwindow* Window()		const { return m_Window; }
	inline const WindowSpec& Spec() const { return m_Spec;	 }

	inline uint32_t FPS() const { return m_FrameTimeInMS; }
	inline uint32_t EventsTime() const { return m_EventsTimeInMS; }
	inline uint32_t UpdateTime() const { return m_UpdateTimeInMS; }
	inline uint32_t RenderTime() const { return m_RenderTimeInMS; }

	inline static Application* Instance() { return s_Instance; }

private:
	void SetWindowCallbacks();
	void ApplyImGuiStyles();

	WindowSpec m_Spec{};
	GLFWwindow* m_Window = nullptr;
	EventQueue m_EventQueue;

	uint32_t m_FrameTimeInMS;

	Clock m_EventsTimer;
	uint32_t m_EventsTimeInMS;

	Clock m_UpdateTimer;
	uint32_t m_UpdateTimeInMS;

	Clock m_RenderTimer;
	uint32_t m_RenderTimeInMS;

	std::stack<std::unique_ptr<Layer>> m_Layers;
	std::unique_ptr<Layer> m_NextLayer;
	bool m_DoPopLayer = false;

	inline static Application* s_Instance = nullptr;
};