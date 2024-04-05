#pragma once

#include "CameraControls.hpp"

#include <glm/glm.hpp>
#include <memory>

class Camera
{
public:
	Camera();
	Camera(float fov, float aspectRatio, float nearClip, float farClip);

	void OnEvent(Event& ev);
	void OnUpdate(float ts);
	
	void LookAt(const glm::vec3& point);
	void SetControls(std::unique_ptr<CameraControls>&& controls);

	glm::vec3 GetUpDirection()		const;
	glm::vec3 GetRightDirection()	const;
	glm::vec3 GetForwardDirection() const;
	glm::quat GetOrientation()		const;

	inline const glm::mat4& GetProjection() { UpdateProjection(); return m_Projection;	}
	inline const glm::mat4& GetViewMatrix() { UpdateView();		  return m_View;		}
	inline glm::mat4 GetViewProjection()	{ return GetProjection() * GetViewMatrix(); }

	glm::vec3 Position = glm::vec3(0.0f);
	float Exposure = 1.0f;

private:
	void UpdateProjection();
	void UpdateView();

	std::unique_ptr<CameraControls> m_ControlsType;
	
	float m_FOV			= 80.0f;
	float m_AspectRatio = 16.0f / 9.0f;
	float m_NearClip	= 0.1f;
	float m_FarClip		= 1000.0f;

	float m_Pitch = 0.0f;
	float m_Yaw	  = 180.0f;

	glm::mat4 m_Projection   = glm::mat4(1.0f);
	glm::mat4 m_View	     = glm::mat4(1.0f);
	glm::vec2 m_ViewportSize = glm::vec2(1280.0f, 720.0f);
	glm::vec2 m_PrevMousePos = glm::vec2(0.0f);

	friend class EditorLayer;
	friend class TrackballControls;
	friend class FpsControls;
	friend class OrbitalControls;
};