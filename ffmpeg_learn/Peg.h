#pragma once
#include <iostream>
#include <memory>

extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
class CPeg
{
public:
	~CPeg() {};
	static CPeg& GetInstacen();
	void Init();
	int OpenInput(std::string url);
	int OpenOutput(std::string url);
	void CloseInput();
	void CloseOutput();
	std::shared_ptr<AVPacket> ReadPacketFromSource();
	int WritePacket(std::shared_ptr<AVPacket> packet);

private:
	AVFormatContext *inputContext = nullptr;
	AVFormatContext *outputContext;
	
};

