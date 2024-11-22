#include <iostream>
#include <filesystem>
#include "send_rtsp.h"

int send_rtsp(const RTSPConfig& config)
{
	if (!std::filesystem::exists(config.video))
	{
		return 10;
	}

	VideoInfo videoInfo = GetVideoInfo(config.video); // ��Ƶ��Ϣ
	if (videoInfo.video_index == -1)
	{
		// û����Ƶ��
		return 20;
	}

	AVFormatContext* pInFmtCtx = NULL;      // ������
	AVFormatContext* pOutFmtCtx = NULL;     // �����
	AVStream* pOutStream = NULL;            // �����Ƶ��
	AVCodec* pCodec = NULL;                 // ������

	int frameNum = 0;       // ֡����
	int ret = 0;            // ������

	auto span = std::chrono::microseconds(int(1000 / videoInfo.fps * 1000)); // ֡���
	decltype(std::chrono::high_resolution_clock::now()) startTime; // ��ʼ����ʱ��

	// ���ļ�
	ret = avformat_open_input(&pInFmtCtx, config.video.c_str(), NULL, NULL);
	if (ret < 0)
	{
		ret = 30;
		goto end;
	}

	// ��ȡ����Ϣ
	ret = avformat_find_stream_info(pInFmtCtx, NULL);
	if (ret != 0)
	{
		ret = 40;
		goto end;
	}

	// �������������
	ret = avformat_alloc_output_context2(&pOutFmtCtx, NULL, "rtsp", config.url.c_str());
	if (ret < 0)
	{
		ret = 50;
		goto end;
	}

	// ����һ���µ���
	pCodec = (AVCodec*)avcodec_find_encoder(pOutFmtCtx->oformat->video_codec);
	if (pCodec == NULL)
	{
		ret = 60;
		goto end;
	}

	pOutStream = avformat_new_stream(pOutFmtCtx, pCodec);
	if (pOutStream == NULL)
	{
		ret = 70;
		goto end;
	}

	// ����������Ϣ
	ret = avcodec_parameters_copy(pOutStream->codecpar, pInFmtCtx->streams[videoInfo.video_index]->codecpar);
	if (ret < 0)
	{
		ret = 80;
		goto end;
	}
	pOutStream->codecpar->codec_tag = 0;

	// д��ͷ����Ϣ
	ret = avformat_write_header(pOutFmtCtx, NULL);
	if (ret < 0)
	{
		ret = 90;
		goto end;
	}

	startTime = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < config.loop; i++)
	{
		// ʱ�����
		AVRational timeBase = av_make_q(1000, int(videoInfo.fps * 1000 + 0.5));
		AVRational otime = pOutFmtCtx->streams[videoInfo.video_index]->time_base;

		AVPacket avPacket;
		while (true)
		{
			if (av_read_frame(pInFmtCtx, &avPacket) == AVERROR_EOF)
			{
				av_packet_unref(&avPacket);
				break;
			}

			if (avPacket.stream_index == videoInfo.video_index)
			{
				// ����ת��ʱ���
				avPacket.pts = av_rescale_q_rnd(frameNum, timeBase, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
				avPacket.dts = avPacket.pts;
				avPacket.duration = 0;
				avPacket.pos = -1;

				// ������֡�ٶ�
				std::this_thread::sleep_until(startTime + span * frameNum);

				// ��֡
				ret = av_interleaved_write_frame(pOutFmtCtx, &avPacket);
				if (ret < 0)
				{
					break;
				}

				frameNum++;

				// ��������
				if (config.callback)
				{
					config.callback(frameNum / (videoInfo.fps * videoInfo.duration));
				}
			}
			av_packet_unref(&avPacket);
		}

		// ���´��ļ�
		avio_closep(&pInFmtCtx->pb);
		avformat_close_input(&pInFmtCtx);
		ret = avformat_open_input(&pInFmtCtx, videoInfo.url.c_str(), NULL, NULL);
		if (ret < 0)
		{
			ret = 20;
			goto end;
		}
	}

end:
	if (pInFmtCtx)
	{
		avio_closep(&pInFmtCtx->pb);
		avformat_close_input(&pInFmtCtx);
	}

	if (pOutFmtCtx && !(pOutFmtCtx->flags & AVFMT_NOFILE))
	{
		avio_close(pOutFmtCtx->pb);
	}

	avformat_free_context(pOutFmtCtx);

	return ret;
}