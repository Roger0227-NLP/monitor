#pragma once

#include "IMonitor.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <queue>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unordered_map>
#include <unordered_set>


#include <curl/curl.h>

#define MESSAGE_MAX_NUM 4

struct  Message {
	std::string _filename;
	int  _line;
	int _level;
	std::string _info;
	time_t _time;

	Message():_filename(""), _line(0),_level(0),_info(""), _time(0){}
};

class LoggerRef {
public:
	LoggerRef();
	~LoggerRef();
};

class Logger {
	friend class LoggerRef;
private:
	static Logger* _instance;
	Logger() {
		//_message_max_num = 32;//最大缓存数
	}

	
	Message _TempMessage; //单个信息缓存
	std::unordered_map<std::string,std::queue<Message>> _messageList;//信息缓存表
	void _add_message(std::string index, Message msg);//添加信息到缓存表
	//int _message_max_num;//最大缓存数
	pthread_mutex_t _swap_lock;//交换区锁
	pthread_mutexattr_t mutexattr;

	void lock_init();
public:
	void LogInit();
	void DirInit();
	void ClearBufToFile();
	void SetFilename(std::string dir);
	void Flush_Messagebuff(std::string str,std::queue<Message>&&);
	static Logger* getInstance();
	//ignore
	Logger& setLinesAndLevel(const char* name, int line, int level);

	Logger& WriteLog(std::string index, std::string str);

};

//#define LOG_DEBUG  ((*Logger::getInstance()).setLinesAndLevel(__FILE__,__LINE__,0))
#define LOG_INFO(dir,str)   ((*Logger::getInstance()).setLinesAndLevel(__FILE__,__LINE__,1)).WriteLog(dir,str)
#define SET_LOG_FILE(str)   (Logger::getInstance()->SetFilename(str));
//#define LOG_WORING ((*Logger::getInstance()).setLinesAndLevel(__FILE__,__LINE__,3))
