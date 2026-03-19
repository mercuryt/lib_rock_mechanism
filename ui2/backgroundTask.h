#pragma once
#include <functional>
#include <thread>
class Window;
class BackgroundTask
{
	std::jthread m_thread;
	std::function<void()> m_callback;
	std::atomic<bool> m_done = false;
public:
	void update(Window& window);
	void start(Window& window, std::function<void()> task, std::function<void()> callback = nullptr);
	[[nodiscard]] bool running() const;
};