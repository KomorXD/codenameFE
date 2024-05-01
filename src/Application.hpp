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

struct AppStats
{
	float FrameTime;
	float UpdateTime;
	float RenderTime;
	float ImGuiRenderTime;
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
	inline const AppStats& Stats()  const { return m_Stats;  }

	inline static Application* Instance() { return s_Instance; }

private:
	void SetWindowCallbacks();
	void ApplyImGuiStyles();

	WindowSpec m_Spec{};
	GLFWwindow* m_Window = nullptr;
	EventQueue m_EventQueue;
	AppStats m_Stats{};

	std::stack<std::unique_ptr<Layer>> m_Layers;
	std::unique_ptr<Layer> m_NextLayer;
	bool m_DoPopLayer = false;

	inline static Application* s_Instance = nullptr;
};