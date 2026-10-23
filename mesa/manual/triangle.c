#include <GL/osmesa.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
  if (argc != 2) {
    fputs("usage: mesa-triangle OUTPUT.ppm\n", stderr);
    return 1;
  }
  const int width = 256, height = 256;
  unsigned char *pixels = calloc((size_t)width * height, 4);
  OSMesaContext context = OSMesaCreateContextExt(OSMESA_RGBA, 24, 8, 0, NULL);
  if (!pixels || !context || !OSMesaMakeCurrent(context, pixels, GL_UNSIGNED_BYTE, width, height)) {
    fputs("mesa-triangle: context or buffer allocation failed\n", stderr);
    if (context) OSMesaDestroyContext(context);
    free(pixels);
    return 1;
  }
  OSMesaPixelStore(OSMESA_Y_UP, 0);
  const char *renderer = (const char *)glGetString(GL_RENDERER);
  if (!renderer || !strstr(renderer, "softpipe") ||
      !OSMesaGetProcAddress("glClear") || OSMesaGetProcAddress("OSMesaPostprocess")) {
    fputs("mesa-triangle: renderer or procedure profile mismatch\n", stderr);
    OSMesaDestroyContext(context);
    free(pixels);
    return 1;
  }
  printf("MESA_RENDERER %s\n", renderer);
  glViewport(0, 0, width, height);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBegin(GL_TRIANGLES);
  glColor3f(1, 0, 0); glVertex2f(-0.8f, -0.8f);
  glColor3f(0, 1, 0); glVertex2f(0.8f, -0.8f);
  glColor3f(0, 0, 1); glVertex2f(0, 0.8f);
  glEnd();
  glFinish();
  unsigned char *center = pixels + 4 * ((height / 2) * width + width / 2);
  int failed = glGetError() != GL_NO_ERROR ||
      pixels[0] || pixels[1] || pixels[2] ||
      center[0] < 10 || center[1] < 10 || center[2] < 10;
  FILE *file = failed ? NULL : fopen(argv[1], "wb");
  if (!file) failed = 1;
  if (file) {
    if (fprintf(file, "P6\n%d %d\n255\n", width, height) < 0) failed = 1;
    for (int i = 0; !failed && i < width * height; ++i) {
      if (fwrite(pixels + 4 * i, 1, 3, file) != 3) failed = 1;
    }
    if (fclose(file)) failed = 1;
  }
  OSMesaDestroyContext(context);
  free(pixels);
  if (failed) {
    fputs("mesa-triangle: render or output failed\n", stderr);
    return 1;
  }
  puts("MESA_TRIANGLE COMPLETE width=256 height=256 postprocess=UNAVAILABLE jit=NONE");
  return 0;
}
