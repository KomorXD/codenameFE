#include "CameraControls.hpp"
#include "Camera.hpp"
#include "../Animations.hpp"

CameraControls::CameraControls(Camera* parent)
	: m_Parent(parent)
{
}

TrackballControls::TrackballControls(Camera* parent)
	: CameraControls(parent)
	, m_PreviousMousePos(Input::GetMousePosition())
{
}

void TrackballControls::OnEvent(Event& ev)
{
	if (ev.Type == Event::MouseWheelScrolled)
	{
		m_Parent->Position += m_Parent->GetForwardDirection() * ev.MouseWheel.OffsetY;
		return;
	}

	if (ev.Type == Event::MouseButtonReleased &&
		(ev.MouseButton.Button == MouseButton::Middle || ev.MouseButton.Button == MouseButton::Right))
	{
		Input::ShowCursor();
		return;
	}

	if (ev.Type == Event::KeyPressed && ev.Key.Code == Key::LeftAlt)
	{
		Input::DisableCursor();
		m_Parent->m_ControlsType = std::make_unique<FpsControls>(m_Parent);
		return;
	}
}

void TrackballControls::OnUpdate(float ts)
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = mousePos - m_PreviousMousePos;
	float deltaLen = glm::length(delta);
	m_PreviousMousePos = mousePos;

	if (Input::IsMouseButtonPressed(MouseButton::Right))
	{
		Input::DisableCursor();

		m_Parent->m_Yaw   += delta.x * 0.1f;
		m_Parent->m_Pitch -= delta.y * 0.1f;
		m_Parent->m_Pitch  = glm::max(glm::min(m_Parent->m_Pitch, 90.0f), -90.0f);

		return;
	}

	if (Input::IsMouseButtonPressed(MouseButton::Middle))
	{
		Input::DisableCursor();

		m_Parent->Position -= m_Parent->GetRightDirection() * delta.x * 0.02f;
		m_Parent->Position -= m_Parent->GetUpDirection() * delta.y * 0.02f;

		return;
	}
}

FpsControls::FpsControls(Camera* parent)
	: CameraControls(parent)
	, m_PreviousMousePos(Input::GetMousePosition())
{
}

void FpsControls::OnEvent(Event& ev)
{
	if (ev.Type == Event::KeyReleased && ev.Key.Code == Key::LeftAlt)
	{
		Input::ShowCursor();
		m_Parent->m_ControlsType = std::make_unique<TrackballControls>(m_Parent);

		return;
	}
}

void FpsControls::OnUpdate(float ts)
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = mousePos - m_PreviousMousePos;
	float deltaLen = glm::length(delta);
	m_PreviousMousePos = mousePos;

	glm::vec3 moveVec(0.0f);

	if (Input::IsKeyPressed(Key::A))
	{
		moveVec -= m_Parent->GetRightDirection();
	}
	else if (Input::IsKeyPressed(Key::D))
	{
		moveVec += m_Parent->GetRightDirection();
	}

	if (Input::IsKeyPressed(Key::W))
	{
		moveVec += m_Parent->GetForwardDirection();
	}
	else if (Input::IsKeyPressed(Key::S))
	{
		moveVec -= m_Parent->GetForwardDirection();
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
		m_Parent->Position += glm::normalize(moveVec) * 15.0f * ts;
	}

	m_Parent->m_Yaw   += delta.x * 0.1f;
	m_Parent->m_Pitch -= delta.y * 0.1f;
	m_Parent->m_Pitch  = glm::max(glm::min(m_Parent->m_Pitch, 90.0f), -90.0f);
}

OrbitalControls::OrbitalControls(Camera* parent, glm::vec3* focusPoint)
	: CameraControls(parent)
	, m_PreviousMousePos(Input::GetMousePosition())
	, m_FocusPoint(focusPoint)
	, m_Distance(glm::length(*focusPoint - parent->Position))
{
}

void OrbitalControls::OnEvent(Event& ev)
{
	if (ev.Type == Event::MouseWheelScrolled)
	{
		Animations::DoFloat(m_Distance, m_Distance - ev.MouseWheel.OffsetY, 0.1f, AnimType::EaseOut);
		return;
	}
}

void OrbitalControls::OnUpdate(float ts)
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = mousePos - m_PreviousMousePos;
	float deltaLen = glm::length(delta);
	m_PreviousMousePos = mousePos;

	if (Input::IsMouseButtonPressed(MouseButton::Right))
	{
		m_Parent->m_Yaw += delta.x * 0.1f;
		m_Parent->m_Pitch -= delta.y * 0.1f;
		m_Parent->m_Pitch = glm::max(glm::min(m_Parent->m_Pitch, 90.0f), -90.0f);
	}

	glm::vec3 normalizedCameraForward = glm::normalize(m_Parent->GetForwardDirection());
	m_Parent->Position = *m_FocusPoint - normalizedCameraForward * m_Distance;
}
