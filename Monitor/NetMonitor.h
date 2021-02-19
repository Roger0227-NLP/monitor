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
����ʵ�ּ�ppt
*/
class NetMonitor :
	public IMonitor
{
	
private:
	int transfer(char x); //�ַ���ת����16 ->10
	std::string ltot(std::string ip_16); //�ַ��� 16����ipת10����ip��ַ 

	void OpenAndReadProcNetToMap(); // ��ȡ /proc/net/[tcp][udp]��map�б�
	void ReadProcessInodeToMap(pid_t pid); //��ȡ /proc/pid/fd/�µ�����inode

	static void GetSumOfNetStream();//ץ���߳��������� ����һ��ʱ���������ֽ������ܺ�
	static void GetPacket(u_char *user, const struct pcap_pkthdr *pkthdr, const u_char *packet);//curl�Ļص�����
public:
	virtual void Calculate();
	virtual void WriteLogs();
	virtual void SetLogFiles();
	virtual void FileOpenReady();

	
private:
	
	static std::mutex lock;
	//inode ����id �б�
	static std::unordered_map<u_int, pid_t> _processFdInodeMap;

	//����port ��Ӧ�� inode
	static std::unordered_map<PortType, u_int> _ipAndInodeMap;

	static std::atomic<u_int64_t> _downloadSum; //_Config.timeIntervalʱ���� ����������
	static std::atomic<u_int64_t>  _updateSum; //_Config.timeIntervalʱ���� �ϴ������� 

	static std::unordered_map<pid_t, u_int> _downloadOfProcessID; //���̶�Ӧ��������
	static std::unordered_map<pid_t, u_int> _updateOfProcessID;//���̶�Ӧ���ϴ���

	static u_int64_t _oldTime;
	static u_int64_t _newTime;
};

