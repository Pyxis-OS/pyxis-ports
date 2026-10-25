#include <cstdio>
#include <cstdlib>
#include <exception>
#include <thread>
#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <imgui.h>
#include "libultraship/libultraship.h"
#include "ship/Context.h"
#include "ship/NativePaths.h"
#include "ship/window/MouseStateManager.h"
#include "ship/config/Config.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/archive/ArchiveManager.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/type/Json.h"
#include "ship/window/gui/resource/GuiTexture.h"
#include "ship/audio/Audio.h"
#include "ship/audio/AudioPlayer.h"
#include "fast/Fast3dWindow.h"
#include "fast/interpreter.h"
#include "fast/resource/ResourceType.h"
#include "fast/resource/factory/TextureFactory.h"
#include "fast/resource/type/Texture.h"
#include "libultraship/libultra/types.h"
#include "libultraship/libultra/gbi.h"

namespace {
class FixtureControls final : public Ship::ControlDeck {
public:
    FixtureControls() : ControlDeck({}, nullptr, {}) {}
    void WriteToPad(void*) override {
        throw std::logic_error("the public UI fixture has no game-pad layout");
    }
};

class FixtureWindow final : public Ship::GuiWindow {
public:
    FixtureWindow() : GuiWindow("", true, "LUS native resource slice", ImVec2(520, 300)) {}
protected:
    void InitElement() override {}
    void UpdateElement() override {}
    void DrawElement() override {
        ImGui::TextUnformatted("Pinned LUS / native SDL2 / Mesa softpipe");
        ImGui::TextUnformatted("Public O2R: parsed JSON, resource workers, FBO + depth");
        ImGui::TextUnformatted("Keyboard/pointer are native; Escape exits cleanly.");
        ImGui::InputText("Text", text, sizeof(text));
        if (ImGui::Button("Count")) ++count;
        ImGui::SameLine(); ImGui::Text("%d", count);
        ImGui::Text("Drawable %.0f x %.0f; scale %.1f / %.1f",
            ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y,
            ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
    }
    char text[128] = "Hello from LUS";
    int count = 0;
};

void require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

void check_depth_fbo() {
    GLuint framebuffer = 0, color = 0, depth = 0;
    GLint old_framebuffer = 0, viewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 16, 16);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "FBO incomplete");
    glViewport(0, 0, 16, 16);
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE);
    glClearDepth(1); glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int pass = 0; pass < 2; ++pass) {
        glColor3f(pass ? 1 : 0, pass ? 0 : 1, 0);
        const float z = pass ? 0.5f : -0.5f;
        glBegin(GL_TRIANGLES);
        glVertex3f(-1, -1, z); glVertex3f(1, -1, z); glVertex3f(0, 1, z);
        glEnd();
    }
    unsigned char pixel[4] = {};
    float pixel_depth = 0;
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glReadPixels(8, 8, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &pixel_depth);
    const bool valid = pixel[1] > 200 && pixel[0] < 10 && std::abs(pixel_depth - 0.25f) < 0.02f;
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, old_framebuffer);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindTexture(GL_TEXTURE_2D, 0); glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer); glDeleteRenderbuffers(1, &depth); glDeleteTextures(1, &color);
    require(valid && glGetError() == GL_NO_ERROR, "FBO/depth readback mismatch");
}
}

int main(int argc, char** argv) {
    std::thread audio;
    std::atomic<bool> stop_audio = false;
    std::atomic<bool> audio_failed = false;
    std::shared_ptr<Fast::Fast3dWindow> window;
    std::shared_ptr<Ship::GuiWindow> gui;
    int status = 1;
    try {
        const std::string archive = argc > 1 ? argv[1] : "app://fixture.o2r";
        auto ctx = Ship::Context::CreateUninitializedInstance("LUS Slice", "lus-slice", "config.json");
        require(ctx->InitLogging() && ctx->InitConfiguration() && ctx->InitConsoleVariables() && ctx->InitConsole(), "state initialization refused");
        gui = std::make_shared<FixtureWindow>();
        window = std::make_shared<Fast::Fast3dWindow>(std::vector<std::shared_ptr<Ship::GuiWindow>>{gui});
        const auto cpus = std::thread::hardware_concurrency();
        const auto reserved = cpus > 2 ? cpus - 2 : 0;
        require(ctx->Init({archive}, {}, reserved, {}, window, std::make_shared<FixtureControls>()), "LUS initialization refused");
        // Exercise application fonts added after the renderer has initialized.
        ImGui::GetIO().Fonts->AddFontDefault();
        window->GetMouseStateManager()->SetAutoCaptureMouse(false);
        window->SetMouseCapture(false);
        SDL_StartTextInput();
        auto manager = ctx->GetResourceManager();
        require(manager->GetResourceLoader()->RegisterResourceFactory(
            std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY,
            "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 0), "texture factory registration failed");
        auto texture = std::dynamic_pointer_cast<Fast::Texture>(manager->LoadResource("fixture/pixel", true));
        require(texture && texture->ImageDataSize == 4 && texture->ImageData[1] == 255, "public texture decode failed");
        auto parsed = std::dynamic_pointer_cast<Ship::Json>(manager->LoadResource("fixture/message.json", true));
        require(parsed && parsed->Data.at("value") == 42, "public O2R resource did not decode");
        auto all = manager->LoadResourcesAsync("fixture/bulk.json");
        auto bulk = all.get();
        require(bulk && bulk->size() == 1, "bulk resource load failed");
        auto bulk_json = std::dynamic_pointer_cast<Ship::Json>(bulk->at(0));
        require(bulk_json && bulk_json->Data.at("value") == 43, "bulk resource decode failed");
        require(manager->LoadResource("fixture/not-present", true) == nullptr, "missing resource unexpectedly loaded");
        auto source_archive = manager->GetArchiveManager()->GetArchiveFromFile("fixture/message.json");
        require(source_archive && !source_archive->WriteFile("unexpected-write", {1, 2, 3}), "read-only archive accepted mutation");
        source_archive.reset();
        Ship::Config read_only("app://readonly.json");
        read_only.SetInt("value", 99);
        require(!read_only.Save(), "read-only config save unexpectedly succeeded");
        Ship::Config unchanged("app://readonly.json");
        require(unchanged.GetInt("value") == 7, "read-only config changed");
        check_depth_fbo();
        fprintf(stderr, "LUS: public resource decode and FBO/depth PASS; GL %s / %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
        auto player = ctx->GetAudio()->GetAudioPlayer();
        require(player != nullptr, "SDL audio player missing");
        audio = std::thread([&, player] {
            try {
                std::array<int16_t, 512 * 2> silence{};
                while (!stop_audio.load()) {
                    if (player->Buffered() < 2048) player->Play(reinterpret_cast<uint8_t*>(silence.data()), sizeof(silence));
                    std::this_thread::sleep_for(std::chrono::milliseconds(8));
                }
            } catch (const std::exception& error) {
                fprintf(stderr, "lus-demo: audio producer: %s\n", error.what());
                audio_failed = true;
            }
        });
        alignas(2) static const char texture_path[] = "__OTR__fixture/pixel";
        Gfx display_list[] = {
            gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_32b, 1, texture_path),
            gsDPSetCycleType(G_CYC_FILL),
            gsDPSetFillColor(0x19331933),
            gsDPFillRectangle(0, 0, 319, 239),
            gsSPEndDisplayList(),
        };
        bool texture_checked = false;
        while (window->IsRunning()) {
            window->HandleEvents();
            if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_ESCAPE]) window->Close();
            require(!audio_failed.load(), "SDL audio producer failed");
            if (window->DrawAndRunGraphicsCommands(display_list, {}) && !texture_checked) {
                auto interpreter = window->GetInterpreterWeak().lock();
                require(interpreter && interpreter->mRdp->texture_to_load.raw_tex_metadata.resource == texture &&
                    interpreter->mRdp->texture_to_load.addr == texture->ImageData, "owned texture dispatch failed");
                texture_checked = true;
                fprintf(stderr, "LUS: resource-owned texture dispatch PASS\n");
            }
            SDL_Delay(8);
        }
        SDL_StopTextInput();
        stop_audio = true;
        audio.join();
        player.reset(); texture.reset(); parsed.reset(); bulk_json.reset(); bulk.reset(); all = {}; manager.reset();
        require(ctx->GetConfig()->Save(), "writable config publication failed");
        // Context owns ordered worker/GUI/GL/audio teardown; release extra frontend owners first.
        window.reset(); gui.reset();
        Ship::Context::DestroyInstance();
        fprintf(stderr, "LUS: shutdown PASS; producer and owned workers joined\n");
        status = 0;
    } catch (const std::exception& error) {
        fprintf(stderr, "lus-demo: %s\n", error.what());
    }
    stop_audio = true;
    if (audio.joinable()) audio.join();
    window.reset(); gui.reset();
    Ship::Context::DestroyInstance();
    return status;
}
