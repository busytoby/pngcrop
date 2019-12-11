#include <stdlib.h>
#include <stdio.h>
#include <png.h>

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers = NULL;

void read_png_file(char *filename) {
  FILE *fp = fopen(filename, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_read_info(png, info);

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  if (row_pointers) abort();

  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  fclose(fp);

  png_destroy_read_struct(&png, &info, NULL);
}

void write_png_file(char *filename) {
  int y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  if (!row_pointers) abort();

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);

  png_destroy_write_struct(&png, &info);
}

int avg_pixel(png_bytep px) {
  int pAvg = 0;
  if(px[0] > px[1])
    if(px[1] > px[2])
      return ((px[0] + px[1]) / 2);
    else
      return ((px[0] + px[2]) / 2);
  else if(px[0] > px[2])
    return ((px[0] + px[1]) / 2);
  else
    return ((px[1] + px[2]) / 2);
}

void process_png_file() {
  int step=1;
  int X = 0;
  int Y = 0;
  int Width = 0;
  int Height = 0;
  int cAvg;
  int saturation = 100;
  int passLimit = 3;

  int steps;

  bool foundY = false;
  bool foundH = false;

  png_bytep row = row_pointers[height - (height / 12)]; // try and get page color
  cAvg = 0;

  for(int n = 1; n < width/28; n++) {
    int xptr1 = ((width /2) - n) * 4;
    int xptr2 = ((width /2) + n) * 4;
    png_bytep px = &(row[xptr1]);
//    printf("-%4d = RGBA(%3d, %3d, %3d, %3d)\n", n, px[0], px[1], px[2], px[3]);
    if(cAvg == 0)
      cAvg = avg_pixel(px);
    else
      cAvg = (cAvg + avg_pixel(px)) / 2;
    px = &(row[xptr2]);
//    printf("+%4d = RGBA(%3d, %3d, %3d, %3d) = %3d\n", n, px[0], px[1], px[2], px[3], cAvg);
    cAvg = (cAvg + avg_pixel(px)) / 2;
    saturation = cAvg - (cAvg / 3);
  }

  row = row_pointers[height / 2];
  /*
  for(int n = 1; n < step * 4; n+=step/2) {
    int xptr1 = ((width /2) - n) * 4;
    int xptr2 = ((width /2) + n) * 4;
    png_bytep px = &(row[xptr1]);
    printf("-%4d = RGBA(%3d, %3d, %3d, %3d)\n", n, px[0], px[1], px[2], px[3]);
    px = &(row[xptr2]);
    printf("+%4d = RGBA(%3d, %3d, %3d, %3d)\n", n, px[0], px[1], px[2], px[3]);
  }
  return;
  */

  int pass = 0;

  for(int x = 1; x < width; x+=step) {
    bool pass = true;
    int avgPx = 0;
    for(int n = 1; n < height / 64; n+=step) {
      png_bytep row1 = row_pointers[(height/2) - n];
      png_bytep row2 = row_pointers[(height/2) + n];
      png_bytep px1 = &(row1[x * 4]);
      png_bytep px2 = &(row2[x * 4]);
      int avgPx1 = avg_pixel(px1); 
      int avgPx2 = avg_pixel(px2); 
      if(avgPx == 0) avgPx = avgPx1;
//      printf("%3d = %3d %3d %3d\n", n, avgPx, avgPx1, avgPx2);
      if(abs((avgPx * 2) - (avgPx1 + avgPx2)) > 20 || (avgPx < saturation)) {
        pass = false;
        break;
      }
      avgPx = (avgPx + avgPx1 + avgPx2) / 3;
    }
    if(pass == true) {
      X = x;
//      printf("%3d\n", X);
      break;
    }
  }

  for(int x = width - 1; x > 0; x-=step) {
    bool pass = true;
    int avgPx = 0;
    for(int n = 1; n < height / 64; n+=step) {
      png_bytep row1 = row_pointers[(height/2) - n];
      png_bytep row2 = row_pointers[(height/2) + n];
      png_bytep px1 = &(row1[x * 4]);
      png_bytep px2 = &(row2[x * 4]);
      int avgPx1 = avg_pixel(px1);
      int avgPx2 = avg_pixel(px2);
      if(avgPx == 0) avgPx = avgPx1;
      if(abs((avgPx * 2) - (avgPx1 + avgPx2)) > 20 || (avgPx < saturation)) {
        pass = false;
        break;
      }
      avgPx = (avgPx + avgPx1 + avgPx2) / 3;
    }
    if(pass == true) {
      Width = x - X;
      break;
    }
  }

/*
  for(int x = 1; x < width; x+=step) {
    png_bytep px = &(row[x * 4]);
    if(avg_pixel(px) > saturation) {
      X = x;
      if(pass++>passLimit) {
        X = X - passLimit;
        break;
      }
    }
  }
*/

/*
  pass = 0;
  for(int x = width-1; x > 0; x-=step) {
    png_bytep px = &(row[x * 4]);
//    printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, height/2, px[0], px[1], px[2], px[3]);
    if(avg_pixel(px) > saturation) {
      Width = x - X;
      if(pass++>passLimit) {
        Width = Width + passLimit;
        break;
      }
    }
  }
*/

  pass = 0;
  for(int y = 1; y < height; y+=step) {
    png_bytep row = row_pointers[y];
    png_bytep px = &(row[(width / 2) * 4]);
//    printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", width/2, y, px[0], px[1], px[2], px[3]);
    if(avg_pixel(px) > saturation) {
      Y = y;
      if(pass++>passLimit) {
        Y = Y - passLimit;
        break;
      }
    }
  }

  pass = 0;
  for(int y = height-1; y > 0; y-=step) {
    png_bytep row = row_pointers[y];
    png_bytep px = &(row[(width / 2) * 4]);
    if(avg_pixel(px) > saturation) {
      Height = y - Y;
      if(pass++>passLimit) {
        Height = Height + passLimit;
        break;
      }
    }
  }

  printf("%dX%d+%d+%d", Width, Height, X, Y);
//          printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);

}

int main(int argc, char *argv[]) {
  if(argc != 2) abort();

  read_png_file(argv[1]);
  process_png_file();
//  write_png_file(argv[2]);

  return 0;
}
