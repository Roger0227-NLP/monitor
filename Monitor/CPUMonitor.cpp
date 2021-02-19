#include "CPUMonitor.h"

#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <linux/kernel.h>

CPUMonitor::AllStat CPUMonitor::_oldTotalCpuTime;
CPUMonitor::AllStat CPUMonitor::_newTotalCpuTime;

uint64_t CPUMonitor::_totalCpuTime;

std::unordered_map<pid_t, CPUMonitor::ProcessTime> CPUMonitor::_oldProcessTimes;
std::unordered_map<pid_t, CPUMonitor::ProcessTime> CPUMonitor::_newProcessTimes;
std::unordered_map<pid_t, uint64_t> CPUMonitor::_processTimeMap;

std::unordered_map<pid_t, std::vector<CPUMonitor::ThreadTime>> CPUMonitor::_newThreadTimes;
std::unordered_map<pid_t, std::vector<CPUMonitor::ThreadTime>> CPUMonitor::_oldThreadTimes;

std::unordered_map<pid_t, std::vector<CPUMonitor::ThreadTime>> CPUMonitor::_processThreadMap;

//从/proc/<Pid>/task下拿到Pid对应的tid
void CPUMonitor::GetThreadIDByPid(pid_t pid, std::vector<int>& threadID)
{
	std::string path = "proc/" + std::to_string(pid) + "/task";
	
	DIR *dir;
	dir = opendir(path.c_str());

	struct dirent *next;
	while ((next = readdir(dir)) != NULL) {
		if (!isdigit(*next->d_name))
			continue;
		if (next->d_type == S_IFDIR) {
			threadID.push_back(std::atoi(next->d_name));
		}
	}
	closedir(dir);
}

//读取/proc/<pid>/task/<tid>/stat文件
void CPUMonitor::GetNowThreadOfProcessTime(std::unordered_map<pid_t, std::vector<ThreadTime>>& threadTimes)
{
	for (auto it = threadTimes.begin(); it != threadTimes.end(); ++it) {
		pid_t pid = it->first;
		it->second.clear();
		std::string path = "/proc/";
		path += std::to_string(pid);
		path += "/task";
		DIR *dir;
		dir = opendir(path.c_str());

		struct dirent *next;
		while ((next = readdir(dir)) != NULL) {
			if (!strcmp(next->d_name, "."))
				continue;
			if (!strcmp(next->d_name, ".."))
				continue;

			if (next->d_type == DT_DIR) {
				char statPath[32] = { 0 };
				sprintf(statPath, "%s/%s/stat", path.c_str(), next->d_name);

				std::ifstream fstat(statPath);

				std::vector<std::string> words;
				std::string word;
				std::string line;

				while (std::getline(fstat, line)){
					std::istringstream instream(line);
					while (instream >> word) {
						words.push_back(word);
						word.clear();
					}
				}

				ThreadTime threadTime;
				threadTime.threadID = std::atoi(next->d_name);
				threadTime.utime = std::atol(words[13].c_str());
				threadTime.stime = std::atol(words[14].c_str());

				it->second.emplace_back(threadTime);
			}
		}
		closedir(dir);
	}

}

//从/proc/stat读取CPU的活动信息
void CPUMonitor::ReadCPUAllStat(AllStat & allCPUStat)
{
	fd.clear();
	fd.seekg(std::_S_beg);//文件指针回到开头

	std::vector<std::vector<std::string>> lines;
	std::vector<std::string> words;
	std::string word;
	std::string line;

	while (std::getline(fd, line))
	{
		std::istringstream instream(line);
		while (instream >> word) {
			words.push_back(word);
			word.clear();
		}
		lines.push_back(words);
	}
	
	allCPUStat.user = std::atol(lines[0][1].c_str());
	allCPUStat.nice = std::atol(lines[0][2].c_str());
	allCPUStat.system = std::atol(lines[0][3].c_str());
	allCPUStat.idle = std::atol(lines[0][4].c_str());
	allCPUStat.irq = std::atol(lines[0][5].c_str());
	allCPUStat.softirq = std::atol(lines[0][6].c_str());
	allCPUStat.stealstolen = std::atol(lines[0][7].c_str());
	allCPUStat.guest = std::atol(lines[0][8].c_str());

}

//从/proc/<pid>/stat获取进程活动信息
void CPUMonitor::ReadProcessStat(std::unordered_map<pid_t, ProcessTime> & processTimes)
{
	
	for (auto it = processTimes.begin(); it != processTimes.end(); ++it) {
		//？
		processDirectstats[it->second.pid].clear();
		processDirectstats[it->second.pid].seekg(std::_S_beg);

		std::vector<std::string> words;
		std::string word;
		std::string line;

		while (std::getline(processDirectstats[it->second.pid], line))
		{
			std::istringstream instream(line);
			while (instream >> word) {
				words.push_back(word);
				word.clear();
			}
		}

		it->second.utime = std::atol(words[13].c_str());
		it->second.stime = std::atol(words[14].c_str());
		it->second.cutime = std::atol(words[15].c_str());
		it->second.cstime = std::atol(words[16].c_str());
	}
}

void CPUMonitor::Calculate()
{

	GetNowThreadOfProcessTime(_newThreadTimes);
	
	ReadCPUAllStat(_newTotalCpuTime);
	ReadProcessStat(_newProcessTimes);

	
	_totalCpuTime = _newTotalCpuTime.guest - _oldTotalCpuTime.guest + _newTotalCpuTime.idle - _oldTotalCpuTime.idle + 
		_newTotalCpuTime.irq - _oldTotalCpuTime.irq + _newTotalCpuTime.nice - _oldTotalCpuTime.nice +
		_newTotalCpuTime.softirq - _oldTotalCpuTime.softirq + _newTotalCpuTime.stealstolen - _oldTotalCpuTime.stealstolen +
		_newTotalCpuTime.system - _oldTotalCpuTime.system + _newTotalCpuTime.user - _oldTotalCpuTime.user;

	for (auto it = _newProcessTimes.begin(); it != _newProcessTimes.end(); ++it) {
		_processTimeMap[it->first] = it->second.cstime - _oldProcessTimes[it->first].cstime +
			it->second.cutime - _oldProcessTimes[it->first].cutime + it->second.utime - _oldProcessTimes[it->first].utime +
			it->second.stime - _oldProcessTimes[it->first].stime;
	}

	for (auto it = _newThreadTimes.begin(); it != _newThreadTimes.end(); ++it) {
		auto tmp = _oldThreadTimes[it->first];
		_processThreadMap[it->first].clear();
		for (int i = 0; i < it->second.size(); ++i) {
			ThreadTime threadTime;
			threadTime.threadID = it->second[i].threadID;
			threadTime.stime = it->second[i].stime - tmp[i].stime;
			threadTime.utime = it->second[i].utime - tmp[i].utime;
			if (threadTime.stime + threadTime.utime > 10000) {
				printf("error 186: old  = %ld , new  = %ld ", _oldThreadTimes[it->first][i].stime+ _oldThreadTimes[it->first][i].utime, _newThreadTimes[it->first][i].stime+ _newThreadTimes[it->first][i].utime);
			}
			_processThreadMap[it->first].push_back(threadTime);
		}
	}

	std::swap(_oldTotalCpuTime, _newTotalCpuTime);
	std::swap(_oldProcessTimes, _newProcessTimes);
	std::swap(_oldThreadTimes, _newThreadTimes);

}

//由Pid从/proc/<pid>/status读取VmRSS字段
int CPUMonitor::GetMemoryByPid(pid_t p)
{
	FILE *fd;
	char name[32], line_buff[256] = { 0 }, file[64] = { 0 };
	int i, vmrss = 0;

	sprintf(file, "/proc/%d/status", p);
	// 以R读的方式打开文件再赋给指针fd
	fd = fopen(file, "r");
	if (fd == NULL)
	{
		return -1;
	}

	// 读取VmRSS这一行的数据
	for (i = 0; i < 40; i++)
	{
		if (fgets(line_buff, sizeof(line_buff), fd) == NULL)
		{
			break;
		}
		if (strstr(line_buff, "VmRSS:1") != NULL)
		{
			sscanf(line_buff, "%s %d", name, &vmrss);
			break;
		}
	}

	fclose(fd);

	return vmrss;
}

void CPUMonitor::WriteLogs()
{
	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		auto tmp = _processThreadMap[(*it).pid];
		std::string threadInfo;
		for (int i = 0; i < tmp.size(); ++i) {
			double val = 100 * _CPUInfo.cpuNumber * (double)(tmp[i].stime + tmp[i].utime) / (double)_totalCpuTime;

			if (val > 100000) {
				std::cout << "_totalCpuTime:" << _totalCpuTime << "  tmp[i].stime  =  tmp[i].utime = : " << tmp[i].stime << tmp[i].utime << std::endl;
			}
			threadInfo += std::to_string(tmp[i].threadID);
			threadInfo += " ";
			threadInfo += std::to_string(val);
			threadInfo += " ";
		}
		
		LOG_INFO("cpu_" + std::to_string((*it).pid) + "_" + (*it).processName, threadInfo);

		//printf("pid : %d _processTimeMap[(*it).pid]  = %ld _totalCpuTime = %ld\n", (*it).pid, _processTimeMap[(*it).pid], _totalCpuTime);
	}
}

void CPUMonitor::SetLogFiles(){

	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		
		SET_LOG_FILE("cpu_" + std::to_string((*it).pid)+"_" + (*it).processName);
	}
}

//初始化
void CPUMonitor::FileOpenReady()
{
	IMonitor::FileOpenReady();

	for (int i = 0; i < _config.processNames.size(); ++i) {

		pid_t pid;
		pid = _config.processNames[i].pid;

		_oldThreadTimes[pid] = std::vector<ThreadTime>();
		_newThreadTimes[pid] = std::vector<ThreadTime>();

		ProcessTime processTime;
		processTime.pid = pid;
		processTime.processName = _config.processNames[i].processName;

		_oldProcessTimes[pid] = processTime;
		_newProcessTimes[pid] = processTime;

		_processTimeMap[pid] = 0;
	}

	GetNowThreadOfProcessTime(_oldThreadTimes);
	GetNowThreadOfProcessTime(_newThreadTimes);

	ReadProcessStat(_oldProcessTimes);
	ReadProcessStat(_newProcessTimes);
}
