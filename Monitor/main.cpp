#include "ini.h"
#include "Timer.h"
#include "Log.h"
#include "IMonitor.h"
#include "MonitorNotify.h"
#include "CPUMonitor.h"
#include "MemoryMonitor.h"
#include "NetMonitor.h"

#include <iostream>
#include <mutex>


int main()
{
	//define monitors
	CPUMonitor cpumonitor;
	MemoryMonitor memoryMonitor;
	NetMonitor netMonitor;

	//read configuration
	MonitorNotify monitorNotify;
	monitorNotify.ReadConfigFile("infomation.ini");

	//add monitors
	monitorNotify.AddMonitor(&cpumonitor);
	monitorNotify.AddMonitor(&memoryMonitor);
	monitorNotify.AddMonitor(&netMonitor);

	//start
	monitorNotify.MonitorStart();

	std::mutex lock;
	lock.lock();
	lock.lock();
	//std::this_thread::sleep_for(std::chrono::microseconds(0xffff));
    return 0;
}
