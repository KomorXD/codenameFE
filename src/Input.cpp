#include "Input.hpp"
#include "Application.hpp"

#include <GLFW/glfw3.h>

bool Input::IsKeyPressed(Key key)
{
	Application* app = Application::Instance();

	return glfwGetKey(app->Window(), (int32_t)key) == GLFW_PRESS;
}

bool Input::IsMouseButtonPressed(MouseButton button)
{
	Application* app = Application::Instance();

	return glfwGetMouseButton(app->Window(), (int32_t)button) == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition()
{
	Application* app = Application::Instance();
	double xPos{};
	double yPos{};

	glfwGetCursorPos(app->Window(), &xPos, &yPos);

	return { (float)xPos, (float)app->Spec().Height - (float)yPos };
}

void Input::HideCursor()
{
	Application* app = Application::Instance();

	glfwSetInputMode(app->Window(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void Input::DisableCursor()
{
	Application* app = Application::Instance();

	glfwSetInputMode(app->Window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Input::ShowCursor()
{
	Application* app = Application::Instance();

	glfwSetInputMode(app->Window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}