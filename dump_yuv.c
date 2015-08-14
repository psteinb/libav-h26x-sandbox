
#include <math.h>
#include <stdint.h>
#include <stdio.h>


#define WIDTH 352
#define HEIGHT 288
#define DEPTH 25

void fill_volume(unsigned char* _volume){

  uint32_t z =0;
  uint32_t y =0;
  uint32_t x =0;
  uint32_t i =0;

  float offset = 0.f;
  float ymin = 0;
  float ymax = 0;
	
  for (z=0; z<DEPTH; ++z)
    {
      for (y=0; y<HEIGHT; ++y)
	{
	  offset = .05*(z/5)*HEIGHT;
	  ymin = (.1*HEIGHT)+offset;
	  ymax = (.13*HEIGHT)+offset;
	  
	  for (x=0; x < WIDTH; ++x)
	    {
	      i = (z*WIDTH*HEIGHT) + (y*WIDTH) + x;

	      _volume[i] = x + y + z * 3;

	      if((y>ymin && y<ymax) &&
		 (x % 4 < 2) && x<=(5*(i+1)))
		_volume[i] = 16;
	    
            }
	  
	  x = 0;
	}
      y = 0;
    }

}

int main(int argc, char **argv)
{
  uint32_t num_elements = WIDTH*HEIGHT*DEPTH;
  uint32_t pixels_per_frame = WIDTH*HEIGHT;

  unsigned char y [num_elements];
  unsigned char u [num_elements/4];
  unsigned char v [num_elements/4];

  uint32_t i =0;
  for(;i<num_elements;++i){
    y [i] = 0;
  }

  fill_volume(y);

  i =0;
  for(;i<num_elements/4;++i){
    u [i] = 128;
    v [i] = 128;
  }

  unsigned int z = 0;

  FILE* out = fopen("yuv420p_25f_352x288.bin","wb");

  for(;z<DEPTH;++z){
    
    fwrite(&y[z*pixels_per_frame],1,pixels_per_frame,out);
    
    fwrite(&u[z*pixels_per_frame/4],1,pixels_per_frame/4,out);

    fwrite(&v[z*pixels_per_frame/4],1,pixels_per_frame/4,out);
    
  }
  fclose(out);

  //works after rename: ffmpeg -s 352x288 -pix_fmt yuv420p -i yuv420p_25f_352x288.yuv -vframes 25 test_yuv.mp4
  //works: ffmpeg -vc rawvideo -f rawvideo -r 25 -s 352x288 -pix_fmt yuv420p -i yuv420p_25f_352x288.bin -vframes 25 raw_yuv.mp4
  printf("%i frames (%ix%i) written to yuv420p_25f_352x288.bin\n",DEPTH,WIDTH,HEIGHT);
  
  return 0;
}
