#include "Application.hpp"

#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Application::Application(const WindowSpec& spec)
	: m_Spec(spec)
{
	assert((s_Instance == nullptr) && "Only one instance of Application allowed!");

	if (glfwInit() == GLFW_FALSE)
	{
		fprintf(stderr, "Failed to initialize GLFW!");

		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	m_Window = glfwCreateWindow(m_Spec.Width, m_Spec.Height,
		m_Spec.Title.c_str(), nullptr, nullptr);

	int width{};
	int height{};
	glfwGetWindowSize(m_Window, &width, &height);
	m_Spec.Width = width;
	m_Spec.Height = height;

	if (!m_Window)
	{
		fprintf(stderr, "Failed to create a window!");

		return;
	}

	glfwMakeContextCurrent(m_Window);
	glfwSwapInterval(0);
	glfwSetWindowUserPointer(m_Window, this);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to load GLAD!");

		return;
	}

	s_Instance = this;
}

Application::~Application()
{
	s_Instance = nullptr;
}

void Application::Run()
{
	double prevTime = 0.0f;
	double currTime = 0.0f;
	double timestep = 1.0f / 60.0f;

	while (!glfwWindowShouldClose(m_Window))
	{
		prevTime = currTime;
		currTime = glfwGetTime();
		timestep = currTime - prevTime;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.16f, 0.16f, 0.16f, 1.0f);

		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}
}
