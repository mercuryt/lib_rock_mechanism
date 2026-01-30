#include "backgroundTask.h"
#include "window.h"
void WindowHasBackgroundTask::onFrame()
{
	if(!m_inProgress)
		return;
	if(m_taskFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
	{
		m_taskFuture.get();
		m_window.m_lockInput = false;
		m_window.setCursor(sf::Cursor::Arrow);
		auto callback = std::move(m_callback);
		m_callback = nullptr;
		m_inProgress = false;
		m_window.m_lockInput = false;
		callback();
	}
}
void WindowHasBackgroundTask::create(std::function<void()>&& action, std::function<void()>&& callback)
{
	assert(!m_inProgress);
	m_inProgress = true;
	m_window.m_lockInput = true;
	m_window.setCursor(sf::Cursor::Wait);
	m_callback = std::move(callback);
	m_taskFuture = std::async(std::launch::async, [action = std::move(action)] {
		action();
	});
}