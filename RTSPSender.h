#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <functional>

extern "C"
{
#include "libavformat/avformat.h"
};

struct RTSPConfig
{
	std::string url;      // 流地址
	std::string video;    // 本地视频
	int loop = 1;         // 循环次数
	std::function<void(double)> callback = nullptr; // 推流进度
};

class RTSPSender
{
public:
	RTSPSender();
	~RTSPSender();

	int init(const RTSPConfig& config);
	int start();

protected:
	void release();

protected:
	std::string m_video;                      // 视频文件
	std::string m_url;                        // rtsp输出
	int m_loop = 1;                           // 循环次数
	int m_videoIndex = -1;                    // 视频索引
	double m_duration = 0;                    // 视频长度
	std::function<void(double)> m_callback;   // 进度监控函数
	AVFormatContext* pInFmtCtx = NULL;        // 输入流
	AVFormatContext* pOutFmtCtx = NULL;       // 输出流
};
