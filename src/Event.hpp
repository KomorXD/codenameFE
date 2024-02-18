#pragma once

#include "Input.hpp"

#include <queue>

struct Event
{
	struct ResizeEvent
	{
		int32_t Width;
		int32_t Height;
	};

	struct KeyEvent
	{
		Key  Code;
		bool AltPressed;
		bool ShiftPressed;
		bool CtrlPressed;
	};

	struct MouseMoveEvent
	{
		float PosX;
		float PosY;
	};

	struct MouseButtonEvent
	{
		MouseButton Button;
	};

	struct MouseScrollEvent
	{
		float OffsetX;
		float OffsetY;
	};

	enum EventType
	{
		None,
		WindowResized,
		KeyPressed,
		KeyReleased,
		KeyHeld,
		MouseMoved,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseWheelScrolled
	};

	EventType Type = None;

	union
	{
		ResizeEvent Size;
		KeyEvent Key;
		MouseMoveEvent Mouse;
		MouseButtonEvent MouseButton;
		MouseScrollEvent MouseWheel;
	};
};

class EventQueue
{
public:
	bool PollEvents(Event& ev);
	void SubmitEvent(Event& ev);

private:
	std::queue<Event> m_Events;
};