#include "Timer.h"

void Timer::setTimeout(std::function<void()> function, int delay) {
	this->clear = false;
	std::thread t([=]() {
		if (this->clear) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		if (this->clear) return;
		function();
	});
	t.detach();
}

void Timer::setInterval(std::function<void()> function, int interval) {
	this->clear = false;
	std::thread t([=]() {
		while (true) {
			if (this->clear) return;
			//Timer调用一次setInterval 会开启一个线程每隔interval调用一次function
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			if (this->clear) return;
			function();
		}
	});
	t.detach();
}

void Timer::stop() {
	this->clear = true;
}