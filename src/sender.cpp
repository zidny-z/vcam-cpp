#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <chrono>

#include <softcam/softcam.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const int W = 640;
const int H = 480;
const float FPS = 20.0f;

int main() {
    int iw = 0, ih = 0, n = 0;

    // Attempt loading from assets directory first, then fallback to working directory
    unsigned char* rgb = stbi_load("assets/fotoorang2.jpeg", &iw, &ih, &n, 3);
    if (!rgb) {
        rgb = stbi_load("fotoorang2.jpeg", &iw, &ih, &n, 3);
    }
    if (!rgb) {
        std::fprintf(stderr, "Error: fotoorang2.jpeg failed to load.\n");
        return 1;
    }

    unsigned char* resized = (unsigned char*)std::malloc(W * H * 3);
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

    // Convert RGB to BGR format required by Softcam
    std::vector<unsigned char> bgr(W * H * 3);
    for (int i = 0; i < W * H; i++) {
        bgr[i * 3 + 0] = resized[i * 3 + 2];  // B
        bgr[i * 3 + 1] = resized[i * 3 + 1];  // G
        bgr[i * 3 + 2] = resized[i * 3 + 0];  // R
    }
    std::free(resized);

    scCamera cam = scCreateCamera(W, H, FPS);
    if (!cam) {
        std::fprintf(stderr, "scCreateCamera failed — ensure softcam.dll is registered.\n");
        return 1;
    }
    std::printf("Softcam is active. Sending frames at %.1f FPS. Press Ctrl+C to stop.\n", FPS);

    // Frame pacing to prevent 100% CPU usage
    const auto frame_delay = std::chrono::milliseconds(static_cast<int>(1000.0f / FPS));
    while (true) {
        scSendFrame(cam, bgr.data());
        std::this_thread::sleep_for(frame_delay);
    }

    scDeleteCamera(cam);
    return 0;
}
