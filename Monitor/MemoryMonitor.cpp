#include "MemoryMonitor.h"

std::unordered_map<pid_t, uint64_t> MemoryMonitor::_processMemoryInfo;

void MemoryMonitor::Calculate()
{
	for (auto it = processDirectstats.begin(); it != processDirectstats.end(); ++it) {
		it->second.clear();
		it->second.seekg(0);
		std::vector<std::string> words;
		std::string word;
		std::string line;

		while (std::getline(it->second, line)) {
			std::istringstream instream(line);
			while (instream >> word) {
				words.push_back(word);
			}
		}

		_processMemoryInfo[it->first] = (long int)std::atoi(words[23].c_str()) * (long int)getpagesize() / 1024;
	}
}

//记录数据到log的缓存区
void MemoryMonitor::WriteLogs()
{
	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {
		LOG_INFO("memory_" + std::to_string((*it).pid) + "_" + (*it).processName, std::to_string(_processMemoryInfo[it->pid]));
	}
}

void MemoryMonitor::SetLogFiles()
{
	for (auto it = IMonitor::_config.processNames.begin(); it != IMonitor::_config.processNames.end(); ++it) {

		SET_LOG_FILE("memory_" + std::to_string((*it).pid) + "_" + (*it).processName);
	}
}
