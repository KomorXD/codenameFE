#include "Event.hpp"

bool EventQueue::PollEvents(Event& ev)
{
	if (m_Events.empty())
	{
		return false;
	}

	ev = m_Events.front();
	m_Events.pop();

	return true;
}

void EventQueue::SubmitEvent(Event& ev)
{
	m_Events.push(ev);
}