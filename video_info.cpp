#include <filesystem>
#include "video_info.h"

extern "C"
{
#include <libavformat/avformat.h>
}

VideoInfo GetVideoInfo(const std::string& video)
{
	VideoInfo info;

	AVFormatContext* pInFmtCtx = NULL;
	AVStream* pVideoStream = NULL;
	AVCodecParameters* codecpar = NULL;

	info.url = video;
	info.size = std::filesystem::file_size(video);

	// 打开文件
	int ret = avformat_open_input(&pInFmtCtx, video.c_str(), NULL, NULL);
	if (ret < 0)
	{
		goto end;
	}

	// 获取流信息
	ret = avformat_find_stream_info(pInFmtCtx, 0);
	if (ret != 0)
	{
		goto end;
	}

	info.stream_num = pInFmtCtx->nb_streams;
	for (unsigned int i = 0; i < pInFmtCtx->nb_streams; i++)
	{
		if (pInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			info.video_index = i;
			codecpar = pInFmtCtx->streams[i]->codecpar;
			pVideoStream = pInFmtCtx->streams[i];

			break;
		}
	}

	if (info.video_index == -1)
	{
		goto end;
	}

	info.fps = av_q2d(pVideoStream->avg_frame_rate);
	info.width = codecpar->width;
	info.height = codecpar->height;

	// 视频时长
	if (pVideoStream->duration > 0)
	{
		info.duration = av_q2d(pVideoStream->time_base) * pVideoStream->duration;
	}
	else
	{
		// 无法直接通过封装信息获取时长, 通过读取帧数来计算
		int cnt = 0;
		AVPacket packet;
		while (true)
		{
			if (av_read_frame(pInFmtCtx, &packet) < 0)
			{
				av_packet_unref(&packet);
				break;
			}

			cnt++;
			av_packet_unref(&packet);
		}
		info.duration = cnt / info.fps;
	}

	// 编码格式
	info.encode = EncodeType::Other;
	if (codecpar->codec_id == AV_CODEC_ID_H264)
	{
		info.encode = EncodeType::H264;
	}
	else if (codecpar->codec_id == AV_CODEC_ID_HEVC)
	{
		info.encode = EncodeType::HEVC;
	}

end:
	if (pInFmtCtx)
	{
		avio_closep(&pInFmtCtx->pb);
		avformat_close_input(&pInFmtCtx);
	}

	return info;
}