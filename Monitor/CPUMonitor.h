#pragma once
#include "IMonitor.h"
#include "Log.h"

#include <fstream>
#include <dirent.h>

/*
读取每个pid对应的所有tid的cpu占用率

oldxxx是上一次循环读取的快照
newxxx是本次循环读取的快照

差值 = newxxx - oldxxx

占用率 = 差值/时间间隔
*/
class CPUMonitor :public IMonitor{

	
	//cpu快照
	struct AllStat {
		uint64_t user;
		uint64_t nice;
		uint64_t system;
		uint64_t idle;
		uint64_t irq;
		uint64_t softirq;
		uint64_t stealstolen;
		uint64_t guest;

		AllStat():user(0),nice(0),system(0),idle(0),irq(0),softirq(0),stealstolen(0),guest(0){}
		uint64_t operator-(const AllStat& p) {
			return user - p.user + nice - p.nice + system - p.system + idle - p.idle + softirq - p.softirq + stealstolen - p.stealstolen + guest - p.stealstolen;
		}
	};
	//进程快照
	struct ProcessTime {
		std::string processName;
		pid_t pid;
		long unsigned int utime;
		long unsigned int stime;
		long int cutime;
		long int cstime;


		ProcessTime():pid(0), processName(""),utime(0),stime(0),cutime(0),cstime(0){}
	};

	struct ThreadTime {
		long int threadID;
		long int utime;
		long int stime;

		ThreadTime() :threadID(0), stime(0), utime(0) {}
	};

	struct ProcessMemory {
		pid_t pid;
		int usedMemory;
	};

	int GetMemoryByPid(pid_t pid);

	void GetThreadIDByPid(pid_t pid, std::vector<int>& threadID);//通过pid得到tid 一对多
	void GetNowThreadOfProcessTime(std::unordered_map<pid_t, std::vector<ThreadTime>>&);//读取/proc/<pid>/task/<tid>/stat文件
	void ReadCPUAllStat(AllStat & allCPUStat);//读取/proc/stat 文件
	void ReadProcessStat(std::unordered_map<pid_t, ProcessTime>  & processTimes);//读取/proc/<pid>/stat文件
public:

	virtual void Calculate();
	virtual void WriteLogs();
	virtual void SetLogFiles();
	virtual void FileOpenReady();

	~CPUMonitor(){}

private:
	
	static AllStat _oldTotalCpuTime;
	static AllStat _newTotalCpuTime;

	static uint64_t _totalCpuTime;

	static std::unordered_map<pid_t,ProcessTime> _oldProcessTimes;
	static std::unordered_map<pid_t,ProcessTime> _newProcessTimes;

	
	static std::unordered_map<pid_t, std::vector<ThreadTime>> _oldThreadTimes;
	static std::unordered_map<pid_t, std::vector<ThreadTime>> _newThreadTimes;

	static std::unordered_map<pid_t, uint64_t> _processTimeMap;
	static std::unordered_map<pid_t, std::vector<ThreadTime>> _processThreadMap;


	
};

