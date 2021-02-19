#include "IMonitor.h"
#include "ini.h"


IMonitor::Config IMonitor::_config;

int IMonitor::_tatalMomery;
//std::unordered_map<pid_t, std::ifstream> IMonitor::processDirectstats;

//std::ifstream  IMonitor::fd;

IMonitor::CPUInfo IMonitor::_CPUInfo;

//memery模块：存入指向pid对应stat的指针
void IMonitor::FileOpenReady()
{
	for (int i = 0; i < IMonitor::_config.processNames.size(); ++i) {
		pid_t pid;
		
		for (int j = 0; j < _config.processNames.size(); ++j) {
			pid = _config.processNames[j].pid;

			char path[32] = { 0 };
			sprintf(path, "/proc/%d/stat", pid);

			processDirectstats[pid].open(path);
		}
	}

	fd.open("/proc/stat", std::ios::in);
	if (!fd) {
		std::cerr << "open /proc/stat error!" << std::endl;
		exit(0);
	}

	_CPUInfo.cpuNumber = get_nprocs_conf();
	_tatalMomery = GetTatalMemroy();

}

//传过来的path="infomation.ini"
void IMonitor::ReadConfig(std::string path)
{
	mINI::INIFile file(path);
	mINI::INIStructure ini;
	file.read(ini);

	int num = std::atoi(ini["process"]["number"].c_str());
	for (int i = 0; i < num; ++i) {
		
		pid_t pids[32] = { 0 };
		int procnum = getPidByName(ini["process"]["name" + std::to_string(i + 1)].c_str(), pids, 32);
		if (procnum <= 0) {
			std::cerr << "where is " << ini["process"]["name" + std::to_string(i + 1)] << std::endl;
			exit(1);
		}

		for (int j = 0; j < procnum; ++j) {
			ProcessInfo processInfo;
			processInfo.processName = GetCmdLineByPid(pids[j]);
			processInfo.pid = pids[j];
			_config.processNames.push_back(processInfo);
		}
	}
	_config.timeInterval = std::atoi(ini["flush_time"]["time"].c_str());
	_config._ip = ini["http_server"]["ip"];
	_config._port = ini["http_server"]["port"];

	_config._localip = ini["local_adress"]["ip"];
}

char * IMonitor::basename(const char *path)
{
	register const char *s;
	register const char *p;

	p = s = path;

	while (*s) {
		if (*s++ == '/') {
			p = s;
		}
	}

	return (char *)p;
}

int IMonitor::getPidByName(const char* process_name, pid_t pid_list[], int list_size)
{
#define  MAX_BUF_SIZE       256

	DIR *dir;
	struct dirent *next;
	int count = 0;
	pid_t pid;
	FILE *fp;
	char *base_pname = NULL;
	char *base_fname = NULL;
	char cmdline[MAX_BUF_SIZE];
	char path[MAX_BUF_SIZE];

	if (process_name == NULL || pid_list == NULL)
		return -EINVAL;

	base_pname = basename(process_name);
	if (strlen(base_pname) <= 0)
		return -EINVAL;

	dir = opendir("/proc");
	if (!dir)
	{
		return -EIO;
	}
	while ((next = readdir(dir)) != NULL) {
		/* skip non-number */
		if (!isdigit(*next->d_name))
			continue;

		pid = strtol(next->d_name, NULL, 0);
		sprintf(path, "/proc/%u/cmdline", pid);
		fp = fopen(path, "r");
		if (fp == NULL)
			continue;

		memset(cmdline, 0, sizeof(cmdline));
		if (fread(cmdline, MAX_BUF_SIZE - 1, 1, fp) < 0) {
			fclose(fp);
			continue;
		}
		fclose(fp);
		base_fname = basename(cmdline);

		if (strcmp(base_fname, base_pname) == 0)
		{
			if (count >= list_size) {
				break;
			}
			else {
				pid_list[count] = pid;
				count++;
			}
		}
	}
	closedir(dir);
	return count;
}

std::vector<std::string> split(std::string strtem, char a)
{
	std::vector<std::string> strvec;

	std::string::size_type pos1, pos2;
	pos2 = strtem.find(a);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		strvec.push_back(strtem.substr(pos1, pos2 - pos1));

		pos1 = pos2 + 1;
		pos2 = strtem.find(a, pos1);
	}
	strvec.push_back(strtem.substr(pos1));
	return strvec;
}



std::string IMonitor::GetCmdLineByPid(pid_t pid){
	std::ifstream fd;
	char path[256] = { 0 };
	sprintf(path, "/proc/%d/cmdline", pid);
	fd.open(path);
	std::string line;
	std::getline(fd, line);
	auto ret = split(line, '/');
	fd.close();
	
	return ret.at(ret.size() - 1);
}




int IMonitor::GetTatalMemroy()
{
	const char* file = "/proc/meminfo";//文件名
	FILE *fd;         //定义文件指针fd
	char line_buff[256] = { 0 };  //读取行的缓冲区
	fd = fopen(file, "r"); //以R读的方式打开文件再赋给指针fd

	//获取memtotal:总内存占用大小
	int i;
	char name[32];//存放项目名称
	int memtotal;//存放内存峰值大小
	char*ret = fgets(line_buff, sizeof(line_buff), fd);//读取memtotal这一行的数据,memtotal在第1行
	sscanf(line_buff, "%s %d", name, &memtotal);
	fclose(fd);     //关闭文件fd
	return memtotal;
}

