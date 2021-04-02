#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <wtypes.h>
#include <chrono>
#include <thread>
#include <ppltasks.h>
#include <fstream>
#include <iterator>
#include <vector>

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavformat/avio.h"
    #include "libswscale/swscale.h"
    #include "libavutil/imgutils.h"
    #include "SDL.h"
    #include "SDL_image.h"
#include "libavcodec/dxva2.h"
#include <libavutil/file.h>
}

#include "SDLFunctionality.h"


#undef main

AVBufferRef* hw_device_ctx = NULL;
enum AVHWDeviceType type;

AVCodec* Codec = NULL;
AVCodecContext* codec_ctx = NULL;
AVPacket* pkt = NULL;
AVFrame* frame = NULL, * pFrameRGB = NULL, * sw_frame = NULL, * tmp_frame = NULL;
AVFormatContext* fmt_ctx = NULL;
AVIOContext* avio_ctx = NULL;

struct SwsContext* sws_ctx = NULL;

uint8_t* buffer = NULL, * avio_ctx_buffer = NULL;
int numBytes = 0;
uint8_t* buffer_rgb;
size_t buffer_size, avio_ctx_buffer_size = 1024 * 180;
int ret = 0;
struct buffer_data bd = { 0 };

s_decoded_frame* struct_export;
s_dimension* dimension_export;

s_decoded_frame* decode(AVCodecContext*, AVFrame*, AVPacket*);
s_decoded_frame* decode_hw_acceleration(AVCodecContext*, AVFrame*, AVPacket*);

static enum AVPixelFormat hw_pix_fmt;

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::microseconds us;
typedef std::chrono::duration<float> fsec;

const char* device_name = "d3d11va";

static int read_packet(void* opaque, uint8_t* buf, int buf_size)
{
	struct buffer_data* bd = (struct buffer_data*)opaque;
	buf_size = FFMIN(buf_size, bd->size);

	printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

	/* copy internal buffer data to buf */
	memcpy(buf, bd->ptr, buf_size);
	bd->ptr += buf_size;
	bd->size -= buf_size;

	return buf_size;
}
static enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
	const enum AVPixelFormat* p;
	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}

	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}
s_dimension* init_ffmpeg(uint8_t* buffer_in, int buffer_size_in)
{
	bd.ptr = buffer_in;
	bd.size = buffer_size_in;
	if (!(fmt_ctx = avformat_alloc_context())) {
		printf("Error : avformat_alloc_context\n");
		return NULL;
	}
	avio_ctx_buffer = static_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size));
	if (!avio_ctx_buffer) {
		printf("Error : avio_ctx_buffer = av_malloc(avio_ctx_buffer_size)\n");
		return NULL;
	}
	avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &bd, &read_packet, NULL, NULL);
	if (!avio_ctx) {
		printf("Error : avio_ctx = avio_alloc_context();\n");
		return NULL;
	}
	fmt_ctx->pb = avio_ctx;

	ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	if (ret < 0) {
		printf("Could not open input\n");
		return NULL;
	}
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0) {
		printf("Could not find stream information\n");
		return NULL;
	}

	codec_ctx = avcodec_alloc_context3(NULL);

	if (ret = avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[0]->codecpar) < 0)
	{
		printf("Cannot get codec parameters\n");
		return NULL;
	}

	/*
	Codec = avcodec_find_decoder_by_name(decoder_name);
	if (Codec == NULL)
	{printf("avcodec_find_decoder_by_name dxva2 failed\n");
		return NULL;
	}
	*/
	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &Codec, 0);
	if (ret < 0) {
		fprintf(stderr, "Cannot find a video stream in the input file\n");
		return NULL;
	}
	

	if (ret = avcodec_open2(codec_ctx, Codec, NULL) < 0)
	{
		printf("Cannot open video decoder\n");
		return NULL;
	}
	pkt = av_packet_alloc();
	av_init_packet(pkt);
	if (!pkt)
	{
		return NULL;
	}

	frame = av_frame_alloc();
	if (!frame)
	{
		printf("Cannot init frame\n");
		return NULL;
	}

	if (ret = av_read_frame(fmt_ctx, pkt) < 0)
	{
		printf("cannot read frame");
		return NULL;
	}

	struct_export = (s_decoded_frame*)malloc(sizeof(s_decoded_frame));
	dimension_export = (s_dimension*)malloc(sizeof(s_dimension));

	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
	{
		return NULL;
	}
	sw_frame = av_frame_alloc();
	if (sw_frame == NULL)
	{
		return NULL;
	}
	tmp_frame = av_frame_alloc();
	if (tmp_frame == NULL)
	{
		return NULL;
	}

	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 16);
	buffer_rgb = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, AV_PIX_FMT_NV12, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer_rgb, AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);

	dimension_export->width = codec_ctx->width;
	dimension_export->height = codec_ctx->height;

	return dimension_export;
}
s_dimension* init_ffmpeg_hw_acceleration(uint8_t* buffer_in, int buffer_size_in)
{
	//Show available decoders by name
	/*
	const AVCodec* current_codec = nullptr;
	void* i = 0;
	while ((current_codec = av_codec_iterate(&i))) {
		if (av_codec_is_decoder(current_codec)) {
			std::cout << "Found decoder " << current_codec->name << std::endl;
		}
	}
	*/
	s_dimension* dimension_export = (s_dimension*)malloc(sizeof(s_dimension));
	dimension_export = init_ffmpeg(buffer_in, buffer_size_in);
	type = av_hwdevice_find_type_by_name(device_name);
	//https://github.com/FFmpeg/FFmpeg/blob/7b100839330ace3b4846ee4a1fc5caf4b8f8a34e/doc/examples/hw_decode.c#L166
	//show available device type ^

	for (int i = 0;; i++) {
		const AVCodecHWConfig* config = avcodec_get_hw_config(Codec, i);
		if (!config) {
			fprintf(stderr, "Decoder %s does not support device type %s.\n",
				Codec->name, av_hwdevice_get_type_name(type));
			return NULL;
		}
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
			config->device_type == type) {
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}
	codec_ctx->get_format = get_hw_format;
	int err = 0;
	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)) < 0) {
		fprintf(stderr, "Failed to create specified HW device.\n");
		return NULL;
	}
	codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	return dimension_export;
}
s_decoded_frame* decode_ffmpeg_hw_acceleration(uint8_t* data, int length)
{
	int result = av_packet_from_data(pkt, data, length);
	if (result < 0) {
		printf("Error : av_packet_from_data\n");
		return NULL;
	}
	s_decoded_frame* struct_export = decode_hw_acceleration(codec_ctx, frame, pkt);
	return struct_export;
}
s_decoded_frame* decode_hw_acceleration(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt)
{
	auto t0 = Time::now();
	auto tstart = t0;
	ret = avcodec_send_packet(dec_ctx, pkt);
	auto t1 = Time::now();
	fsec fs = t1 - t0;
	us d = std::chrono::duration_cast<us>(fs);
	
	
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		return NULL;
	}
	int index = 0;

	t0 = Time::now();
	ret = avcodec_receive_frame(dec_ctx, frame);
	t1 = Time::now();
	fs = t1 - t0;
	us e = std::chrono::duration_cast<us>(fs);
	us f;
	us g = std::chrono::duration_cast<us>(t1 - tstart);
	
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		printf("AVERROR(EAGAIN) or AVERROR_EOF");
		return NULL;
	}
	else if (ret < 0) {
		fprintf(stderr, "Error during decoding\n");
		return NULL;
	}
	fflush(stdout);
	
	if (frame->format == hw_pix_fmt) {
		//retrieve data from GPU to CPU
		t0 = Time::now();
		if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
			fprintf(stderr, "Error transferring the data to system memory\n");
			return NULL;
		}
		t1 = Time::now();
		fs = t1 - t0;
		f = std::chrono::duration_cast<us>(fs);
		
		
		tmp_frame = sw_frame;
	}
	else
		tmp_frame = frame;
	/*
	std::cout << "input time : " << d.count() << "us\n";
	std::cout << "exec time : " << e.count() << "us\n";
	std::cout << "output time : " << f.count() << "us\n";
	std::cout << "total time (d, e) : " << g.count() << "us\n\n";
	*/
	//Convert the image from its natice format to RGB.
	sws_scale(sws_ctx, (uint8_t const* const*)tmp_frame->data, tmp_frame->linesize, 0, codec_ctx->height, pFrameRGB->data, pFrameRGB->linesize);

	struct_export->data = pFrameRGB->data[0];
	struct_export->linesize = pFrameRGB->linesize[0];
	struct_export->width = codec_ctx->width;
	struct_export->height = codec_ctx->height;

	return struct_export;
}
char* get_filename(int index_name)
{
	char s1[5] = "D:\\\\";
	char s2[7];	//6-pshfios ariumos max(999.999 max frames + 1 NULL_TERMINATOR)

	if (index_name > 9999)
	{
		printf("ERROR : index_name > 6 digits number\n");
		return NULL;
	}
	sprintf(s2, "%d", index_name);

	char* result = (char*)malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}
s_decoded_frame* decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt)
{
	auto t0 = Time::now();
	auto tstart = t0;
	ret = avcodec_send_packet(dec_ctx, pkt);
	auto t1 = Time::now();
	fsec fs = t1 - t0;
	us d = std::chrono::duration_cast<us>(fs);


	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		return NULL;
	}
	int index = 0;

	t0 = Time::now();
	ret = avcodec_receive_frame(dec_ctx, frame);
	t1 = Time::now();
	fs = t1 - t0;
	us e = std::chrono::duration_cast<us>(fs);
	std::cout << "input time : " << d.count() << "us\n";
	std::cout << "exec time : " << e.count() << "us\n";

	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		printf("AVERROR(EAGAIN) or AVERROR_EOF");
		return NULL;
	}
	else if (ret < 0) {
		fprintf(stderr, "Error during decoding\n");
		return NULL;
	}
	fflush(stdout);

	//Convert the image from its natice format to RGB.
	sws_scale(sws_ctx, (uint8_t const* const*)frame->data, frame->linesize, 0, codec_ctx->height, pFrameRGB->data, pFrameRGB->linesize);

	struct_export->data = pFrameRGB->data[0];
	struct_export->linesize = pFrameRGB->linesize[0];
	struct_export->width = codec_ctx->width;
	struct_export->height = codec_ctx->height;

	av_frame_unref(frame);
	av_frame_unref(pFrameRGB);

	return struct_export;
}
s_decoded_frame* decode_ffmpeg(uint8_t* data, int length)
{
	int result = av_packet_from_data(pkt, data, length);
	if (result < 0) {
		printf("Error : av_packet_from_data\n");
		return NULL;
	}
	s_decoded_frame* struct_export = decode(codec_ctx, frame, pkt);
	return struct_export;
}

int main()
{
    printf("START PROGRAM __");
    SDLFunctionality* sdlf = new SDLFunctionality();
    int i = 1;
	s_dimension* dims;
	uint8_t* data = NULL;
	size_t iSize = NULL;
	s_decoded_frame* struct_extracted = NULL;

	
    sdlf->Init();
	printf("Hello World!\n");
	av_file_map(get_filename(0), &data, &iSize, NULL, NULL);
	dims = init_ffmpeg_hw_acceleration(data, iSize);
	if (dims != NULL) {
		printf("width, height :: %d, %d\n", dims->width, dims->height);
	}
    while (i < 100) {
		
		auto t0 = Time::now();
		s_decoded_frame* frame = decode_ffmpeg_hw_acceleration(data, iSize);
		auto t1 = Time::now();
		fsec fs = t1 - t0;
		us d = std::chrono::duration_cast<us>(fs);
		std::cout << d.count() << "us\n";
		
		if (frame != NULL) {
			sdlf->ShowImage(frame);
		}
		av_file_map(get_filename(i), &data, &iSize, NULL, NULL);
		i++;
		if (i == 99) {
			i = 0;
		}
    }
    //printf("close result : %d\n", 6);
    //sdlf->Quit();

}
