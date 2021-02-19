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
monitor����
��Ҫ���ã�������4���麯��
virtual void Calculate() = 0;   ���������Ҫ���ڼ��㣬ÿ��timerѭ�����ᱻ����
virtual void WriteLogs() = 0;	��Ҫ�Ǽ�¼���ݵ�log�Ļ�����
����������û�б����ϵ����𣬸��Զ��ܰ����Է��Ĺ��ܣ�ֻ��Ϊ�˹��ܻ���д������������

virtual void SetLogFiles() = 0;  ����������̴߳���ʱ��timerѭ��ǰ������һ����������log�ļ���log�Ļ�����
virtual void FileOpenReady();    �������һ�����̴߳���ʱ��timerѭ��ǰ������һ�� �������ļ����
���������FileOpenReady���������ȵ���һ�λ���� FileOpenReady ����Ҫ�����������йر���򿪵��ļ����

*/

class IMonitor{
	struct ProcessInfo{
		int pid;
		std::string processName;
	};
	
	//ͨ�������ļ���getPidByName��ȡ�Ľ�����Ϣ
	struct Config{
		std::vector<ProcessInfo> processNames;

		int timeInterval;

		std::string _ip;
		std::string _port;
		std::string _localip;
		Config():timeInterval(0){}
	};

	//��ȡ������cpu����
	struct CPUInfo {
		int cpuNumber;
	};


public:

	virtual void Calculate() = 0;
	//��¼���ݵ�log�Ļ�����
	virtual void WriteLogs() = 0;
	virtual void SetLogFiles() = 0;
	virtual void FileOpenReady();
	static void ReadConfig(std::string path);

	//��������̬����������ͨ���������õ���Ӧ��pid,������һ����pid�����ж��
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

