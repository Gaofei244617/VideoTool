#pragma once

#include <string>
#include "video_info.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
}

/****** 视频帧读取器 ******/
class VideoReader
{
public:
	VideoReader() {};
	VideoReader(const std::string url);
	~VideoReader();

	void init(const std::string url);
	int read_frame(unsigned char* data);
	void reset();
	VideoInfo info()const;

protected:
	void release();

protected:
	VideoInfo videoInfo;  // 视频流信息
	AVFormatContext* pFormatContext = NULL;
	AVBSFContext* bsfCtx = NULL;
	AVPacket* pPacket = NULL;
};
