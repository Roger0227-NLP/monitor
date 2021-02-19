#pragma once
/*
	setInterval ������һ���߳� interval���� ����һ��function
	setTimeoutֻ������Դ�
	stop����ֹͣ���м�ʱ��
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

