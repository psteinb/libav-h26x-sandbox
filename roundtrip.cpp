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
}
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

    //video_encode_example(oname.c_str(), codec_id);

    std::vector<uint8_t> fbuffer;
    std::string buffered = "buffered-";
    buffered += oname;
    video_encode_to_buffer(fbuffer, codec_id);
    std::ofstream ofile(buffered, std::ios::binary | std::ios::out | std::ios::trunc );
    ofile.write(reinterpret_cast<const char*>(&fbuffer[0]),fbuffer.size());
    ofile.close();

    


    
    return 0;
}
