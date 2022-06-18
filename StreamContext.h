#pragma once

#include <string>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
}

// ��Ƶ�����ʽ
enum class EncodeType
{
	UNKNOWN = 0,      // δ֪
	H264 = 1,         // H.264
	HEVC = 2          // H.265
};

// ��Ƶ��Ϣ
struct VideoInfo
{
	int width = 0;                      // ��Ƶ��
	int height = 0;                     // ��Ƶ��
	double fps = 0;                     // ƽ��֡��
	double duration = 0;                // ��Ƶʱ�����룩
	int64_t bitRate = 0;                // ��Ƶ����
	EncodeType encodeType = EncodeType::UNKNOWN; // �����ʽ
};

class StreamContext
{
public:
	StreamContext() {};
	StreamContext(const std::string url);
	~StreamContext();

	void init(const std::string url);
	int readFrame(unsigned char* data);
	VideoInfo info()const;

protected:
	VideoInfo videoInfo;  // ��Ƶ����Ϣ
	int videoIdx = -1; // ��Ƶ������
	AVFormatContext* pFormatContext = NULL;
	AVBSFContext* bsfCtx = NULL;
	AVPacket* pPacket = NULL;
};

/*****************************************************************************/
StreamContext::StreamContext(const std::string url)
{
	init(url);
}

StreamContext::~StreamContext()
{
	av_packet_free(&pPacket);
	av_bsf_free(&bsfCtx);
	avio_closep(&pFormatContext->pb);
	avformat_close_input(&pFormatContext);
}

void StreamContext::init(const std::string url)
{
	if (avformat_open_input(&pFormatContext, url.c_str(), NULL, NULL) < 0)
	{
		throw "open url error";
		return;
	}

	if (avformat_find_stream_info(pFormatContext, NULL) < 0)
	{
		throw "find stream info error";
		return;
	}

	// ������Ƶʱ�����룩
	if (pFormatContext->duration != AV_NOPTS_VALUE)
	{
		int secs, us;
		int64_t duration = pFormatContext->duration;
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		videoInfo.duration = secs + us / double(AV_TIME_BASE);
	}

	// ������Ƶ��
	for (int i = 0; i < pFormatContext->nb_streams; i++)
	{
		AVCodecParameters* pCodecParam = pFormatContext->streams[i]->codecpar;
		if (pCodecParam->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIdx = i;
			if (pCodecParam->codec_id == AV_CODEC_ID_H264)
			{
				videoInfo.encodeType = EncodeType::H264;
			}
			else if (pCodecParam->codec_id == AV_CODEC_ID_HEVC)
			{
				videoInfo.encodeType = EncodeType::HEVC;
			}

			videoInfo.bitRate = pCodecParam->bit_rate;     // ����
			videoInfo.width = pCodecParam->width;
			videoInfo.height = pCodecParam->height;
			videoInfo.fps = av_q2d(pFormatContext->streams[i]->avg_frame_rate);

			break;
		}
	}

	const char* fmt = videoInfo.encodeType == EncodeType::H264 ? "h264_mp4toannexb" : "hevc_mp4toannexb";
	AVBitStreamFilter* bsf = (AVBitStreamFilter*)av_bsf_get_by_name(fmt);
	av_bsf_alloc(bsf, &bsfCtx);

	avcodec_parameters_copy(bsfCtx->par_in, pFormatContext->streams[videoIdx]->codecpar);
	av_bsf_init(bsfCtx);

	pPacket = av_packet_alloc();
}

int StreamContext::readFrame(unsigned char* data)
{
	while (av_read_frame(pFormatContext, pPacket) == 0)
	{
		if (pPacket->stream_index == videoIdx)
		{
			int offset = 0;
			if (av_bsf_send_packet(bsfCtx, pPacket) == 0)
			{
				while (av_bsf_receive_packet(bsfCtx, pPacket) == 0)
				{
					memcpy(data + offset, pPacket->data, pPacket->size);
					offset += pPacket->size;
				}
			}
			av_packet_unref(pPacket);
			return offset;
		}
		else
		{
			av_packet_unref(pPacket);
		}
	}
	return -1;
}

VideoInfo StreamContext::info()const
{
	return videoInfo;
}

