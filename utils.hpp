#ifndef _UTILS_H_
#define _UTILS_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

extern "C" {
#include <libavcodec/avcodec.h>
}

void saveFrame(const AVFrame* frame, int width, int height, int frameNumber, const std::string& _frame_base)
{

    std::stringstream oname;
    oname << _frame_base << "-slice" << frameNumber << ".ppm";

    std::ofstream file(oname.str().c_str(),
		       std::ios_base::binary |
		       std::ios_base::trunc |
		       std::ios_base::out);

    if (!file.good())
    {
        throw std::runtime_error("Unable to open the file to write the frame");
    }

    file << "P5\n" << width << '\n' << height << "\n255\n";

    for (int i = 0; i < height; ++i)
    {
        file.write((char*)(frame->data[0] + i * frame->linesize[0]), width);
    }

    file.close();
}

//https://ffmpeg.org/doxygen/trunk/avio_reading_8c-example.html#a18

struct buffer_data {
    const uint8_t *ptr;
    size_t size; ///< size left in the buffer
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = std::min((decltype(bd->size))buf_size, bd->size);
    printf("ptr:%p size:%zu\n", bd->ptr, bd->size);
    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;
    return buf_size;
}

// //http://stackoverflow.com/questions/9604633/reading-a-file-located-in-memory-with-libavformat
// 

// static int readFunction(void* opaque, uint8_t* buf, int buf_size) {
//     auto& me = *reinterpret_cast<buffer_data*>(opaque);
//     me.read(reinterpret_cast<char*>(buf), buf_size);
//     return ;
// }

// std::ifstream stream("file.avi", std::ios::binary);

// const std::shared_ptr<unsigned char> buffer(reinterpret_cast<unsigned char*>(av_malloc(8192)), &av_free);
// const std::shared_ptr<AVIOContext> avioContext(avio_alloc_context(buffer.get(), 8192, 0, reinterpret_cast<void*>(static_cast<std::istream*>(&stream)), &readFunction, nullptr, nullptr), &av_free);

// const auto avFormat = std::shared_ptr<AVFormatContext>(avformat_alloc_context(), &avformat_free_context);
// auto avFormatPtr = avFormat.get();
// avFormat->pb = avioContext.get();
// avformat_open_input(&avFormatPtr, "dummyFilename", nullptr, nullptr);




#endif /* _UTILS_H_ */
