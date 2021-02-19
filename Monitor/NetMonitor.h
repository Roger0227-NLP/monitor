#pragma once
#include "IMonitor.h"
#include "Log.h"

#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <thread>
#include <pcap.h>
using SrcIpAndDstIpType = std::unordered_set<std::string>;

using PortType = u_int;

/*
具体实现见ppt
*/
class NetMonitor :
	public IMonitor
{
	
private:
	int transfer(char x); //字符串转数字16 ->10
	std::string ltot(std::string ip_16); //字符串 16进制ip转10进制ip地址 

	void OpenAndReadProcNetToMap(); // 读取 /proc/net/[tcp][udp]到map列表
	void ReadProcessInodeToMap(pid_t pid); //读取 /proc/pid/fd/下的所有inode

	static void GetSumOfNetStream();//抓包线程启动函数 计算一定时间内网络字节流的总和
	static void GetPacket(u_char *user, const struct pcap_pkthdr *pkthdr, const u_char *packet);//curl的回调函数
public:
	virtual void Calculate();
	virtual void WriteLogs();
	virtual void SetLogFiles();
	virtual void FileOpenReady();

	
private:
	
	static std::mutex lock;
	//inode 进程id 列表
	static std::unordered_map<u_int, pid_t> _processFdInodeMap;

	//本地port 对应的 inode
	static std::unordered_map<PortType, u_int> _ipAndInodeMap;

	static std::atomic<u_int64_t> _downloadSum; //_Config.timeInterval时间内 下载总流量
	static std::atomic<u_int64_t>  _updateSum; //_Config.timeInterval时间内 上传总流量 

	static std::unordered_map<pid_t, u_int> _downloadOfProcessID; //进程对应的下载量
	static std::unordered_map<pid_t, u_int> _updateOfProcessID;//进程对应的上传量

	static u_int64_t _oldTime;
	static u_int64_t _newTime;
};

