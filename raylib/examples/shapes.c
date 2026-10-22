/* Editable raylib drawing example; no external assets or audio. */
#include <raylib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int dimension(const char *text)
{
  char *end;
  errno = 0;
  long value = strtol(text, &end, 10);
  return !errno && *text && !*end && value >= 320 && value <= 4096 ? (int)value : 0;
}

int main(int argc, char **argv)
{
  int width = 960, height = 540;
  if (argc != 1 && argc != 3) {
    fprintf(stderr, "usage: raylib-shapes [width height]\n");
    return 1;
  }
  if (argc == 3) {
    width = dimension(argv[1]);
    height = dimension(argv[2]);
    if (!width || !height) {
      fprintf(stderr, "raylib-shapes: dimensions must be 320..4096\n");
      return 1;
    }
  }
  InitWindow(width, height, "raylib / Pyxis");
  if (!IsWindowReady()) {
    fprintf(stderr, "raylib-shapes: display initialization failed\n");
    CloseWindow();
    return 1;
  }
  SetTargetFPS(60);
  Image image = GenImageChecked(96, 96, 12, 12, SKYBLUE, DARKBLUE);
  Texture2D checker = LoadTextureFromImage(image);
  UnloadImage(image);
  if (!checker.id) {
    fprintf(stderr, "raylib-shapes: checker texture failed\n");
    CloseWindow();
    return 1;
  }
  bool paused = false, three_d = false;
  float angle = 0.0f, radius = 35.0f;
  double started = GetTime();
  unsigned long frames = 0;
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_F1)) three_d = !three_d;
    if (IsKeyPressed(KEY_L)) {
      if (IsCursorHidden()) EnableCursor();
      else DisableCursor();
    }
    if (IsKeyPressed(KEY_W)) SetMousePosition(width/2, height/2);
    if (!paused) angle += GetFrameTime()*45.0f;
    radius += GetMouseWheelMove()*2.0f;
    if (radius < 10.0f) radius = 10.0f;
    if (radius > 80.0f) radius = 80.0f;
    BeginDrawing();
    ClearBackground((Color){ 18, 24, 38, 255 });
    if (three_d) {
      Camera3D camera = { .position = { 5, 4, 6 }, .target = { 0, 0, 0 },
          .up = { 0, 1, 0 }, .fovy = 45, .projection = CAMERA_PERSPECTIVE };
      BeginMode3D(camera);
      /* Draw the nearer cube first: later distant faces must stay behind it. */
      DrawCube((Vector3){ 1, 1, 1 }, 2, 2, 2, ORANGE);
      DrawCube((Vector3){ -0.75f, 1, -0.75f }, 2, 2, 2, SKYBLUE);
      EndMode3D();
    } else {
      DrawRectangle(45, 140, 180, 110, BLUE);
      DrawRectangleLines(40, 135, 190, 120, SKYBLUE);
      DrawCircle(310, 195, radius, GOLD);
      DrawTriangle((Vector2){ 475, 140 }, (Vector2){ 420, 250 }, (Vector2){ 530, 250 }, GREEN);
      DrawTexturePro(checker, (Rectangle){ 0, 0, 96, 96 },
          (Rectangle){ width*0.72f, height*0.42f, 96, 96 }, (Vector2){ 48, 48 }, angle, WHITE);
      DrawLine(40, 285, width - 40, 285, GRAY);
      DrawText("Shapes, checker texture and the built-in font", 40, 310, 20, RAYWHITE);
    }
    Vector2 pointer = GetMousePosition();
    if (IsCursorOnScreen() && pointer.x >= 0 && pointer.y >= 0) {
      Color color = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? GREEN : MAGENTA;
      DrawCircleLines((int)pointer.x, (int)pointer.y, 12, color);
      DrawLine((int)pointer.x - 16, (int)pointer.y, (int)pointer.x + 16, (int)pointer.y, color);
    }
    DrawText("raylib on Pyxis", 40, 35, 30, RAYWHITE);
    DrawText("Space: pause   F1: 2D / 3D   Wheel: circle size   Esc: exit", 40, 80, 18, LIGHTGRAY);
    DrawText("L: cursor lock   W: request centre position", 40, 110, 18, LIGHTGRAY);
    DrawText(TextFormat("%d x %d canvas | %d FPS", GetScreenWidth(), GetScreenHeight(), GetFPS()),
        40, height - 45, 20, RAYWHITE);
    DrawRectangle(0, 0, 12, 12, RED);
    DrawRectangle(width - 12, 0, 12, 12, GREEN);
    DrawRectangle(0, height - 12, 12, 12, BLUE);
    DrawRectangle(width - 12, height - 12, 12, 12, YELLOW);
    EndDrawing();
    ++frames;
  }
  double elapsed = GetTime() - started;
  fprintf(stderr, "raylib-shapes: canvas=%dx%d output=%dx%d frames=%lu elapsed=%.3f s",
      width, height, GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()), frames, elapsed);
  if (elapsed > 0.0) fprintf(stderr, "; average %.1f FPS", frames/elapsed);
  fputc('\n', stderr);
  UnloadTexture(checker);
  CloseWindow();
  return 0;
}
