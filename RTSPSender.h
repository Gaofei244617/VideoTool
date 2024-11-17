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
	std::string m_video;                      // ��Ƶ�ļ�
	std::string m_url;                        // rtsp���
	int m_videoIndex = -1;                    // ��Ƶ����
	double m_duration = 0;                    // ��Ƶ����
	std::function<void(double)> m_callback;   // ���ȼ�غ���
	AVFormatContext* pInFmtCtx = NULL;        // ������
	AVFormatContext* pOutFmtCtx = NULL;       // �����
};
