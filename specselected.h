#ifndef SPECSELECTED_H
#define SPECSELECTED_H
/* GIMP RGBA C-Source image dump (specselected.c) */

struct gimp_image_struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  //unsigned char	 pixel_data[651 * 348 * 4 + 4];
  const unsigned char *pixel_data;
};
typedef struct gimp_image_struct gimp_image_t;

extern const gimp_image_t gimp_image_hp8591a;   // 651, 348, 4,
extern const gimp_image_t gimp_image_pm3295;    // 638, 318, 4,
#endif
