#include "MonitorNotify.h"
#include "curl/curl.h"

void MonitorNotify::ReadConfigFile(std::string path)
{
	IMonitor::ReadConfig(path);
	CURL* curl = curl_easy_init();

	CURLcode res;
	char url[256] = { 0 };
	sprintf(url, "http://%s:%s/start", IMonitor::_config._ip.c_str(), IMonitor::_config._port.c_str());
	curl_easy_setopt(curl, CURLOPT_URL, url);//设置url
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		// 获取详细错误信息
		const char* szErr = curl_easy_strerror(res);
		fprintf(stderr, "failed: %s\n", szErr);
		curl_easy_cleanup(curl);
		exit(0);
	}
	curl_easy_cleanup(curl);

}

void MonitorNotify::AddMonitor(IMonitor* monitor)
{
	_monitors.insert(monitor);
}

void MonitorNotify::RemoveMonitor(IMonitor* monitor)
{
	_monitors.erase(monitor);
}

void MonitorNotify::MonitorStart(){

	for (auto it = _monitors.begin(); it != _monitors.end(); ++it) {
		(*it)->SetLogFiles();
		(*it)->FileOpenReady();
		timer.setInterval([=]() {
			(*it)->Calculate();
			(*it)->WriteLogs();
		}, IMonitor::_config.timeInterval);
	}
}

void MonitorNotify::MonitorStop()
{
	timer.stop();
}
