#pragma once

#include "../Event.hpp"
#include <glm/glm.hpp>

class Camera;

class CameraControls
{
public:
	CameraControls(Camera* parent);
	virtual ~CameraControls() = default;

	virtual void OnEvent(Event& ev) = 0;
	virtual void OnUpdate(float ts) = 0;

protected:
	Camera* m_Parent = nullptr;
};

class TrackballControls : public CameraControls
{
public:
	TrackballControls(Camera* parent);
	virtual ~TrackballControls() = default;

	virtual void OnEvent(Event& ev) override;
	virtual void OnUpdate(float ts) override;

private:
	glm::vec2 m_PreviousMousePos{};
};

class FpsControls : public CameraControls
{
public:
	FpsControls(Camera* parent);
	virtual ~FpsControls() = default;

	virtual void OnEvent(Event& ev) override;
	virtual void OnUpdate(float ts) override;

private:
	glm::vec2 m_PreviousMousePos{};
};

class OrbitalControls : public CameraControls
{
public:
	OrbitalControls(Camera* parent, glm::vec3* focusPoint);
	virtual ~OrbitalControls() = default;

	virtual void OnEvent(Event& ev) override;
	virtual void OnUpdate(float ts) override;

private:
	float m_Distance = 3.0f;
	glm::vec2 m_PreviousMousePos{};
	glm::vec3* m_FocusPoint = nullptr;
};