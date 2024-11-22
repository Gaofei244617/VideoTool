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
	std::string url;         // 视频路径
	int64_t size = 0;        // 文件长度(Byte)
	double duration = 0;     // 视频时长(秒)
	double fps = 0;          // 帧率
	int width = 0;           // 画面宽度
	int height = 0;          // 画面高度
	int stream_num = 0;      // 流数量
	int video_index = -1;    // 视频流索引
	EncodeType encode;       // 视频流编码格式
};

VideoInfo GetVideoInfo(const std::string& video);