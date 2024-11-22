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
	std::string url;      // ����ַ
	std::string video;    // ������Ƶ
	int loop = 1;         // ѭ������
	std::function<void(double)> callback = nullptr; // ���ȼ��
};

int send_rtsp(const RTSPConfig& config);