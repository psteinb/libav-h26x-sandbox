#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <iterator>

extern "C" {
#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
  #include "av_format_extend.h"
}

#include "utils.hpp"

#define INBUF_SIZE 4096

static const uint32_t WIDTH = 352;
static const uint32_t HEIGHT = 288;
static const uint32_t DEPTH = 25;

/*
 * Video encoding example
 */
static void video_encode_example(const char *filename, AVCodecID codec_id)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int  ret,  got_output;
    FILE *f;
    AVFrame *frame;
    AVPacket pkt;
    /* uint8_t endcode[] = { 0, 0, 1, 0xb7 }; */

    printf("Encode video file %s\n", filename);

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = WIDTH;
    c->height = HEIGHT;
    /* frames per second */
    c->time_base = (AVRational){1,25};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }

    /* encode 1 second of video */
    for (uint32_t i = 0; i < DEPTH; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        fflush(stdout);
        /* prepare a dummy image */
        /* Y */
	float offset = 0.f;
	float ymin = 0;
	float ymax = 0;
        for (uint32_t y = 0; y < (uint32_t)c->height; y++) {
	  
	  offset = .05*(i/5)*c->height;
	  ymin = (.1*c->height)+offset;
	  ymax = (.13*c->height)+offset;
	  
	  for (uint32_t x = 0; x < (uint32_t)c->width; x++) {
	      
	    frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
	    if((y>ymin && y<ymax) &&
	       (x % 4 < 2) && x<=(5*(i+1)))
	      frame->data[0][(y) * frame->linesize[0] + x] = 16;
	    
            }
        }

        /* Cb and Cr */
        for (uint32_t y = 0; y < c->height/2u; y++) {
            for (uint32_t x = 0; x < c->width/2u; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
		/* if((y>(.1/2.*c->height) && y<(.3/2.*c->height)) && */
		/*    (x % 2 < 1) && x<=(5*(i+1))){ */
		/*   frame->data[1][y * frame->linesize[1] + x] = 128; */
		/*   frame->data[2][y * frame->linesize[2] + x] = 128; */
		/* } */

            }
        }

        frame->pts = i;

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
	  printf("Write frame %3d (size=%5d)\n", i,pkt.size);
	  fwrite(pkt.data, 1, pkt.size, f);
	  av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    got_output = 1;
    for (uint32_t i = DEPTH; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d) [delayed]\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    //fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    printf("\n");
}

/*
 * Video encoding example
 */
static void video_encode_to_buffer(std::vector<uint8_t>& _buffer, AVCodecID codec_id)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int  ret,  got_output;
    AVFrame *frame;
    AVPacket pkt;
    
    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = WIDTH;
    c->height = HEIGHT;
    /* frames per second */
    c->time_base = (AVRational){1,25};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }

    /* encode 1 second of video */
    for (uint32_t i = 0; i < DEPTH; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        fflush(stdout);
        /* prepare a dummy image */
        /* Y */
	float offset = 0.f;
	float ymin = 0;
	float ymax = 0;
        for (uint32_t y = 0; y < (uint32_t)c->height; y++) {
	  
	  offset = .05*(i/5)*c->height;
	  ymin = (.1*c->height)+offset;
	  ymax = (.13*c->height)+offset;
	  
	  for (uint32_t x = 0; x < (uint32_t)c->width; x++) {
	      
	    frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
	    if((y>ymin && y<ymax) &&
	       (x % 4 < 2) && x<=(5*(i+1)))
	      frame->data[0][(y) * frame->linesize[0] + x] = 16;
	    
            }
        }

        /* Cb and Cr */
        for (uint32_t y = 0; y < c->height/2u; y++) {
            for (uint32_t x = 0; x < c->width/2u; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
		/* if((y>(.1/2.*c->height) && y<(.3/2.*c->height)) && */
		/*    (x % 2 < 1) && x<=(5*(i+1))){ */
		/*   frame->data[1][y * frame->linesize[1] + x] = 128; */
		/*   frame->data[2][y * frame->linesize[2] + x] = 128; */
		/* } */

            }
        }

        frame->pts = i;

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
	  printf("Write frame %3d (size=%5d)\n", i,pkt.size);
	  std::copy(pkt.data,pkt.data+pkt.size,std::back_inserter(_buffer));
	    //	  fwrite(pkt.data, 1, pkt.size, f);
	  av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    got_output = 1;
    for (uint32_t i = DEPTH; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d) [delayed]\n", i, pkt.size);
	    std::copy(pkt.data,pkt.data+pkt.size,std::back_inserter(_buffer));
            // fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    //fwrite(endcode, 1, sizeof(endcode), f);
    // fclose(f);

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    printf("\n");
}

int decode_buffer_to_files(const std::vector<uint8_t>& _buffer, const std::string& _fbase){

    av_register_all();
    AVFrame* frame = avcodec_alloc_frame();
    if (!frame)
    {
        return 1;
    }

    AVFormatContext* formatContext = NULL;
    formatContext = avformat_alloc_context();//TODO: handle errors!!

    //taken from https://ffmpeg.org/doxygen/trunk/avio_reading_8c-example.html#a18
    AVIOContext *avio_ctx = NULL;
    uint8_t *avio_ctx_buffer = NULL;
    size_t avio_ctx_buffer_size = 4096;

    
    buffer_data read_data = {0};
    read_data.ptr = &_buffer[0];
    read_data.size = _buffer.size();
      
    avio_ctx_buffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
      std::cerr << "failed to allocated avio_ctx_buffer\n";
      //handle deallocation
      avformat_close_input(&formatContext);
      av_freep(&avio_ctx_buffer);
      return 1;
    }
    
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
                                  0, &read_data, &read_packet, NULL, NULL);
    if (!avio_ctx) {
      
      std::cerr << "failed to allocated avio_ctx_buffer\n";
      //handle deallocation
      avformat_close_input(&formatContext);
      return 1;
      
    }
    formatContext->pb = avio_ctx;
    
    if (avformat_open_input(&formatContext, "", NULL, NULL) != 0)
    {
      avformat_close_input(&formatContext);
      if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
      }
      av_free(frame);
      return 1;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0)
    {
        av_free(frame);
        avformat_close_input(&formatContext);
	if (avio_ctx) {
	  av_freep(&avio_ctx->buffer);
	  av_freep(&avio_ctx);
	}
        return 1;
    }

    if (formatContext->nb_streams < 1 ||
        formatContext->streams[0]->codec->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        av_free(frame);
        avformat_close_input(&formatContext);
	if (avio_ctx) {
	  av_freep(&avio_ctx->buffer);
	  av_freep(&avio_ctx);
	}
        return 1;
    }

    AVStream* stream = formatContext->streams[0];
    AVCodecContext* codecContext = stream->codec;

    codecContext->codec = avcodec_find_decoder(codecContext->codec_id);
    if (codecContext->codec == NULL)
    {
        av_free(frame);
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
	if (avio_ctx) {
	  av_freep(&avio_ctx->buffer);
	  av_freep(&avio_ctx);
	}
        return 1;
    }
    else if (avcodec_open2(codecContext, codecContext->codec, NULL) != 0)
    {
        av_free(frame);
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
	if (avio_ctx) {
	  av_freep(&avio_ctx->buffer);
	  av_freep(&avio_ctx);
	}
        return 1;
    }

    AVPacket packet;
    av_init_packet(&packet);



    int frameNumber = 0;
    while (av_read_frame(formatContext, &packet) == 0)
    {
        if (packet.stream_index == stream->index)
        {
            int frameFinished = 0;
            avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

            if (frameFinished)
            {
	      saveFrame(frame, codecContext->width, codecContext->height, frameNumber++,_fbase.c_str());
                std::cout << frameNumber << ":\trepeat: " << frame->repeat_pict
                      << "\tkeyframe: " << frame->key_frame
                      << "\tbest_ts: " << frame->best_effort_timestamp << '\n';
            }
        }
    }


    int frameFinished = 1;
    while(frameFinished){

      packet.data = NULL;
      packet.size = 0;
      int rcode = avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);
      if(rcode < 0 ){
	std::cerr << "[delayed]\tdecide error detected\n";
	break;
      }
      
      if (frameFinished)
	{
	  saveFrame(frame, codecContext->width, codecContext->height, frameNumber++,_fbase.c_str());
	  std::cout << frameNumber << ":\trepeat: " << frame->repeat_pict
		<< "\tkeyframe: " << frame->key_frame
		<< "\tbest_ts: " << frame->best_effort_timestamp << "[delayed]\n";
	}
    }
    
    av_free_packet(&packet);
    av_free(frame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    if (avio_ctx) {
	  av_freep(&avio_ctx->buffer);
	  av_freep(&avio_ctx);
	}
    return 0;

}

int decode_video_file(const std::string& _fname)
{
    av_register_all();
    AVFrame* frame = avcodec_alloc_frame();
    if (!frame)
    {
        return 1;
    }

    AVFormatContext* formatContext = NULL;
    if (avformat_open_input(&formatContext, _fname.c_str(), NULL, NULL) != 0)
    {
        av_free(frame);
        return 1;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0)
    {
        av_free(frame);
        avformat_close_input(&formatContext);
        return 1;
    }

    if (formatContext->nb_streams < 1 ||
        formatContext->streams[0]->codec->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        av_free(frame);
        avformat_close_input(&formatContext);
        return 1;
    }

    AVStream* stream = formatContext->streams[0];
    AVCodecContext* codecContext = stream->codec;

    codecContext->codec = avcodec_find_decoder(codecContext->codec_id);
    if (codecContext->codec == NULL)
    {
        av_free(frame);
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        return 1;
    }
    else if (avcodec_open2(codecContext, codecContext->codec, NULL) != 0)
    {
        av_free(frame);
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        return 1;
    }

    AVPacket packet;
    av_init_packet(&packet);



    int frameNumber = 0;
    while (av_read_frame(formatContext, &packet) == 0)
    {
        if (packet.stream_index == stream->index)
        {
            int frameFinished = 0;
            avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

            if (frameFinished)
            {
	      saveFrame(frame, codecContext->width, codecContext->height, frameNumber++,_fname.c_str());
                std::cout << frameNumber << ":\trepeat: " << frame->repeat_pict
                      << "\tkeyframe: " << frame->key_frame
                      << "\tbest_ts: " << frame->best_effort_timestamp << '\n';
            }
        }
    }


    int frameFinished = 1;
    while(frameFinished){

      packet.data = NULL;
      packet.size = 0;
      int rcode = avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);
      if(rcode < 0 ){
	std::cerr << "[delayed]\tdecide error detected\n";
	break;
      }
      
      if (frameFinished)
	{
	  saveFrame(frame, codecContext->width, codecContext->height, frameNumber++,_fname.c_str());
	  std::cout << frameNumber << ":\trepeat: " << frame->repeat_pict
		<< "\tkeyframe: " << frame->key_frame
		<< "\tbest_ts: " << frame->best_effort_timestamp << "[delayed]\n";
	}
    }
    
    av_free_packet(&packet);
    av_free(frame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    return 0;
}


int main(int argc, char **argv)
{

    /* register all the codecs */
    avcodec_register_all();

    if (argc < 2){

      std::cout << "usage: ./roundtrip <codec>\n"
		<< "app to experiment on how to decode an encoded piece of memory\nrather then writing it to disk"
		<< "codec\tpossible values: 'h264' or 'hevc'\n";
      return 1;
    }

    std::string oname;
    AVCodecID codec_id;
    
    std::string file_type = argv[1];
    if(file_type.find("h264") != std::string::npos){
      codec_id = AV_CODEC_ID_H264;
      oname = "test.h264";
    }
    else{

      if(file_type.find("hevc") != std::string::npos){
	codec_id  = AV_CODEC_ID_HEVC;
	oname = "test.hevc";
      }
      else{
	std::cerr << "file_type unknown " << file_type << "\n";
	return 1;
      }
      
    }

    //that works!
    video_encode_example(oname.c_str(), codec_id);
    decode_video_file(oname);

    std::vector<uint8_t> fbuffer;
    std::ifstream ifile(oname, std::ios::binary | std::ios::in );
    std::stringstream buffer;
    buffer << ifile.rdbuf();
    std::string content = buffer.str();
    std::copy(content.begin(), content.end(),std::back_inserter(fbuffer));
    ifile.close();
    std::cerr << "(re-)read "<< fbuffer.size() <<"B from "<< oname <<"\n";
    
    //TODO: that needs to work!
    std::string buffered_name = "buffered-";
    buffered_name += oname;
    if(!decode_buffer_to_files(fbuffer,buffered_name))
      std::cerr << "decode_buffer_to_files failed\n";
    
    
    // std::string buffered = "buffered-";
    // buffered += oname;
    // video_encode_to_buffer(fbuffer, codec_id);
    // std::ofstream ofile(buffered, std::ios::binary | std::ios::out | std::ios::trunc );
    // ofile.write(reinterpret_cast<const char*>(&fbuffer[0]),fbuffer.size());
    // ofile.close();

    


    
    return 0;
}
