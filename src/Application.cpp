#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Application.hpp"
#include "Application.hpp"
#include "Logger.hpp"
#include "layers/EditorLayer.hpp"

Application::Application(const WindowSpec& spec)
	: m_Spec(spec)
{
	assert((s_Instance == nullptr) && "Only one instance of Application allowed!");

	if (glfwInit() == GLFW_FALSE)
	{
		LOG_CRITICAL("Failed to initialize GLFW!");

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
		LOG_CRITICAL("Failed to create a window!");

		return;
	}

	glfwMakeContextCurrent(m_Window);
	glfwSwapInterval(0);
	glfwSetWindowUserPointer(m_Window, this);
	SetWindowCallbacks();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		LOG_CRITICAL("Failed to load GLAD!");

		return;
	}

	Logger::Init();

	s_Instance = this;
}

Application::~Application()
{
	s_Instance = nullptr;
}

void Application::Run()
{
	m_Layers.push(std::make_unique<EditorLayer>());
	
	double prevTime = 0.0;
	double currTime = 0.0;
	double timestep = 1.0 / 60.0;

	while (!glfwWindowShouldClose(m_Window))
	{
		prevTime = currTime;
		currTime = glfwGetTime();
		timestep = currTime - prevTime;

		Event ev{};
		while (m_EventQueue.PollEvents(ev))
		{
			m_Layers.top()->OnEvent(ev);
		}

		m_Layers.top()->OnUpdate((float)timestep);
		m_Layers.top()->OnRender();

		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}
}

void Application::Shutdown()
{
	LOG_WARN("Application is about to shut down...");

	glfwSetWindowShouldClose(m_Window, true);
}

void Application::PushLayer(std::unique_ptr<Layer>&& layer)
{
	m_Layers.push(std::move(layer));
	m_Layers.top()->OnAttach();
}

void Application::PopLayer()
{
	if (m_Layers.empty())
	{
		return;
	}

	m_Layers.pop();

	if (!m_Layers.empty())
	{
		m_Layers.top()->OnAttach();
	}
}

void Application::SetWindowCallbacks()
{
	glfwSetErrorCallback(
		[](int errorCode, const char* description)
		{
			LOG_ERROR("GLFW error #{}: {}", errorCode, description);
		});

	glfwSetKeyCallback(m_Window,
		[](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			Event ev{};

			switch (action)
			{
			case GLFW_PRESS:   ev.Type = Event::KeyPressed;  break;
			case GLFW_RELEASE: ev.Type = Event::KeyReleased; break;
			case GLFW_REPEAT:  ev.Type = Event::KeyHeld;	 break;
			}

			ev.Key.Code = (Key)key;
			ev.Key.AltPressed = Input::IsKeyPressed((Key)GLFW_KEY_LEFT_ALT);
			ev.Key.CtrlPressed = Input::IsKeyPressed((Key)GLFW_KEY_LEFT_CONTROL);
			ev.Key.ShiftPressed = Input::IsKeyPressed((Key)GLFW_KEY_LEFT_SHIFT);

			app->m_EventQueue.SubmitEvent(ev);
		});

	glfwSetCursorPosCallback(m_Window,
		[](GLFWwindow* window, double xPos, double yPos)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			Event ev{};

			ev.Type = Event::MouseMoved;
			ev.Mouse.PosX = (float)xPos;
			ev.Mouse.PosY = (float)yPos;

			app->m_EventQueue.SubmitEvent(ev);
		});

	glfwSetMouseButtonCallback(m_Window,
		[](GLFWwindow* window, int button, int action, int mods)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			Event ev{};

			ev.Type = (action == GLFW_RELEASE ? Event::MouseButtonReleased : Event::MouseButtonPressed);
			ev.MouseButton.Button = (MouseButton)button;

			app->m_EventQueue.SubmitEvent(ev);
		});

	glfwSetScrollCallback(m_Window,
		[](GLFWwindow* window, double xOffset, double yOffset)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			Event ev{};

			ev.Type = Event::MouseWheelScrolled;
			ev.MouseWheel.OffsetX = (float)xOffset;
			ev.MouseWheel.OffsetY = (float)yOffset;

			app->m_EventQueue.SubmitEvent(ev);
		});

	glfwSetWindowSizeCallback(m_Window,
		[](GLFWwindow* window, int width, int height)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			Event ev{};

			ev.Type = Event::WindowResized;
			ev.Size.Width = width;
			ev.Size.Height = height;

			app->m_Spec.Width = width;
			app->m_Spec.Height = height;

			app->m_EventQueue.SubmitEvent(ev);
		});
}