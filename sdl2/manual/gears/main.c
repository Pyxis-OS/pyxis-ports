/* Small asset-free desktop OpenGL example, using the ordinary SDL GL API. */
#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>
#include <stdio.h>

static const float pi = 3.14159265358979323846f;

static void point(float radius, float angle, float z)
{
  glVertex3f(radius*cosf(angle), radius*sinf(angle), z);
}

static void gear(float inner, float root, float tip, float width, int teeth)
{
  int sides = teeth*4;
  float step = 2*pi/(float)sides;
  glBegin(GL_QUADS);
  for (int i = 0; i < sides; ++i) {
    float a = step*i, b = step*(i + 1);
    float ra = (i%4 == 1 || i%4 == 2) ? tip : root;
    float rb = ((i + 1)%4 == 1 || (i + 1)%4 == 2) ? tip : root;
    glNormal3f(0, 0, 1);
    point(inner, a, width/2); point(ra, a, width/2);
    point(rb, b, width/2); point(inner, b, width/2);
    glNormal3f(0, 0, -1);
    point(inner, b, -width/2); point(rb, b, -width/2);
    point(ra, a, -width/2); point(inner, a, -width/2);
    float dx = rb*cosf(b) - ra*cosf(a);
    float dy = rb*sinf(b) - ra*sinf(a);
    glNormal3f(dy, -dx, 0);
    point(ra, a, width/2); point(ra, a, -width/2);
    point(rb, b, -width/2); point(rb, b, width/2);
    glNormal3f(-cosf((a+b)/2), -sinf((a+b)/2), 0);
    point(inner, b, width/2); point(inner, b, -width/2);
    point(inner, a, -width/2); point(inner, a, width/2);
  }
  glEnd();
}

/* Original 5x7 bitmap lettering, drawn as ordinary GL geometry. */
static const Uint8 letters[][7] = {
  {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
  {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
  {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
  {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
  {14,4,4,4,4,4,14}, {7,2,2,2,2,18,12},
  {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
  {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
  {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
  {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
  {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
  {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
  {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
};

static void text(float x, float y, float scale, const char *message)
{
  glBegin(GL_QUADS);
  for (; *message; ++message, x += 6*scale) {
    unsigned char c = (unsigned char)*message;
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    for (int row = 0; row < 7; ++row) {
      unsigned bits = c >= 'A' && c <= 'Z' ? letters[c - 'A'][row] :
          c == ':' && (row == 2 || row == 5) ? 4 :
          c == '-' && row == 3 ? 14 :
          c == '_' && row == 6 ? 31 : 0;
      for (int col = 0; col < 5; ++col) {
        if (!(bits & (16u >> col))) continue;
        float left = x + col*scale, top = y + row*scale;
        glVertex2f(left, top); glVertex2f(left + scale, top);
        glVertex2f(left + scale, top + scale); glVertex2f(left, top + scale);
      }
    }
  }
  glEnd();
}

static void draw(int width, int height, float angle, float tilt, float turn,
    const char *renderer, bool help)
{
  glViewport(0, 0, width, height);
  glClearColor(0.035f, 0.047f, 0.067f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float aspect = (float)width/(float)height;
  glFrustum(-aspect, aspect, -1, 1, 3, 60);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0, 0, -27);
  glRotatef(tilt, 1, 0, 0);
  glRotatef(turn, 0, 1, 0);
  const GLfloat light[] = {-4, 6, 10, 0};
  glLightfv(GL_LIGHT0, GL_POSITION, light);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  glPushMatrix();
  glTranslatef(-2.15f, -1.15f, 0);
  glRotatef(angle, 0, 0, 1);
  glColor3f(0.95f, 0.42f, 0.20f);
  gear(0.7f, 2.55f, 2.95f, 0.75f, 20);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(2.15f, -1.15f, 0);
  glRotatef(-2*angle - 9, 0, 0, 1);
  glColor3f(0.12f, 0.72f, 0.67f);
  gear(0.45f, 1.2f, 1.6f, 1.0f, 10);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-2.15f, 3.15f, 0);
  glRotatef(-2*angle - 9, 0, 0, 1);
  glColor3f(0.28f, 0.48f, 0.92f);
  gear(0.45f, 1.2f, 1.6f, 0.65f, 10);
  glPopMatrix();

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  float scale = width >= 800 ? 2.0f : 1.0f;
  glColor3f(0.86f, 0.91f, 0.98f);
  text(28, 27, scale, "PYXIS - OPENGL GEARS");
  glColor3f(0.49f, 0.62f, 0.75f);
  char label[96];
  snprintf(label, sizeof(label), "GL_RENDERER: %s", renderer);
  text(28, height - 30, scale, label);
  if (help) {
    glColor3f(0.64f, 0.72f, 0.81f);
    text(28, 52, 1, "DRAG OR ARROWS: ROTATE   SPACE: PAUSE   C: CANVAS   H: HELP   ESC: QUIT");
  }
}

int main(void)
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "sdl-gears: %s\n", SDL_GetError());
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  /* Leave SDL's ordinary double-buffer default in place. */
  SDL_Window *window = SDL_CreateWindow("OpenGL gears", 0, 0, 960, 640,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext context = window ? SDL_GL_CreateContext(window) : NULL;
  if (!context) {
    fprintf(stderr, "sdl-gears: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  const char *renderer = (const char *)glGetString(GL_RENDERER);
  const char *version = (const char *)glGetString(GL_VERSION);
  int double_buffer = 0;
  if (!renderer || !version || SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &double_buffer) < 0 ||
      !double_buffer) {
    fprintf(stderr, "sdl-gears: double-buffered GL context unavailable\n");
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  char title[128];
  snprintf(title, sizeof(title), "OpenGL gears - GL_RENDERER: %s", renderer);
  SDL_SetWindowTitle(window, title);
  printf("sdl-gears: GL_RENDERER=%s GL_VERSION=%s doublebuffer=%d\n", renderer, version, double_buffer);
  bool running = true, paused = false, help = true;
  int width = 960, height = 640;
  float angle = 12, tilt = 25, turn = -18;
  Uint64 previous = SDL_GetPerformanceCounter(), started = previous, frames = 0;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE: running = false; break;
          case SDLK_SPACE: paused = !paused; break;
          case SDLK_h: help = !help; break;
          case SDLK_c:
            width = width == 960 ? 640 : 960;
            height = width == 960 ? 640 : 480;
            SDL_SetWindowSize(window, width, height);
            break;
          case SDLK_LEFT: turn -= 5; break;
          case SDLK_RIGHT: turn += 5; break;
          case SDLK_UP: tilt -= 5; break;
          case SDLK_DOWN: tilt += 5; break;
        }
      }
      if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
        turn += event.motion.xrel*0.35f;
        tilt += event.motion.yrel*0.35f;
      }
      if (event.type == SDL_WINDOWEVENT &&
          (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
           event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
           event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
        printf("sdl-gears: window event %u logical=%dx%d\n", event.window.event,
            event.window.data1, event.window.data2);
      }
    }
    Uint64 now = SDL_GetPerformanceCounter();
    if (!paused) angle += (float)((double)(now - previous)/SDL_GetPerformanceFrequency())*32;
    previous = now;
    SDL_GL_GetDrawableSize(window, &width, &height);
    draw(width, height, angle, tilt, turn, renderer, help);
    SDL_ClearError();
    SDL_GL_SwapWindow(window);
    if (*SDL_GetError() || glGetError() != GL_NO_ERROR) {
      fprintf(stderr, "sdl-gears: presentation failed: %s\n", SDL_GetError());
      SDL_GL_DeleteContext(context);
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
    ++frames;
    SDL_Delay(1);
  }
  double elapsed = (double)(SDL_GetPerformanceCounter() - started)/SDL_GetPerformanceFrequency();
  printf("sdl-gears: frames=%llu elapsed=%.3f s\n", (unsigned long long)frames, elapsed);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
