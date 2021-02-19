#include "NetMonitor.h"


//inode ����id �б�
std::unordered_map<u_int, pid_t> NetMonitor::_processFdInodeMap;

//����port ��Ӧ�� inode
std::unordered_map<PortType, u_int> NetMonitor::_ipAndInodeMap;

std::atomic<u_int64_t>  NetMonitor::_downloadSum; //_Config.timeIntervalʱ���� ����������
std::atomic<u_int64_t>  NetMonitor::_updateSum; //_Config.timeIntervalʱ���� �ϴ������� 


std::unordered_map<pid_t, u_int> NetMonitor::_downloadOfProcessID; //���̶�Ӧ��������
std::unordered_map<pid_t, u_int> NetMonitor::_updateOfProcessID;//���̶�Ӧ���ϴ���

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

	//���� map��Ϣ ��ֹ�����ӽ���û��ͳ�Ƶ�
	
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

	IMonitor::FileOpenReady();//������ִ��

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

	std::thread t(GetSumOfNetStream);//����ץ���߳�
	//detach()�������ǽ����̺߳����̵߳Ĺ������룬Ҳ����˵detach()�����߳��ں�̨�����������У�
	//���߳��޷���ȡ�����̵߳Ŀ���Ȩ����ʹ���߳̽��������߳�δִ��Ҳ���������
	t.detach();

}

//�ַ���ת����16 ->10,ʮ�������ַ���תʮ��������
int NetMonitor::transfer(char x)
{
	int y = 0;//����ֵ
	if (x >= '0' && x <= '9')//0-9������
	{
		y = x - '0';
		return y;
	}
	if (x >= 'a' && x <= 'f')//a-f����ĸ
	{
		y = x - 'a' + 10;
		return y;
	}
	if (x >= 'A' && x <= 'F')//A-F����ĸ
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

	for (int i = 1; i < words.size(); ++i) { // ��һ��Ϊ���ƣ�������һ��
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

	for (int i = 1; i < words.size(); ++i) { // ������һ��
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

	//readdirÿ�ζ���������һ���ļ�������ָ��dirent�Ľṹ��
	while ((next = readdir(dir)) != NULL) {
		if (!isdigit(*next->d_name))
			continue;

		char fdname[128] = { 0 };
		sprintf(fdname, "/proc/%d/fd/%s", pid, next->d_name);
		char link[32] = { 0 };

		//������path�ķ����������ݴ洢������buf��ָ���ڴ�ռ䣬ִ�гɹ��򷵻��ַ������ַ�����ʧ�ܷ���-1
		int res = readlink(fdname, link, sizeof(link));
		if (res < 0) {
			std::cout << "readlink:cant find fdname" << fdname << std::endl;
			continue;
		}
		links.push_back(link);
	}
	
	int inode = 0;
	for (auto s : links) {
		//����ֵ����0��ʾ���ַ������
		if (!strncmp(s.c_str(), "socket:[", 8)) {
			std::string node(s.begin() + 8, s.end() - 1);

			inode = std::atoi(node.c_str());

			_processFdInodeMap[inode] = pid;
		}
	}

	closedir(dir);
}

//ץ���߳��������� ����һ��ʱ���������ֽ������ܺ�
void NetMonitor::GetSumOfNetStream()
{
	char *device;
	char errBuf[PCAP_ERRBUF_SIZE];
	pcap_t *handle;

	//���ڲ��������豸�����ؿɱ�pcap_open_live()�������õ������豸��ָ��
	device = pcap_lookupdev(errBuf);
	if (device){
		printf("lookup %s\n", device);
	}
	else{
		printf("lookup is error %s\n", errBuf);
		return;
	}
	//pcap_open_live���ڻ�ȡ���ݰ������������Բ鿴�����ϵ����ݰ�
	handle = pcap_open_live(device, 65535, 0, 0, errBuf);
	if (handle){
		printf("open %s\n", device);
	}
	else{
		printf("open is error %s\n", errBuf);
		return;
	}

	char filter_app[] = "tcp or udp";/* ���˱��ʽ*/
	bpf_u_int32 net; /* ִ����̽���豸��IP��ַ */
	bpf_u_int32 mask; /* ִ����̽���豸���������� */
	struct bpf_program filter;

	//���ڻ�ȡ����������ź���������
	pcap_lookupnet(device, &net, &mask, errBuf);
	/* �Էǻ���ģʽ�򿪻Ự */
	handle = pcap_open_live(device, BUFSIZ, 0, 0, errBuf);
	/* ���벢Ӧ�ù����� */
	pcap_compile(handle, &filter, filter_app, 0, net);
	pcap_setfilter(handle, &filter);

	// typedef void (*pcap_handler)(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes);
	// int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);

	int i = 0;
	//-1��ʾ����
	pcap_loop(handle, -1, GetPacket, (u_char *)&i);

	pcap_close(handle);
}

//����Ŀ��˿ڣ�Դ�˿ڣ�Э��ţ�����С,packet��ƫ����
void NetMonitor::GetPacket(u_char * user, const pcap_pkthdr * pkthdr, const u_char * packet)
{
	int * id = (int *)user;
	*id += 1;
	struct in_addr addr;
	struct iphdr *ipptr; //ip ͷ��
	struct tcphdr *tcpptr;//tcp���ݽṹ
	struct udphdr *udpptr; //udpͷ��
	struct ether_header *eptr;//��̫����ͷ

	//useful
	u_char *ptr;
	char *data;

	ipptr = (struct iphdr*)(packet + sizeof(struct ether_header));//�õ�ip��ͷ

	addr.s_addr = ipptr->daddr;
	//printf("Destination IP: %s\n", inet_ntoa(addr));
	addr.s_addr = ipptr->saddr;
	//printf("Source IP: %s\n", inet_ntoa(addr));
	bpf_u_int32 dateLenth = 0;

	__be16 ssrc;//Դ�˿�
	__be16 sdst;//Ŀ��˿�

	if (ipptr->protocol == 6) { //tcp
		tcpptr = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));//�õ�TCPͷ��ָ��
		dateLenth = pkthdr->len - sizeof(struct ether_header) - sizeof(struct iphdr) - sizeof(struct tcphdr);

		ssrc = tcpptr->source;
		sdst = tcpptr->dest;
	}
	if (ipptr->protocol == 17) { // udp
		udpptr = (struct udphdr*)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));//�õ�UDPͷ��ָ��
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

	//Epoch������struct timevalʱ��������tv_usecΪ΢����������������ͷ
	_newTime = pkthdr->ts.tv_sec * 1000 + pkthdr->ts.tv_usec / 1000;
}

