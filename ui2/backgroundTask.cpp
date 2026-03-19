#include "backgroundTask.h"
#include "window.h"
bool BackgroundTask::running() const { return !m_done && m_thread.joinable(); }
void BackgroundTask::start(Window& window, std::function<void()> task, std::function<void()> callback)
{
	assert(!running());
	window.m_lockInput = true;
	window.m_cursor = ImGuiMouseCursor_Wait;
	m_done = false;
	m_callback = std::move(callback);
	m_thread = std::jthread(
	[this, task = std::move(task)](std::stop_token)
	{
		try
		{
			task();
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
		m_done = true;
	});
}

void BackgroundTask::update(Window& window)
{
	if(m_done && m_thread.joinable())
	{
		m_thread.join();
		if(m_callback != nullptr)
		{
			auto cb = std::move(m_callback);
			m_callback = nullptr;
			cb();
			window.m_lockInput = false;
			window.m_cursor = ImGuiMouseCursor_Arrow;
		}
	}
}