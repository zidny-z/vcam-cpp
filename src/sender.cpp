#include <cstdio>
#include <cstdlib>
#include <vector>

#include <softcam.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const int W = 640;
const int H = 480;
const float FPS = 20.0f;

int main() {
    int iw, ih, n;
    unsigned char* rgb = stbi_load("fotoorang2.jpeg", &iw, &ih, &n, 3);
    if (!rgb) {
        std::fprintf(stderr, "fotoorang2.jpeg gagal dimuat\n");
        return 1;
    }

    unsigned char* resized = (unsigned char*)std::malloc(W * H * 3);
    if (!resized) {
        std::fprintf(stderr, "malloc gagal\n");
        stbi_image_free(rgb);
        return 1;
    }
    stbir_resize_uint8(rgb, iw, ih, 0, resized, W, H, 0, 3);
    stbi_image_free(rgb);

    std::vector<unsigned char> bgr(W * H * 3);
    for (int i = 0; i < W * H; i++) {
        bgr[i * 3 + 0] = resized[i * 3 + 2];  // B
        bgr[i * 3 + 1] = resized[i * 3 + 1];  // G
        bgr[i * 3 + 2] = resized[i * 3 + 0];  // R
    }
    std::free(resized);

    scCamera cam = scCreateCamera(W, H, FPS);
    if (!cam) {
        std::fprintf(stderr, "scCreateCamera gagal — pastikan softcam.dll sudah diregistrasi\n");
        return 1;
    }
    std::printf("Ok, Softcam is now active. Tekan Ctrl+C untuk stop.\n");

    for (;;) {
        scSendFrame(cam, bgr.data());
    }

    scDeleteCamera(cam);
    return 0;
}
