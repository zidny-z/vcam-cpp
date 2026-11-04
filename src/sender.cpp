#include <cstdio>
#include <cstdlib>
#include <vector>

#include <softcam/softcam.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const int W = 640;
const int H = 480;
const float FPS = 20.0f;

int main() {
    // 1. Load gambar (setara cv2.imread)
    int iw, ih, n;
    unsigned char* rgb = stbi_load("fotoorang2.jpeg", &iw, &ih, &n, 3);
    if (!rgb) {
        std::fprintf(stderr, "fotoorang2.jpeg gagal dimuat\n");
        return 1;
    }
    std::printf("Loaded: %dx%d\n", iw, ih);

    // 2. Resize ke 640x480 (setara cv2.resize INTER_AREA)
    unsigned char* resized = (unsigned char*)std::malloc(W * H * 3);
    if (!resized) {
        std::fprintf(stderr, "malloc gagal\n");
        stbi_image_free(rgb);
        return 1;
    }
    stbir_resize_uint8(rgb, iw, ih, 0, resized, W, H, 0, 3);
    stbi_image_free(rgb);

    // 3. RGB -> BGR (Softcam/DirectShow memakai BGR)
    std::vector<unsigned char> bgr(W * H * 3);
    for (int i = 0; i < W * H; i++) {
        bgr[i * 3 + 0] = resized[i * 3 + 2];  // B
        bgr[i * 3 + 1] = resized[i * 3 + 1];  // G
        bgr[i * 3 + 2] = resized[i * 3 + 0];  // R
    }
    std::free(resized);

    // 4. Buat virtual camera
    scCamera cam = scCreateCamera(W, H, FPS);
    if (!cam) {
        std::fprintf(stderr, "scCreateCamera gagal — pastikan softcam.dll sudah diregistrasi\n");
        return 1;
    }
    std::printf("Ok, Softcam is now active. Tekan Ctrl+C untuk stop.\n");

    // 5. Loop kirim frame.
    //    scCreateCamera sudah mengatur timing FPS, jadi tidak perlu sleep manual.
    for (;;) {
        scSendFrame(cam, bgr.data());
    }

    scDeleteCamera(cam);
    return 0;
}
