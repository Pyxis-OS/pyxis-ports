// SPDX-License-Identifier: MPL-2.0
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>

int main()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    std::fprintf(stderr, "imgui-demo: SDL initialization: %s\n", SDL_GetError());
    return 1;
  }
  SDL_DisplayMode output;
  if (SDL_GetCurrentDisplayMode(0, &output) != 0) {
    std::fprintf(stderr, "imgui-demo: output size: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  SDL_Window *window = SDL_CreateWindow("Dear ImGui on Pyxis", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, output.w, output.h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext gl = window ? SDL_GL_CreateContext(window) : nullptr;
  if (!gl || SDL_GL_MakeCurrent(window, gl) != 0 || SDL_GL_SetSwapInterval(0) != 0) {
    std::fprintf(stderr, "imgui-demo: OpenGL initialization: %s\n", SDL_GetError());
    if (gl) SDL_GL_DeleteContext(gl);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  std::fprintf(stderr, "imgui-demo: %s; %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  io.IniFilename = nullptr;
  ImGui::StyleColorsDark();
  bool platform_ready = ImGui_ImplSDL2_InitForOpenGL(window, gl);
  bool renderer_ready = platform_ready && ImGui_ImplOpenGL3_Init("#version 330");
  bool device_ready = renderer_ready && ImGui_ImplOpenGL3_CreateDeviceObjects();
  if (!device_ready) {
    std::fprintf(stderr, "imgui-demo: ImGui backend initialization failed: %s\n", SDL_GetError());
    if (renderer_ready) ImGui_ImplOpenGL3_Shutdown();
    if (platform_ready) ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  bool done = false, show_demo = false;
  int result = 0;
  int clicks = 0;
  float slider = 0.5f;
  char text[128] = "Hello from Pyxis";
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
        done = true;
    }
    if (done) break;
    if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &output) != 0) {
      std::fprintf(stderr, "imgui-demo: output size: %s\n", SDL_GetError());
      result = 1;
      break;
    }
    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    if (width != output.w || height != output.h) {
      SDL_SetWindowSize(window, output.w, output.h);
      SDL_GL_GetDrawableSize(window, &width, &height);
      if (width != output.w || height != output.h) {
        std::fprintf(stderr, "imgui-demo: cannot follow output size: %s\n", SDL_GetError());
        result = 1;
        break;
      }
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(28, 28), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(570, 410), ImGuiCond_FirstUseEver);
    ImGui::Begin("Dear ImGui on Pyxis");
    ImGui::Text("Dear ImGui %s / SDL2 / Mesa software OpenGL", IMGUI_VERSION);
    ImGui::Separator();
    ImGui::TextWrapped("A reusable native C++ UI library. Keyboard navigation, text input, pointer and resizing use the declared SDL2 grants.");
    ImGui::InputText("Text", text, sizeof(text));
    ImGui::SliderFloat("Value", &slider, 0.0f, 1.0f);
    if (ImGui::Button("Click me")) ++clicks;
    ImGui::SameLine();
    ImGui::Text("%d clicks", clicks);
    ImGui::Checkbox("Show upstream demo", &show_demo);
    ImGui::Text("Canvas follows output 1:1; resize the output to check.");
    ImGui::Text("Drawable: %d x %d | %.1f frames/s", width, height, io.Framerate);
    ImGui::TextWrapped("Tab and arrows navigate; Escape exits. Clipboard is local to this ImGui context. External clipboard, gamepads and extra platform windows are not enabled.");
    ImGui::End();
    if (show_demo) ImGui::ShowDemoWindow(&show_demo);
    ImGui::Render();
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
    SDL_Delay(8);
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}
