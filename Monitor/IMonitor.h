#pragma once
#include "ini.h"
#include "Timer.h"

#include <iostream>
#include <string>
#include <sys/sysinfo.h>
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

/*
monitor基类
主要作用，定义了4个虚函数
virtual void Calculate() = 0;   这个函数主要用于计算，每次timer循环都会被调用
virtual void WriteLogs() = 0;	主要是记录数据到log的缓存区
这两个函数没有本质上的区别，各自都能包含对方的功能，只是为了功能划分写成了两个而已

virtual void SetLogFiles() = 0;  这个函数在线程创建时（timer循环前）调用一次用来创建log文件和log的缓冲区
virtual void FileOpenReady();    这个函数一样在线程创建时（timer循环前）调用一次 用来打开文件句柄
如果重载了FileOpenReady函数必须先调用一次基类的 FileOpenReady 并且要在析构函数中关闭你打开的文件句柄

*/

class IMonitor{
	struct ProcessInfo{
		int pid;
		std::string processName;
	};
	
	//通过配置文件和getPidByName读取的进程信息
	struct Config{
		std::vector<ProcessInfo> processNames;

		int timeInterval;

		std::string _ip;
		std::string _port;
		std::string _localip;
		Config():timeInterval(0){}
	};

	//读取本机的cpu核数
	struct CPUInfo {
		int cpuNumber;
	};


public:

	virtual void Calculate() = 0;
	//记录数据到log的缓存区
	virtual void WriteLogs() = 0;
	virtual void SetLogFiles() = 0;
	virtual void FileOpenReady();
	static void ReadConfig(std::string path);

	//这两个静态方法是用来通过进程名得到对应的pid,进程名一个但pid可以有多个
	static char * basename(const char * path);
	static int getPidByName(const char * process_name, pid_t pid_list[], int list_size);
	static std::string GetCmdLineByPid(pid_t pid);

	int GetTatalMemroy();
	static int GetTimeInterval() { return _config.timeInterval; }
	virtual ~IMonitor(){
		if (fd)
			fd.close();

		for (auto it = processDirectstats.begin(); it != processDirectstats.end(); ++it) {
			if (!it->second) {
				it->second.close();
			}
		}
	}

public:
	static Config _config;
	//std::vector<ProcessInfo> _processInfomations;

protected:
	std::unordered_map<pid_t, std::ifstream> processDirectstats; //"proc/<pid>/stat"
	std::ifstream  fd; //"proc/stat"

	static CPUInfo _CPUInfo;
	static int _tatalMomery;
	
};

