#include <iostream>
#include <filesystem>
#include "RTSPSender.h"

extern "C"
{
#include "libavutil/mathematics.h"
};

RTSPSender::~RTSPSender()
{
	release();
}

int RTSPSender::init(const RTSPConfig& config)
{
	m_video = config.video;
	m_url = config.url;
	m_loop = config.loop;
	m_callback = config.callback;

	if (!std::filesystem::exists(m_video))
	{
		return 10;
	}

	// 打开文件
	int ret = avformat_open_input(&pInFmtCtx, m_video.c_str(), NULL, NULL);
	if (ret < 0)
	{
		release();
		return 20;
	}

	// 获取流信息
	ret = avformat_find_stream_info(pInFmtCtx, 0);
	if (ret != 0)
	{
		release();
		return 30;
	}

	// 视频流索引
	for (unsigned int i = 0; i < pInFmtCtx->nb_streams; i++)
	{
		if (pInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoIndex = i;
			break;
		}
	}

	if (m_videoIndex == -1)
	{
		release();
		return 40;
	}

	// 视频时长
	int64_t d = pInFmtCtx->streams[m_videoIndex]->duration;
	m_duration = av_q2d(pInFmtCtx->streams[m_videoIndex]->time_base) * d;

	// 创建输出上下文
	ret = avformat_alloc_output_context2(&pOutFmtCtx, NULL, "rtsp", m_url.c_str());
	if (ret < 0)
	{
		release();
		return 50;
	}

	// 创建一个新的流
	AVOutputFormat* pOutFmt = (AVOutputFormat*)pOutFmtCtx->oformat;
	AVCodec* pCodec = (AVCodec*)avcodec_find_encoder(pOutFmt->video_codec);
	if (pCodec == NULL)
	{
		release();
		return 60;
	}

	AVStream* outStream = avformat_new_stream(pOutFmtCtx, pCodec);
	if (outStream == NULL)
	{
		release();
		return 70;
	}

	// 复制配置信息
	ret = avcodec_parameters_copy(outStream->codecpar, pInFmtCtx->streams[m_videoIndex]->codecpar);
	if (ret < 0)
	{
		release();
		return 80;
	}
	outStream->codecpar->codec_tag = 0;

	//// 打开输出io (rtsp不需手动调用该函数)
	//ret = avio_open(&pOutFmtCtx->pb, url.c_str(), AVIO_FLAG_WRITE);
	//if (ret < 0)
	//{
	//	release();
	//	return 90;
	//}

	return 0;
}

int RTSPSender::start()
{
	double fps = av_q2d(pInFmtCtx->streams[m_videoIndex]->avg_frame_rate);   // 帧率
	auto span = std::chrono::microseconds(int(1000 / fps * 1000));           // 帧间隔

	// 时间基数
	AVRational timeBase;
	timeBase.num = 1000;
	timeBase.den = int(fps * 1000 + 0.5);
	AVRational otime = pOutFmtCtx->streams[m_videoIndex]->time_base;

	// 写入头部信息
	int ret = avformat_write_header(pOutFmtCtx, NULL);
	if (ret < 0)
	{
		release();
		return 200;
	}

	int frameNum = 0; // 帧计数
	AVPacket avPacket; // 推流每一帧数据
	auto startTime = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < m_loop; i++)
	{
		while (true)
		{
			ret = av_read_frame(pInFmtCtx, &avPacket);
			if (ret == AVERROR_EOF)
			{
				break;
			}

			if (avPacket.stream_index != m_videoIndex)
			{
				continue;
			}

			// 计算转换时间戳
			avPacket.pts = av_rescale_q_rnd(frameNum, timeBase, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
			avPacket.dts = avPacket.pts;
			avPacket.duration = 0;
			avPacket.pos = -1;

			// 控制推帧速度
			std::this_thread::sleep_until(startTime + span * frameNum);

			// 推帧
			ret = av_interleaved_write_frame(pOutFmtCtx, &avPacket);
			if (ret < 0)
			{
				break;
			}

			frameNum++;

			// 推流进度
			if (m_callback)
			{
				m_callback(frameNum * 1.0 / fps / m_duration);
			}
		}

		// 重新打开文件
		avio_closep(&pInFmtCtx->pb);
		avformat_close_input(&pInFmtCtx);
		ret = avformat_open_input(&pInFmtCtx, m_video.c_str(), NULL, NULL);
		if (ret < 0)
		{
			release();
			return 20;
		}
	}

	release();

	return 0;
}

void RTSPSender::release()
{
	if (pOutFmtCtx && !(pOutFmtCtx->flags & AVFMT_NOFILE))
	{
		avio_close(pOutFmtCtx->pb);
	}

	avformat_free_context(pOutFmtCtx);

	if (pInFmtCtx)
	{
		avio_closep(&pInFmtCtx->pb);
		avformat_close_input(&pInFmtCtx);
	}

	pInFmtCtx = NULL;
	pOutFmtCtx = NULL;
}