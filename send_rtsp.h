#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <functional>
#include "video_info.h"

extern "C"
{
#include "libavformat/avformat.h"
};

struct RTSPConfig
{
	std::string url;      // 流地址
	std::string video;    // 本地视频
	int loop = 1;         // 循环次数
	std::function<void(double)> callback = nullptr; // 进度监控
};

int send_rtsp(const RTSPConfig& config);