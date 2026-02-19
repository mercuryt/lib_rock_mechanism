#pragma once
#include <functional>
#include <future>

class Window;

class WindowHasBackgroundTask
{
	std::future<void> m_taskFuture;
	std::function<void()> m_callback;
	Window& m_window;
	bool m_inProgress = false;
public:
	WindowHasBackgroundTask(Window& window) : m_window(window) { }
	void onFrame();
	void create(std::function<void()>&& action, std::function<void()>&& callback = nullptr);
	[[nodiscard]] bool hasTask() const { return m_inProgress; }
};