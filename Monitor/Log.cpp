#include "Log.h"

#include <thread>
#include <mutex>
std::mutex g_lock;

Logger* Logger::_instance = NULL;

//����RAII����Դ��ȡ����ʼ��������ʼʱ��main����ǰִ�еģ�main֮������
LoggerRef init;

std::vector<std::string> split(const std::string &s, const std::string &seperator) {
	std::vector<std::string> result;
	typedef std::string::size_type string_size;
	string_size i = 0;

	while (i != s.size()) {
		//�ҵ��ַ������׸������ڷָ�������ĸ��
		int flag = 0;
		while (i != s.size() && flag == 0) {
			flag = 1;
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[i] == seperator[x]) {
					++i;
					flag = 0;
					break;
				}
		}

		//�ҵ���һ���ָ������������ָ���֮����ַ���ȡ����
		flag = 0;
		string_size j = i;
		while (j != s.size() && flag == 0) {
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[j] == seperator[x]) {
					flag = 1;
					break;
				}
			if (flag == 0)
				++j;
		}
		if (i != j) {
			result.push_back(s.substr(i, j - i));
			i = j;
		}
	}
	return result;
}
Logger* Logger::getInstance() {
	if (_instance == NULL)
	{
		_instance = new Logger();
	}

	return _instance;
}

//ignore
Logger& Logger::setLinesAndLevel(const char* name, int line, int level) {
	_TempMessage._filename.clear();
	_TempMessage._filename = name;
	_TempMessage._line = line;
	_TempMessage._level = level;
	return *_instance;
}

Logger& Logger::WriteLog(std::string index, std::string str) {
	//д�����̻߳����
	pthread_mutex_lock(&_swap_lock);
	_TempMessage._time = time(NULL);//��õ�ǰʱ��
	//_TempMessage._info.clear();
	_TempMessage._info = str;
	_add_message(index, _TempMessage);//���뻺��
	pthread_mutex_unlock(&_swap_lock);
	return *_instance;
}

void  Logger::_add_message(std::string index, Message msg) {

	if (_messageList.find(index) == _messageList.end())
		return;
	_messageList[index].push(msg);//���뻺��

	if (_messageList[index].size() >= MESSAGE_MAX_NUM)
	{

		std::queue<Message> tmp;
		std::swap(tmp, _messageList[index]);
		std::thread t(&Logger::Flush_Messagebuff, this, index, std::move(tmp));
		t.detach();

	}
}

//ignore
const char* level_change(int n) {

	static const char level0[] = { "DEBUG" };
	static const char level1[] = { "INFO" };
	static const char level2[] = { "\033[0;31mERROR\033[0m" };
	static const char level3[] = { "\033[0;35mWARRING\033[0m" };

	switch (n) {
	case 0:return level0;
	case 1:return level1;
	case 2:return level2;
	case 3:return level3;
	}
	return NULL;
}

void  Logger::Flush_Messagebuff(std::string str, std::queue<Message>&& messageList) {

	std::lock_guard<std::mutex> loker(g_lock);

	CURL *multi_curl = curl_multi_init();
	int still_running = 0;

	time_t now = time(0);

	char filename[32] = { 0 };
	sprintf(filename, "%s", str.c_str());

	auto ret = split(filename, "_");
	std::string methodname = ret[0];
	std::string pid = ret[1];
	std::string processname = ret[2];

	curl_httppost *formpost = NULL;
	curl_httppost *lastptr = NULL;
	std::string strResponse;

	char url[1024] = { 0 };

	sprintf(url, "http://%s:%s/%s", IMonitor::_config._ip.c_str(), IMonitor::_config._port.c_str(), (methodname + "?pid=" + pid + "&processname=" + processname + "&ip=" + IMonitor::_config._localip).c_str());

	CURLMcode res;
	CURL *curl = curl_easy_init();
	struct curl_slist* headerlist = NULL;
	curl_easy_setopt(curl, CURLOPT_URL, url);//����url

	std::string values;

	for (; messageList.size();) {

		Message temp = messageList.front();

		tm* a = gmtime(&temp._time);
		char logs[1024] = { 0 };
		sprintf(logs, "%d-%02d-%02d %02d:%02d:%02d %s*", a->tm_year + 1900, a->tm_mon + 1, a->tm_mday, a->tm_hour + 8, a->tm_min, a->tm_sec, temp._info.c_str());
		values += logs;
		messageList.pop();
	}
	//std::cout << values << std::endl;
	//�Ľ�����jsoin��ʽ����
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "values",
		CURLFORM_COPYCONTENTS, values.c_str(), CURLFORM_END);

	// ����post�ύ��ʽ
	curl_easy_setopt(curl, CURLOPT_POST, 1);

	//���ñ�
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&strResponse);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

	//��multi������easy���
	curl_multi_add_handle(multi_curl, curl);

	//����
	//res = curl_easy_perform(curl);
	//res = curl_multi_perform(multi_curl,&still_running);

	int handles = 0;
	CURLMcode code = CURLM_OK;
	while ((code = curl_multi_perform(multi_curl, &handles)) == CURLM_CALL_MULTI_PERFORM)
		;

	if (code != CURLM_OK)
	{
		// ��ȡ��ϸ������Ϣ
		const char* szErr = curl_multi_strerror(code);
		fprintf(stderr, "failed: %s\n", szErr);
	}

	// �ͷű�
	curl_formfree(formpost);

	curl_slist_free_all(headerlist);
	curl_easy_cleanup(curl);
	time_t now2 = time(0);

	std::cout << "send " << str << " info ok , used : \t" << now2 - now << " s" << std::endl;

}


void  Logger::SetFilename(std::string dir) {
	_messageList[dir];
	for (auto it = _messageList.begin(); it != _messageList.end(); ++it) {
		char buf[128] = { 0 };
		sprintf(buf, "./log/%s.log", it->first.c_str());
		int fd = open(buf, O_RDWR | O_CREAT, 0666);
		close(fd);
	}
}

void Logger::LogInit()
{
	lock_init();
	mkdir("./log", 0777);
	_messageList.clear();
}


void  Logger::lock_init() {

	pthread_mutexattr_init(&mutexattr);
	pthread_mutex_init(&_swap_lock, &mutexattr);
}

void Logger::ClearBufToFile()
{
	for (auto it = _messageList.begin(); it != _messageList.end(); ++it) {
		//Flush_Messagebuff(it->first,it->second);
	}
}

void Logger::DirInit()
{
	LogInit();
	for (auto it = _messageList.begin(); it != _messageList.end(); ++it) {
		char buf[128] = { 0 };
		sprintf(buf, "./log/%s.log", it->first.c_str());
		int fd = open(buf, O_RDWR | O_CREAT, 0666);
		close(fd);
	}
}

void signal_demo(int)
{
	//Logger::getInstance()->ClearBufToFile();
	exit(0);
}

LoggerRef::LoggerRef() {
	Logger::getInstance()->DirInit();
	signal(SIGINT, signal_demo);
	signal(SIGSEGV, signal_demo);

}

LoggerRef::~LoggerRef() {
	//Logger::getInstance()->ClearBufToFile();
}


