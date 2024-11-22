#pragma once

#include <string>

enum class EncodeType
{
	H264 = 0,
	HEVC = 1,
	Other
};

struct VideoInfo
{
	std::string url;         // ��Ƶ·��
	int64_t size = 0;        // �ļ�����(Byte)
	double duration = 0;     // ��Ƶʱ��(��)
	double fps = 0;          // ֡��
	int width = 0;           // ������
	int height = 0;          // ����߶�
	int stream_num = 0;      // ������
	EncodeType encode;       // �����ʽ
};

VideoInfo GetVideoInfo(const std::string& video);