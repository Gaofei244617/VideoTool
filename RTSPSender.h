#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <functional>

extern "C"
{
#include "libavformat/avformat.h"
};

class RTSPSender
{
public:
	RTSPSender() {};
	~RTSPSender();

	int init(const std::string& video, const std::string& url, std::function<void(double)> callback = nullptr);
	int start();

protected:
	void release();

protected:
	std::string m_video;                      // 视频文件
	std::string m_url;                        // rtsp输出
	int m_videoIndex = -1;                    // 视频索引
	double m_duration = 0;                    // 视频长度
	std::function<void(double)> m_callback;   // 进度监控函数
	AVFormatContext* pInFmtCtx = NULL;        // 输入流
	AVFormatContext* pOutFmtCtx = NULL;       // 输出流
};
