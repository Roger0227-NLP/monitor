#pragma once
/*
	setInterval 会启动一个线程 interval毫秒 调用一次function
	setTimeout只会调用以此
	stop立刻停止所有计时器
*/
#include <functional>
#include <chrono>
#include <thread>
class Timer {
	bool clear = false;

public:
	void setTimeout(std::function<void()> function, int delay);
	void setInterval(std::function<void()> function, int interval);
	void stop();

};

