extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <fstream>
#include "yuv_to_jpg.h"

int yuv420p_to_jpg(const AVFrame* frame, const std::string& output_filename)
{
	// 查找JPEG编码器
	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	if (!codec)
	{
		std::cerr << "JPEG encoder not found" << std::endl;
		return -1;
	}

	AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
	if (!pCodecCtx)
	{
		std::cerr << "Could not allocate video codec context" << std::endl;
		return -1;
	}

	// 设置编码器参数
	pCodecCtx->width = frame->width;
	pCodecCtx->height = frame->height;
	pCodecCtx->time_base = { 1, 25 };
	pCodecCtx->pix_fmt = (AVPixelFormat)frame->format;

	AVPacket* pkt = av_packet_alloc();;

	// 打开编码器
	if (avcodec_open2(pCodecCtx, codec, nullptr) < 0)
	{
		std::cerr << "Could not open codec" << std::endl;
		return -1;
	}

	int ret = avcodec_send_frame(pCodecCtx, frame);
	if (ret < 0)
	{
		std::cerr << "Error sending a frame for encoding" << std::endl;
		return -1;
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(pCodecCtx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			break;
		}
		else if (ret < 0)
		{
			std::cerr << "Error during encoding" << std::endl;
			return -1;
		}

		// 将JPG数据写入文件
		std::ofstream outfile(output_filename, std::ios::binary);
		if (outfile.is_open())
		{
			outfile.write(reinterpret_cast<const char*>(pkt->data), pkt->size);
			outfile.close();
		}
		else
		{
			std::cerr << "Unable to open output file" << std::endl;
			return -1;
		}

		av_packet_unref(pkt);
	}

	avcodec_free_context(&pCodecCtx);

	return 0;
}