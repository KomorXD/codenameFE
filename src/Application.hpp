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

	inline float FrameTime()  const { return m_FrameTimeInMS;  }
	inline float EventsTime() const { return m_EventsTimeInMS; }
	inline float UpdateTime() const { return m_UpdateTimeInMS; }
	inline float RenderTime() const { return m_RenderTimeInMS; }

	inline static Application* Instance() { return s_Instance; }

private:
	void SetWindowCallbacks();
	void ApplyImGuiStyles();

	WindowSpec m_Spec{};
	GLFWwindow* m_Window = nullptr;
	EventQueue m_EventQueue;

	float m_FrameTimeInMS;

	Clock m_EventsTimer;
	float m_EventsTimeInMS;
	
	Clock m_UpdateTimer;
	float m_UpdateTimeInMS;

	Clock m_RenderTimer;
	float m_RenderTimeInMS;

	std::stack<std::unique_ptr<Layer>> m_Layers;
	std::unique_ptr<Layer> m_NextLayer;
	bool m_DoPopLayer = false;

	inline static Application* s_Instance = nullptr;
};