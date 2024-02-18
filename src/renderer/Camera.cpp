#include "Camera.hpp"
#include "../Event.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Camera::Camera()
{
	UpdateProjection();
	UpdateView();
}

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
	: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
{
	UpdateProjection();
	UpdateView();
}

void Camera::OnEvent(Event& ev)
{
	if (ev.Type == Event::WindowResized)
	{
		m_ViewportSize = { (float)ev.Size.Width, (float)ev.Size.Height };

		return;
	}

	if (ev.Type == Event::MouseWheelScrolled)
	{
		m_Position += GetForwardDirection() * ev.MouseWheel.OffsetY;

		return;
	}

	if (ev.Type == Event::KeyPressed && ev.Key.Code == Key::LeftAlt)
	{
		m_ControlType = CameraControlType::FirstPersonControl;
		Input::DisableCursor();

		return;
	}

	if (ev.Type == Event::KeyReleased && ev.Key.Code == Key::LeftAlt)
	{
		m_ControlType = CameraControlType::TrackballControl;
		Input::ShowCursor();

		return;
	}

	if (m_ControlType == CameraControlType::TrackballControl && ev.Type == Event::MouseButtonReleased && 
		(ev.MouseButton.Button == MouseButton::Middle || ev.MouseButton.Button == MouseButton::Right))
	{
		Input::ShowCursor();

		return;
	}
}

void Camera::OnUpdate(float ts)
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = mousePos - m_PrevMousePos;
	float deltaLen = glm::length(delta);
	m_PrevMousePos = mousePos;

	if (m_ControlType == CameraControlType::TrackballControl)
	{
		if (Input::IsMouseButtonPressed(MouseButton::Right))
		{
			Input::DisableCursor();

			m_Yaw += delta.x * 0.1f;
			m_Pitch -= delta.y * 0.1f;

			m_Pitch = glm::max(glm::min(m_Pitch, 90.0f), -90.0f);

			return;
		}

		if (Input::IsMouseButtonPressed(MouseButton::Middle))
		{
			Input::DisableCursor();

			m_Position -= GetRightDirection() * delta.x * 0.02f;
			m_Position -= GetUpDirection() * delta.y * 0.02f;

			return;
		}

		return;
	}
	
	if (m_ControlType == CameraControlType::FirstPersonControl)
	{
		glm::vec3 moveVec(0.0f);

		if (Input::IsKeyPressed(Key::A))
		{
			moveVec -= GetRightDirection();
		}
		else if (Input::IsKeyPressed(Key::D))
		{
			moveVec += GetRightDirection();
		}

		if (Input::IsKeyPressed(Key::W))
		{
			moveVec += GetForwardDirection();
		}
		else if (Input::IsKeyPressed(Key::S))
		{
			moveVec -= GetForwardDirection();
		}

		if (Input::IsKeyPressed(Key::Space))
		{
			moveVec += glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else if (Input::IsKeyPressed(Key::LeftShift))
		{
			moveVec -= glm::vec3(0.0f, 1.0f, 0.0f);
		}

		if (glm::length(moveVec) != 0.0f)
		{
			m_Position += glm::normalize(moveVec) * 15.0f * ts;
		}

		m_Yaw += delta.x * 0.1f;
		m_Pitch -= delta.y * 0.1f;
		m_Pitch = glm::max(glm::min(m_Pitch, 90.0f), -90.0f);

		return;
	}
}

void Camera::LookAt(const glm::vec3& point)
{
	glm::vec3 direction = glm::normalize(point - m_Position);

	m_Yaw = glm::degrees(std::atan2f(direction.x, direction.z));
	m_Pitch = glm::degrees(std::asinf(-direction.y));
}

glm::vec3 Camera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::quat Camera::GetOrientation() const
{
	return glm::quat(glm::vec3(glm::radians(-m_Pitch), glm::radians(-m_Yaw), 0.0f));
}

void Camera::UpdateProjection()
{
	m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
	m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
}

void Camera::UpdateView()
{
	m_View = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(GetOrientation());
	m_View = glm::inverse(m_View);
}
