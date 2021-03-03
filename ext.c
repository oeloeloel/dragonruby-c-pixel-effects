/*
DragonRuby C Extension Pixel Array
Written by @Akzidenz-Grotesk (with help from @AlexDenisov & @Kenneth | CANICVS)
Demonstrates some quick and pretty dirty image filters
Loads image files into Pixel Array
Performs image manipulation every tick
Returns a modified 100x100 pixel image to DragonRuby
*/

#ifndef NULL
#define NULL 0
#endif
typedef unsigned int Uint32;

extern void *(*drb_symbol_lookup)(const char *sym);
typedef void *(*drb_load_image_fn)(const char *fname, int *w, int *h);
typedef void (*drb_upload_pixel_array_fn)(const char *name, const int w, const int h, const Uint32 *pixels);

Uint32 *image = NULL; // source image pixel array
Uint32 *alpha = NULL; // target image with alpha channel
int im_w; // source image dimensions
int im_h;
int al_w; // target image dimensions
int al_h;

// called from DragonRuby
// loads the images and stores them for later use
__attribute__((annotate("drb_ffi:")))
void ext_load_image(char* im_fname, int *imw, int *imh, char* al_fname, int *alw, int *alh) {

  static drb_load_image_fn drb_load_image = NULL;
    
  if (!drb_load_image) {
    drb_load_image = drb_symbol_lookup("drb_load_image");
  }
  
  if (drb_load_image) {
    void *vpi = drb_load_image(im_fname, imw, imh); // load source image
    if(vpi){
      image = (Uint32*) vpi; // store image
      im_w = *imw; // record dimensions
      im_h = *imh;
    }
 
    void *vpa = drb_load_image(al_fname, alw, alh); // load target image (with alpha channel)
    if(vpa){
      alpha = (Uint32*) vpa;
      al_w = *alw;
      al_h = *alh;
    }
  }
}

// called from DragonRuby
// every tick to get changed image
// takes mouse location and required effect
__attribute__((annotate("drb_ffi:")))
void ext_update_image(int mouse_x, int mouse_y, int mode){
    static drb_upload_pixel_array_fn drb_upload_pixel_array = NULL;
       if (!drb_upload_pixel_array) {
        drb_upload_pixel_array = drb_symbol_lookup("drb_upload_pixel_array");
        if (!drb_upload_pixel_array) {
            return;
    }
  }
    
  int row = 0;  // row (y) within target image
  int col = 0; // col (x) within target image
  int im_row; // row (y) within source image
  int im_col; // col (x) within source image
  Uint32 rgb; // current pixel colour without alpha
  Uint32 output[al_w * al_h]; // pixel array to return

  // loop through pixels in target image
  for (int i = 0; i < (al_w * al_h); i++) {
    col = i % al_w; // calculate row and col of current pixel in target
    row = i / al_w;

    // calculate row/col in source image corresponding to target image pixel
    im_col = col + mouse_x; 
    im_row = row + mouse_y;
    
    // if source image pixel is outside of the source image dimensions...
    if (im_col < 0 || im_col > im_w - 1 || im_row < 0 || im_row > im_h - 1){
      // outgoing RGB value will be grey background colour
      rgb = 0x00CCCCCC;
    }else{ // manipulate the pixel
      // capture the current source pixel and remove the alpha channel
      rgb = image[(im_col) + (im_row * im_w)] - 0xFF000000;
      // capture the the red channel
      Uint32 r = rgb % 256;
      // zero the red channel
      rgb -= r;
      // capture the green channel
      Uint32 g = rgb % 65536;
      // zero the green channel
      rgb -= g;
      // capture the blue channel
      Uint32 b = rgb % 16777216;
      rgb -= b;
      
      switch(mode){
        case 0: // normal, capture source pixel RGB with alpha stripped
          rgb = image[(im_col) + (im_row * im_w)] - 0xFF000000;
          break;
        case 1: // red-channel
          rgb = r;
          break;
        case 2: // green-channel
          rgb = g;
          break;
        case 3: // blue-channel
          rgb = b;
          break;
        case 4: // desaturate (average RGB channels)
          g = g / 256;
          b = b / 65536;
          r = (r + g + b) / 3; // There is a better way to do this
          rgb = ((r * 65536) + (r * 256) + r);
          break;
        case 5: // threshold (snap pixels to black or white)
          g = g / 256;
          b = b / 65536;
          r = (r + g + b) / 3; // There is a better way to do this
          r = r < 128 ? 0 : 255;
          rgb = ((r * 65536) + (r * 256) + r);
          break;
        case 6: // posterize (snap each colour channel in pixel to 0 or 255, yielding 8 possible colours)
          g = g / 256;
          b = b / 65536;
          r = r < 128 ? 0 : 255;
          g = g < 128 ? 0 : 255;
          b = b < 128 ? 0 : 255;
          rgb = ((b * 65536) + (g * 256) + r);
          break;
      }
    }
    output[i] = alpha[i] + rgb; // combine modified source RGB with target alpha
  }

  drb_upload_pixel_array("lena", al_w, al_h, output); // upload pixel array to DragonRuby
}
