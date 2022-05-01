#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
}

void getES(const char* filename)
{
	AVFormatContext* pFormatContext = NULL;
	avformat_open_input(&pFormatContext, filename, NULL, NULL);
	if (avformat_find_stream_info(pFormatContext, NULL) < 0)
	{
		return;
	}

	int videoIdx = -1;
	std::string type;
	for (int i = 0; i < pFormatContext->nb_streams; i++)
	{
		AVCodecParameters* pCodecParam = pFormatContext->streams[i]->codecpar;
		if (pCodecParam->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIdx = i;
			if (pCodecParam->codec_id == AV_CODEC_ID_H264)
			{
				type = "H264";
			}
			else if (pCodecParam->codec_id == AV_CODEC_ID_HEVC)
			{
				type = "H265";
			}
			break;
		}
	}

	const char* fmt = type == "H264" ? "h264_mp4toannexb" : "hevc_mp4toannexb";
	const AVBitStreamFilter* bsf = av_bsf_get_by_name(fmt);

	AVBSFContext* bsfCtx = NULL;
	av_bsf_alloc(bsf, &bsfCtx);

	avcodec_parameters_copy(bsfCtx->par_in, pFormatContext->streams[videoIdx]->codecpar);
	av_bsf_init(bsfCtx);

	std::ofstream out("out.h265", std::ios::out | std::ios::binary);
	std::ofstream len("out.len", std::ios::out | std::ios::binary);

	AVPacket* pPacket = av_packet_alloc();
	while (av_read_frame(pFormatContext, pPacket) == 0)
	{
		if (pPacket->stream_index == videoIdx)
		{
			if (av_bsf_send_packet(bsfCtx, pPacket) == 0)
			{
				while (av_bsf_receive_packet(bsfCtx, pPacket) == 0)
				{
					out.write((char*)pPacket->data, pPacket->size);
					len.write((char*)&(pPacket->size), sizeof(pPacket->size));
				}
			}
		}
		av_packet_unref(pPacket);
	}
	av_packet_free(&pPacket);
	out.close();
	len.close();
}

