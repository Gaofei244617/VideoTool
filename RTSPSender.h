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
	std::string url;      // ����ַ
	std::string video;    // ������Ƶ
	int loop = 1;         // ѭ������
	std::function<void(double)> callback = nullptr; // ��������
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
	std::string m_video;                      // ��Ƶ�ļ�
	std::string m_url;                        // rtsp���
	int m_loop = 1;                           // ѭ������
	int m_videoIndex = -1;                    // ��Ƶ����
	double m_duration = 0;                    // ��Ƶ����
	std::function<void(double)> m_callback;   // ���ȼ�غ���
	AVFormatContext* pInFmtCtx = NULL;        // ������
	AVFormatContext* pOutFmtCtx = NULL;       // �����
};
