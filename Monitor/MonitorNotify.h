#pragma once
#include "IMonitor.h"
#include "Timer.h"

#include <unordered_set>
class MonitorNotify{

public:

	void ReadConfigFile(std::string path);
	void AddMonitor(IMonitor* monitor);
	void RemoveMonitor(IMonitor* monitor);
	void MonitorStart();
	void MonitorStop();
	MonitorNotify(){
	
	}
	virtual ~MonitorNotify(){}
private:

	Timer timer;
	std::unordered_set<IMonitor*> _monitors;
};

