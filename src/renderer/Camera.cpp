#include "Camera.hpp"
#include "../Event.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Camera::Camera()
	: m_ControlsType(std::make_unique<TrackballControls>(this))
{
	UpdateProjection();
	UpdateView();
}

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
	: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
	, m_ControlsType(std::make_unique<TrackballControls>(this))
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

	m_ControlsType->OnEvent(ev);
}

void Camera::OnUpdate(float ts)
{
	m_ControlsType->OnUpdate(ts);
}

void Camera::LookAt(const glm::vec3& point)
{
	glm::vec3 direction = glm::normalize(point - Position);

	m_Yaw = glm::degrees(std::atan2f(direction.x, direction.z));
	m_Pitch = glm::degrees(std::asinf(-direction.y));
}

void Camera::SetControls(std::unique_ptr<CameraControls>&& controls)
{
	m_ControlsType = std::move(controls);
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
	m_View = glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(GetOrientation());
	m_View = glm::inverse(m_View);
}
