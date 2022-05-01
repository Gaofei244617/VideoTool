#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

bool isH265IDR(const int type)
{
	return type == 19 || type == 20 || type == 21;
}

int ES2MP4(std::vector<Nalu>& vencStream, std::string fileName, int width, int height, double frameRate, std::string type)
{
	AVFormatContext* pOutFmtCtx = NULL;
	AVOutputFormat* pOutFmt = NULL;
	AVCodec* pCodec = NULL;
	AVStream* pStream = NULL;
	AVPacket* pPacket = av_packet_alloc();

	AVRational timeBase;
	timeBase.num = 100;
	timeBase.den = int(frameRate * 100);

	int frameNum = 0;
	bool vps = false, sps = false, pps = false, sei = false;
	bool writeFirstFrame = false;

	if (vencStream.empty())
	{
		printf("Input stream is NULL\n");
		return -1;
	}

	int ret = avformat_alloc_output_context2(&pOutFmtCtx, NULL, NULL, fileName.c_str());
	if (ret < 0)
	{
		printf("Failed to alloc output format context [fileName: %s]\n", fileName.c_str());
		return -1;
	}

	pOutFmt = (AVOutputFormat*)pOutFmtCtx->oformat;

	pCodec = (AVCodec*)avcodec_find_encoder(pOutFmt->video_codec);
	if (pCodec == NULL)
	{
		printf("No encoder[%s] found\n", avcodec_get_name(pOutFmt->video_codec));
		goto end;
	}

	pStream = avformat_new_stream(pOutFmtCtx, pCodec);
	if (pStream == NULL)
	{
		printf("Failed to alloc stream\n");
		goto end;
	}

	//宽高，帧率，编码格式
	pStream->codecpar->width = width;
	pStream->codecpar->height = height;
	pStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	if (type == "H264")
	{
		pStream->codecpar->codec_id = AV_CODEC_ID_H264;
	}
	else if (type == "H265")
	{
		pStream->codecpar->codec_id = AV_CODEC_ID_HEVC;
	}

	//extradata空间大小只需能装下VPS, SPS, PPS, SEI等即可
	pStream->codecpar->extradata = (uint8_t*)av_malloc(1024);
	pStream->codecpar->extradata_size = 0;

	if (!(pOutFmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&pOutFmtCtx->pb, fileName.c_str(), AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("could not open %s\n", fileName.c_str());
			goto end;
		}
	}

	ret = avformat_write_header(pOutFmtCtx, NULL);
	if (ret < 0)
	{
		printf("Error occurred when opening output file \n");
	}

	if (type == "H264")
	{
		for (int i = 0; i < vencStream.size(); i++)
		{
			// H264: 6-SEI, 7-SPS, 8-PPS, 5-IDR
			bool bExtraData = false;
			if (vencStream[i].type == 7 && !sps)
			{
				bExtraData = true;
				sps = true;
			}
			else if (vencStream[i].type == 8 && !pps)
			{
				bExtraData = true;
				pps = true;
			}
			else if (vencStream[i].type == 6 && !sei)
			{
				bExtraData = true;
				sei = true;
			}

			if (bExtraData)
			{
				uint8_t* dst = pStream->codecpar->extradata + pStream->codecpar->extradata_size;
				memcpy(dst, vencStream[i].data, vencStream[i].data_len);
				pStream->codecpar->extradata_size += vencStream[i].data_len;
			}

			// 写入封装器的第一帧必须是IDR帧
			if (writeFirstFrame || vencStream[i].type == 5)
			{
				writeFirstFrame = true;
				pPacket->flags |= vencStream[i].type == 5 ? AV_PKT_FLAG_KEY : 0;
				pPacket->stream_index = pStream->index;
				pPacket->data = vencStream[i].data;
				pPacket->size = vencStream[i].data_len;
				pPacket->pts = av_rescale_q(frameNum, timeBase, pStream->time_base);
				pPacket->dts = pPacket->pts;
				pPacket->duration = 0;	//0 if unknown.
				pPacket->pos = -1;		//-1 if unknown

				ret = av_interleaved_write_frame(pOutFmtCtx, pPacket);
				if (ret < 0)
				{
					printf("av_interleaved_write_frame failed\n");
					goto end;
				}

				av_packet_unref(pPacket);

				frameNum++;
			}
		}
	}
	else if (type == "H265")
	{
		for (int i = 0; i < vencStream.size(); i++)
		{
			// H265: 32-VPS, 33-SPS, 34-PPS, 39-SEI_PREFIX, 40-SEI_SUFFI
			bool bExtraData = false;
			if (vencStream[i].type == 32 && !vps)
			{
				bExtraData = true;
				vps = true;
			}
			else if (vencStream[i].type == 33 && vps && !sps)
			{
				bExtraData = true;
				sps = true;
			}
			else if (vencStream[i].type == 34 && vps && sps && !pps)
			{
				bExtraData = true;
				pps = true;
			}

			if (bExtraData)
			{
				uint8_t* dst = pStream->codecpar->extradata + pStream->codecpar->extradata_size;
				memcpy(dst, vencStream[i].data, vencStream[i].data_len);
				pStream->codecpar->extradata_size += vencStream[i].data_len;
			}

			// 写入封装器的第一帧必须是IDR帧
			// H265: IDR_W_RADL = 19, IDR_N_LP = 20, CRA_NUT = 21
			if (writeFirstFrame || isH265IDR(vencStream[i].type))
			{
				writeFirstFrame = true;
				pPacket->flags |= isH265IDR(vencStream[i].type) ? AV_PKT_FLAG_KEY : 0;
				pPacket->stream_index = pStream->index;
				pPacket->data = vencStream[i].data;
				pPacket->size = vencStream[i].data_len;
				pPacket->pts = av_rescale_q(frameNum, timeBase, pStream->time_base);
				pPacket->dts = pPacket->pts;
				pPacket->duration = 0;	//0 if unknown.
				pPacket->pos = -1;		//-1 if unknown

				ret = av_interleaved_write_frame(pOutFmtCtx, pPacket);
				if (ret < 0)
				{
					printf("av_interleaved_write_frame failed\n");
					goto end;
				}

				av_packet_unref(pPacket);

				frameNum++;
			}
		}
	}
	av_packet_free(&pPacket);

	av_write_trailer(pOutFmtCtx);

end:
	if (pStream && pStream->codecpar->extradata)
	{
		av_free(pStream->codecpar->extradata);
		pStream->codecpar->extradata = NULL;
	}

	if (pOutFmtCtx && !(pOutFmtCtx->flags & AVFMT_NOFILE))
	{
		avio_close(pOutFmtCtx->pb);
	}

	avformat_free_context(pOutFmtCtx);

	return 0;
}
