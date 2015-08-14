#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

extern "C"
{
  #include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
};


int main(int argc, char **argv)
{
    av_register_all();
    AVFrame* frame = avcodec_alloc_frame();
    if (!frame)
    {
        return 1;
    }

    AVFormatContext* formatContext = NULL;
    if (avformat_open_input(&formatContext, argv[1], NULL, NULL) != 0)
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
	      saveFrame(frame, codecContext->width, codecContext->height, frameNumber++,argv[1]);
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
	  saveFrame(frame, codecContext->width, codecContext->height, frameNumber++,argv[1]);
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
