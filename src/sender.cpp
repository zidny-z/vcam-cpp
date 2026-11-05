#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <windows.h>
#include <softcam/softcam.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const int W = 640;
const int H = 480;
const float FPS = 20.0f;

static volatile std::sig_atomic_t g_running = 1;

static void StopRunning(int) {
    g_running = 0;
}

static std::string ExecutableDirectory() {
    char path[MAX_PATH] = {};
    const DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return std::string();
    }
    std::string full(path, len);
    const std::size_t slash = full.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : full.substr(0, slash + 1);
}

static unsigned char* LoadPhoto(int* width, int* height, int* channels) {
    const std::string dir = ExecutableDirectory();
    const std::string candidates[] = {
        dir + "fotoorang2.jpeg",
        dir + "assets\\fotoorang2.jpeg",
        "assets/fotoorang2.jpeg",
        "fotoorang2.jpeg",
    };

    for (const std::string& path : candidates) {
        if (path.empty()) {
            continue;
        }
        unsigned char* pixels = stbi_load(path.c_str(), width, height, channels, 3);
        if (pixels) {
            std::printf("Loaded image: %s (%dx%d)\n", path.c_str(), *width, *height);
            return pixels;
        }
    }
    return nullptr;
}

int main() {
    int iw = 0, ih = 0, n = 0;
    unsigned char* rgb = LoadPhoto(&iw, &ih, &n);
    if (!rgb) {
        char cwd[MAX_PATH] = {};
        GetCurrentDirectoryA(MAX_PATH, cwd);
        std::fprintf(stderr, "Error: fotoorang2.jpeg failed to load (%s).\n",
                     stbi_failure_reason());
        std::fprintf(stderr, "Place fotoorang2.jpeg next to sender.exe, in an assets\\ subfolder, "
                             "or run sender.exe from the folder that contains it.\n");
        std::fprintf(stderr, "Current directory: %s\n", cwd);
        return 1;
    }

    unsigned char* resized = static_cast<unsigned char*>(std::malloc(W * H * 3));
    if (!resized) {
        std::fprintf(stderr, "Error: Memory allocation failed.\n");
        stbi_image_free(rgb);
        return 1;
    }

    if (!stbir_resize_uint8(rgb, iw, ih, 0, resized, W, H, 0, 3)) {
        std::fprintf(stderr, "Error: Image resizing failed.\n");
        std::free(resized);
        stbi_image_free(rgb);
        return 1;
    }
    stbi_image_free(rgb);

    std::vector<unsigned char> bgr(W * H * 3);
    for (int i = 0; i < W * H; i++) {
        bgr[i * 3 + 0] = resized[i * 3 + 2];
        bgr[i * 3 + 1] = resized[i * 3 + 1];
        bgr[i * 3 + 2] = resized[i * 3 + 0];
    }
    std::free(resized);

    scCamera cam = scCreateCamera(W, H, FPS);
    if (!cam) {
        std::fprintf(stderr,
                     "scCreateCamera failed.\n"
                     "  - Register the camera: run register.bat as administrator.\n"
                     "  - Close any other sender.exe instance (only one camera is allowed).\n"
                     "  - Match the architecture of sender.exe and softcam.dll (x64).\n");
        return 1;
    }

    std::signal(SIGINT, StopRunning);
    std::signal(SIGTERM, StopRunning);

    std::printf("Softcam is active. Select \"DirectShow Softcam\" as the camera in your app "
                "(not \"OBS Virtual Camera\"). Sending frames at %.1f FPS. Press Ctrl+C to stop.\n",
                FPS);
    std::fflush(stdout);

    while (g_running) {
        scSendFrame(cam, bgr.data());
    }

    scDeleteCamera(cam);
    return 0;
}
