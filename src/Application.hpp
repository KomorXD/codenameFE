#pragma once

#include <string>
#include <memory>

struct GLFWwindow;

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

	inline GLFWwindow* Window() { return m_Window; }
	inline const WindowSpec& Spec() const { return m_Spec; }

	inline static Application& Instance() { return *s_Instance; }

private:
	WindowSpec m_Spec{};
	GLFWwindow* m_Window = nullptr;

	inline static Application* s_Instance = nullptr;
};