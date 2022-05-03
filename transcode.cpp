extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int transcode(const char* in_file, const char* out_file)
{
	// Input
	AVFormatContext* pInFmtCtx = NULL;
	if (avformat_open_input(&pInFmtCtx, in_file, NULL, NULL) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "ERROR could not open input file: %s\n", in_file);
		return -1;
	}

	if (avformat_find_stream_info(pInFmtCtx, NULL) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "ERROR could not get the stream info\n");
		return -1;
	}

	int videoIdx = -1;
	for (int i = 0; i < pInFmtCtx->nb_streams; i++)
	{
		if (pInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIdx = i;
			break;
		}
	}

	AVCodecParameters* pCodecParam = pInFmtCtx->streams[videoIdx]->codecpar;
	const AVCodec* pCodec = avcodec_find_decoder(pCodecParam->codec_id); // ½âÂëÆ÷
	AVCodecContext* pInCodecContext = avcodec_alloc_context3(pCodec);

	avcodec_parameters_to_context(pInCodecContext, pCodecParam);
	pInCodecContext->framerate = av_guess_frame_rate(pInFmtCtx, pInFmtCtx->streams[videoIdx], NULL);
	int ret = avcodec_open2(pInCodecContext, pCodec, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "error: avcodec_open2: %s\n", out_file);
		return -1;
	}

	// Output
	AVFormatContext* pOutFmtCtx = NULL;
	avformat_alloc_output_context2(&pOutFmtCtx, NULL, NULL, out_file);

	const AVCodec* pEncodec = avcodec_find_encoder(AV_CODEC_ID_HEVC); // ±àÂëÆ÷
	AVCodecContext* pOutCodecContext = avcodec_alloc_context3(pEncodec);

	AVStream* pOutStream = avformat_new_stream(pOutFmtCtx, pEncodec);

	pOutCodecContext->width = pInCodecContext->width;
	pOutCodecContext->height = pInCodecContext->height;
	pOutCodecContext->sample_aspect_ratio = pInCodecContext->sample_aspect_ratio;
	pOutCodecContext->pix_fmt = pInCodecContext->pix_fmt;
	//pOutCodecContext->time_base = av_inv_q(pInCodecContext->framerate);
	pOutCodecContext->time_base = pInCodecContext->time_base;

	pOutStream->time_base = pOutCodecContext->time_base;

	if (pOutFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		pOutCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	ret = avcodec_open2(pOutCodecContext, pEncodec, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "error: avcodec_open2\n");
		return -1;
	}

	avcodec_parameters_from_context(pOutStream->codecpar, pOutCodecContext);

	if (!(pOutFmtCtx->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&pOutFmtCtx->pb, out_file, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "error: avio_open: %s\n", out_file);
			return -1;
		}
	}
	avformat_write_header(pOutFmtCtx, NULL);

	AVPacket* pInPacket = av_packet_alloc();
	AVPacket* pOutPacket = av_packet_alloc();
	AVFrame* pFrame = av_frame_alloc();

	while (av_read_frame(pInFmtCtx, pInPacket) >= 0)
	{
		if (pInPacket->stream_index == videoIdx)
		{
			av_packet_rescale_ts(pInPacket, pInFmtCtx->streams[videoIdx]->time_base, pInCodecContext->time_base);
			ret = avcodec_send_packet(pInCodecContext, pInPacket);
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "error: avcodec_send_packet\n");
				continue;
			}

			while (true)
			{
				ret = avcodec_receive_frame(pInCodecContext, pFrame);
				if (ret < 0)
				{
					break;
				}
				else
				{
					int ret2 = avcodec_send_frame(pOutCodecContext, pFrame);
					if (ret2 < 0)
					{
						av_log(NULL, AV_LOG_ERROR, "Error sending a frame for encoding\n");
						continue;
					}

					while (true)
					{
						ret2 = avcodec_receive_packet(pOutCodecContext, pOutPacket);
						if (ret2 < 0)
						{
							break;
						}
						av_packet_rescale_ts(pOutPacket, pOutCodecContext->time_base, pOutFmtCtx->streams[0]->time_base);
						av_interleaved_write_frame(pOutFmtCtx, pOutPacket);
					}
				}
			}

			av_packet_unref(pInPacket);
			av_packet_unref(pOutPacket);
			av_frame_unref(pFrame);
		}
	}

	av_packet_free(&pInPacket);
	av_packet_free(&pOutPacket);
	av_frame_free(&pFrame);

	av_write_trailer(pOutFmtCtx);

end:
	avio_closep(&pInFmtCtx->pb);
	avformat_close_input(&pInFmtCtx);
	avio_closep(&pOutFmtCtx->pb);
	avformat_free_context(pOutFmtCtx);

	return 0;
}

int main()
{
	transcode("input.mp4", "output.mp4"); // H264 -> H265
	return 0;
}