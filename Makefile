# use pkg-config for getting CFLAGS and LDLIBS
FFMPEG_LIBS=    libavdevice                        \
                libavformat                        \
                libavfilter                        \
                libavcodec                         \
                libswresample                      \
                libswscale                         \
                libavutil                          \

CFLAGS += -Wall -g
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
CXXFLAGS := $(CFLAGS)
LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)

EXAMPLES=  h265enc h26xdec h264enc dump_yuv				   

# the following examples make explicit use of the math library

.phony: all clean-test clean

all:  $(EXAMPLES)

h264enc:           LDLIBS += -lm
h265enc:           LDLIBS += -lm 
h26xdec: h26xdec.cpp
	$(CXX) $< -lm $(CXXFLAGS) $(LDLIBS) -o $@

clean-test:
	$(RM) test*.pgm test.h264 test.mp2 test.sw test.mpg *ppm

clean: clean-test
	$(RM) $(EXAMPLES) 
