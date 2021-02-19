#pragma once
#include "IMonitor.h"
#include "Timer.h"
#include "Log.h"

#include <unordered_map>
class MemoryMonitor : public IMonitor
{
public:
	virtual void Calculate();
	virtual void WriteLogs();
	virtual void SetLogFiles();

private:
	static std::unordered_map<pid_t, uint64_t> _processMemoryInfo;
};

