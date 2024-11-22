#include "video_reader.h"

VideoReader::VideoReader(const std::string url)
{
	init(url);
}

VideoReader::~VideoReader()
{
	release();
}

void VideoReader::init(const std::string url)
{
	videoInfo = GetVideoInfo(url);

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

	const char* fmt = videoInfo.encode == EncodeType::H264 ? "h264_mp4toannexb" : "hevc_mp4toannexb";
	AVBitStreamFilter* bsf = (AVBitStreamFilter*)av_bsf_get_by_name(fmt);
	av_bsf_alloc(bsf, &bsfCtx);

	avcodec_parameters_copy(bsfCtx->par_in, pFormatContext->streams[videoInfo.video_index]->codecpar);
	av_bsf_init(bsfCtx);

	pPacket = av_packet_alloc();
}

void VideoReader::reset()
{
	release();
	init(videoInfo.url);
}

int VideoReader::read_frame(unsigned char* data)
{
	while (av_read_frame(pFormatContext, pPacket) == 0)
	{
		if (pPacket->stream_index == videoInfo.video_index)
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

VideoInfo VideoReader::info()const
{
	return videoInfo;
}

void VideoReader::release()
{
	av_packet_free(&pPacket);
	av_bsf_free(&bsfCtx);
	avio_closep(&pFormatContext->pb);
	avformat_close_input(&pFormatContext);
}