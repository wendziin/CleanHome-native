#ifndef CLEANHOME_IMAGE_H
#define CLEANHOME_IMAGE_H

#include "render.h"

typedef struct Images Images;

int image_init(Images **out);
void image_shutdown(Images *images);
GLuint image_get(Images *images, const char *path, int *width, int *height);

#endif
