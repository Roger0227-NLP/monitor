#include "NetMonitor.h"


//inode 进程id 列表
std::unordered_map<u_int, pid_t> NetMonitor::_processFdInodeMap;

//本地port 对应的 inode
std::unordered_map<PortType, u_int> NetMonitor::_ipAndInodeMap;

std::atomic<u_int64_t>  NetMonitor::_downloadSum; //_Config.timeInterval时间内 下载总流量
std::atomic<u_int64_t>  NetMonitor::_updateSum; //_Config.timeInterval时间内 上传总流量 


std::unordered_map<pid_t, u_int> NetMonitor::_downloadOfProcessID; //进程对应的下载量
std::unordered_map<pid_t, u_int> NetMonitor::_updateOfProcessID;//进程对应的上传量

u_int64_t NetMonitor::_oldTime;
u_int64_t NetMonitor::_newTime;

std::mutex NetMonitor::lock;

void NetMonitor::Calculate(){

	//double seconds = (double)(_newTime - _oldTime) / (double)1000;

	double dlspeedKb = 1000 * (double)_downloadSum / 1024 / _config.timeInterval;///seconds;

	double upspeedKb = 1000 * (double)_updateSum / 1024 / _config.timeInterval;//seconds;

	//std::cout << /*"second:" << seconds << */" dl:" << dlspeedKb << " up" << upspeedKb << std::endl;

	_downloadSum = 0;
	_updateSum = 0;

	_oldTime = _newTime;
	
	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		/*
		std::cout << "processname : " << it->processName << " up: " << (double)_updateOfProcessID[it->pid] * 1000 / 1024 / _config.timeInterval
			<< " down: " << (double)_downloadOfProcessID[it->pid] * 1000 / 1024 / _config.timeInterval << std::endl;
		*/
		LOG_INFO("net_" + std::to_string(it->pid) + "_" + it->processName,
			std::to_string((double)_updateOfProcessID[it->pid] * 1000 / 1024 / _config.timeInterval) + " " +
			std::to_string((double)_downloadOfProcessID[it->pid] * 1000 / 1024 / _config.timeInterval)
		);
		
	}
}

void NetMonitor::WriteLogs()
{

	//更新 map信息 防止新连接建立没有统计到
	
	std::lock_guard<std::mutex> lockGuard(lock);
	OpenAndReadProcNetToMap();

	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		_updateOfProcessID[it->pid] = 0;
		_downloadOfProcessID[it->pid] = 0;
		ReadProcessInodeToMap(it->pid);
	}
	
}

void NetMonitor::SetLogFiles()
{
	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		SET_LOG_FILE("net_" + std::to_string(it->pid) + "_" + it->processName);
	}
}

void NetMonitor::FileOpenReady()
{
	
	_newTime = 0;
	_oldTime = 0;

	_downloadSum = 0;
	_updateSum = 0;

	IMonitor::FileOpenReady();//基类先执行

	OpenAndReadProcNetToMap();
	
	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		_downloadOfProcessID[it->pid] = 0;
		_updateOfProcessID[it->pid] = 0;
		ReadProcessInodeToMap(it->pid);
	}
	
	/*
	for (auto it = _ipAndInodeMap.begin(); it != _ipAndInodeMap.end(); ++it) {
		std::cout << "local port : " << it->first << " inode : " << it->second << std::endl;
	}

	for (auto it = _processFdInodeMap.begin(); it != _processFdInodeMap.end(); ++it) {
		std::cout << "inode  : " << it->first << " pid : " << it->second << std::endl;
	}
	
	*/

	std::thread t(GetSumOfNetStream);//开启抓包线程
	//detach()的作用是将子线程和主线程的关联分离，也就是说detach()后子线程在后台独立继续运行，
	//主线程无法再取得子线程的控制权，即使主线程结束，子线程未执行也不会结束。
	t.detach();

}

//字符串转数字16 ->10,十六进制字符串转十进制数字
int NetMonitor::transfer(char x)
{
	int y = 0;//返回值
	if (x >= '0' && x <= '9')//0-9的数字
	{
		y = x - '0';
		return y;
	}
	if (x >= 'a' && x <= 'f')//a-f的字母
	{
		y = x - 'a' + 10;
		return y;
	}
	if (x >= 'A' && x <= 'F')//A-F的字母
	{
		y = x - 'A' + 10;
		return y;
	}
	printf("args error!");
	exit(1);
}

std::string NetMonitor::ltot(std::string ip_16)
{
	std::string res;
	for (int i = 0; i < 8; i += 2) {
		int y0 = transfer(ip_16[i]) * 16 + transfer(ip_16[i + 1]);
		res += std::to_string(y0);
		if (i + 2 < 8)
			res += ".";
	}
	return res;
}

//u
void NetMonitor::OpenAndReadProcNetToMap()
{
	const char * proceNetTcpPath = "/proc/net/tcp";

	std::ifstream tcpfd(proceNetTcpPath);

	std::vector<std::vector<std::string>> words;
	std::vector<std::string> wordsofline;
	std::string word;
	std::string line;

	while (std::getline(tcpfd, line))
	{
		std::istringstream instream(line);
		while (instream >> word) {
			wordsofline.push_back(word);
			word.clear();
		}
		words.push_back(wordsofline);
		wordsofline.clear();

	}

	for (int i = 1; i < words.size(); ++i) { // 第一行为名称，跳过第一行
		std::string localAddress;
		std::string remAddress;

		localAddress = words[i][1];
		remAddress = words[i][2];

		std::string lport = localAddress.substr(11, 2) + localAddress.substr(9, 2);


		//int number = (int)strtol(lport.c_str(), NULL, 16);

		u_int port;
		sscanf(lport.c_str(), "%x", &port);
		u_int inode = std::atoi(words[i][9].c_str());

		_ipAndInodeMap[port] = inode;
	}

	tcpfd.close();

	words.clear();

	proceNetTcpPath = "/proc/net/udp";

	std::ifstream udpfd(proceNetTcpPath);

	while (std::getline(udpfd, line))
	{
		std::istringstream instream(line);
		while (instream >> word) {
			wordsofline.push_back(word);
			word.clear();
		}
		words.push_back(wordsofline);
		wordsofline.clear();

	}

	for (int i = 1; i < words.size(); ++i) { // 跳过第一行
		std::string localAddress;
		std::string remAddress;

		localAddress = words[i][1];
		remAddress = words[i][2];

		std::string lport = localAddress.substr(11, 2) + localAddress.substr(9, 2);

		PortType port;
		sscanf(lport.c_str(), "%x", &port);
		u_int inode = std::atoi(words[i][9].c_str());

		_ipAndInodeMap[port] = inode;
	}

	udpfd.close();
}

//u
void NetMonitor::ReadProcessInodeToMap(pid_t pid)
{
	char path[128] = { 0 };

	sprintf(path, "/proc/%d/fd", pid);

	std::vector<std::string> links;

	DIR* dir;
	dir = opendir(path);
	struct dirent *next;

	//readdir每次读都读到下一个文件，返回指向dirent的结构体
	while ((next = readdir(dir)) != NULL) {
		if (!isdigit(*next->d_name))
			continue;

		char fdname[128] = { 0 };
		sprintf(fdname, "/proc/%d/fd/%s", pid, next->d_name);
		char link[32] = { 0 };

		//将参数path的符号链接内容存储到参数buf所指的内存空间，执行成功则返回字符串的字符数，失败返回-1
		int res = readlink(fdname, link, sizeof(link));
		if (res < 0) {
			std::cout << "readlink:cant find fdname" << fdname << std::endl;
			continue;
		}
		links.push_back(link);
	}
	
	int inode = 0;
	for (auto s : links) {
		//返回值等于0表示两字符串相等
		if (!strncmp(s.c_str(), "socket:[", 8)) {
			std::string node(s.begin() + 8, s.end() - 1);

			inode = std::atoi(node.c_str());

			_processFdInodeMap[inode] = pid;
		}
	}

	closedir(dir);
}

//抓包线程启动函数 计算一定时间内网络字节流的总和
void NetMonitor::GetSumOfNetStream()
{
	char *device;
	char errBuf[PCAP_ERRBUF_SIZE];
	pcap_t *handle;

	//用于查找网络设备，返回可被pcap_open_live()函数调用的网络设备名指针
	device = pcap_lookupdev(errBuf);
	if (device){
		printf("lookup %s\n", device);
	}
	else{
		printf("lookup is error %s\n", errBuf);
		return;
	}
	//pcap_open_live用于获取数据包捕获描述符以查看网络上的数据包
	handle = pcap_open_live(device, 65535, 0, 0, errBuf);
	if (handle){
		printf("open %s\n", device);
	}
	else{
		printf("open is error %s\n", errBuf);
		return;
	}

	char filter_app[] = "tcp or udp";/* 过滤表达式*/
	bpf_u_int32 net; /* 执行嗅探的设备的IP地址 */
	bpf_u_int32 mask; /* 执行嗅探的设备的网络掩码 */
	struct bpf_program filter;

	//用于获取网卡的网络号和子网掩码
	pcap_lookupnet(device, &net, &mask, errBuf);
	/* 以非混杂模式打开会话 */
	handle = pcap_open_live(device, BUFSIZ, 0, 0, errBuf);
	/* 编译并应用过滤器 */
	pcap_compile(handle, &filter, filter_app, 0, net);
	pcap_setfilter(handle, &filter);

	// typedef void (*pcap_handler)(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes);
	// int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);

	int i = 0;
	//-1表示阻塞
	pcap_loop(handle, -1, GetPacket, (u_char *)&i);

	pcap_close(handle);
}

//解析目标端口，源端口，协议号，包大小,packet是偏移量
void NetMonitor::GetPacket(u_char * user, const pcap_pkthdr * pkthdr, const u_char * packet)
{
	int * id = (int *)user;
	*id += 1;
	struct in_addr addr;
	struct iphdr *ipptr; //ip 头部
	struct tcphdr *tcpptr;//tcp数据结构
	struct udphdr *udpptr; //udp头部
	struct ether_header *eptr;//以太网字头

	//useful
	u_char *ptr;
	char *data;

	ipptr = (struct iphdr*)(packet + sizeof(struct ether_header));//得到ip包头

	addr.s_addr = ipptr->daddr;
	//printf("Destination IP: %s\n", inet_ntoa(addr));
	addr.s_addr = ipptr->saddr;
	//printf("Source IP: %s\n", inet_ntoa(addr));
	bpf_u_int32 dateLenth = 0;

	__be16 ssrc;//源端口
	__be16 sdst;//目标端口

	if (ipptr->protocol == 6) { //tcp
		tcpptr = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));//得到TCP头部指针
		dateLenth = pkthdr->len - sizeof(struct ether_header) - sizeof(struct iphdr) - sizeof(struct tcphdr);

		ssrc = tcpptr->source;
		sdst = tcpptr->dest;
	}
	if (ipptr->protocol == 17) { // udp
		udpptr = (struct udphdr*)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));//得到UDP头部指针
		dateLenth = pkthdr->len - sizeof(struct ether_header) - sizeof(struct iphdr) - sizeof(struct udphdr);

		ssrc = udpptr->source;
		sdst = udpptr->dest;
	}

	std::lock_guard<std::mutex> lockGuard(lock);

	if (_ipAndInodeMap.find((u_int)ssrc) != _ipAndInodeMap.end()) {
		auto it = _ipAndInodeMap.find((u_int)ssrc);
		auto pidit = _processFdInodeMap.find(it->second);
		if (pidit != _processFdInodeMap.end()) {
			//std::cout << _updateSum << std::endl;
			_updateOfProcessID[pidit->second] += dateLenth;
			_updateSum += dateLenth;
		}
	}
	
	if (_ipAndInodeMap.find((u_int)sdst) != _ipAndInodeMap.end()) {
		auto it = _ipAndInodeMap.find((u_int)sdst);
		auto pidit = _processFdInodeMap.find(it->second);
		if (pidit != _processFdInodeMap.end()) {
			_downloadOfProcessID[pidit->second] += dateLenth;
			//std::cout << _downloadSum << std::endl;
			_downloadSum += dateLenth;
		}
	}

	//Epoch到创建struct timeval时的秒数，tv_usec为微秒数，即秒后面的零头
	_newTime = pkthdr->ts.tv_sec * 1000 + pkthdr->ts.tv_usec / 1000;
}

