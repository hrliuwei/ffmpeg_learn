#include "Peg.h"
//#pragma warning(disable:4996)
// namespace {
// 	int decode_write_frame(const char* outfilename,)
// }

CPeg& CPeg::GetInstacen()
{
	static CPeg s_peg;
	return s_peg;
}
int64_t lastReadPacktTime = 0;
static int interrupt_cb(void *ctx)
{
	int timeout = 3;
	if (av_gettime() - lastReadPacktTime > timeout*1000*1000){
		return -1;
	}
	return 0;
}

void CPeg::Init()
{
	av_register_all();
	avcodec_register_all();
	//avfilter_register_all();
	avformat_network_init();
	avdevice_register_all();
	avcodec_register_all();
	
	av_log_set_level(AV_LOG_ERROR);
}

int CPeg::OpenInput(std::string url)
{
	
	AVDictionary *format_opts = nullptr;
	//av_dict_set_int(&format_opts, "rtbufsize", 3041280 * 10, 0);
 	av_dict_set(&format_opts, "avioflags", "direct", 0);
 	av_dict_set(&format_opts, "video_size", "960x720", 0);
 	av_dict_set(&format_opts, "framerate", "30", 0);
 	av_dict_set(&format_opts, "vcodec", "mjpeg", 0);
	AVInputFormat *inputFormat = av_find_input_format("dshow");
	std::string urlex = "video=" + url;

	inputContext = avformat_alloc_context();
	lastReadPacktTime = av_gettime();
	inputContext->interrupt_callback.callback = interrupt_cb;
	int ret = avformat_open_input(&inputContext, urlex.c_str(), inputFormat, &format_opts);
	if (ret < 0){
		av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
		return ret;
	}
	ret = avformat_find_stream_info(inputContext, nullptr);
	if (ret < 0){
		av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
	}
	else {
		av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n", url.c_str());
	}
	return 0;
}

int CPeg::OpenOutput(std::string url)
{
	int ret = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts", url.c_str());
	if (ret < 0){
		av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
		goto ERROR;
	}
	ret = avio_open2(&outputContext->pb, url.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
	if (ret  < 0 ){
		av_log(NULL, AV_LOG_ERROR, "open avio failed");
		goto ERROR;
	}
	for (int i = 0; i < inputContext->nb_streams; i++) {
		AVStream* stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);
		ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);
		if (ret < 0){
			av_log(NULL, AV_LOG_ERROR, "copy coddec context failed");
			goto ERROR;
		}
	}
	ret = avformat_write_header(outputContext, nullptr);
	if (ret < 0){
		av_log(NULL, AV_LOG_ERROR, "format write header failed");
		goto ERROR;
	}
	av_log(NULL, AV_LOG_FATAL, " Open output file success %s\n", url.c_str());
	return ret;

ERROR:
	if (outputContext){
		for (int i = 0; i < outputContext->nb_streams; i++) {
			avcodec_close(outputContext->streams[i]->codec);
		}
		avformat_close_input(&outputContext);
	}
	return ret;
}


void CPeg::CloseInput()
{
	if (inputContext != nullptr){
		avformat_close_input(&inputContext);
	}
}

void CPeg::CloseOutput()
{
	if (outputContext != nullptr){
		for (int i = 0; i < outputContext->nb_streams; ++i) {
			AVCodecContext *codecContext = outputContext->streams[i]->codec;
			avcodec_close(codecContext);
		}
		avformat_close_input(&outputContext);
	}
}

std::shared_ptr<AVPacket> CPeg::ReadPacketFromSource()
{
	std::shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket* p) {av_packet_free(&p); av_free(p); });
	av_init_packet(packet.get());
	lastReadPacktTime = av_gettime();
	int ret = av_read_frame(inputContext, packet.get());
	if (ret >=0 ){
		return packet;
	}
	return nullptr;
}

void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb)
{
	if (pkt->pts != AV_NOPTS_VALUE)
		pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
	if (pkt->dts != AV_NOPTS_VALUE)
		pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
	if (pkt->duration > 0)
		pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
}

int CPeg::WritePacket(std::shared_ptr<AVPacket> packet)
{
	auto inputStream = inputContext->streams[packet->stream_index];
	auto outputStream = outputContext->streams[packet->stream_index];
	av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
	return av_interleaved_write_frame(outputContext, packet.get());
}

void CPeg::ProcessImage(std::shared_ptr<AVPacket> packte)
{
	//av_decode_video
}