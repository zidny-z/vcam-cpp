#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <windows.h>
#include <softcam/softcam.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

namespace {

const int W = 640;
const int H = 480;
const float FPS = 20.0f;

const wchar_t* const kFilterClsid = L"{AEF3B972-5FA5-4647-9571-358EB472BC9E}";
const wchar_t* const kCategoryClsid = L"{860BB310-5D01-11D0-BD3B-00A0C911CE86}";
const wchar_t* const kFilterName = L"DirectShow Softcam";

// REGFILTER2 blob for the Softcam filter: one output pin, Video/NULL media type.
const BYTE kFilterData[] = {
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x70, 0x69, 0x33, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x74, 0x79, 0x33, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

volatile std::sig_atomic_t g_running = 1;

void StopRunning(int) {
    g_running = 0;
}

std::wstring ExecutableDirectory() {
    wchar_t path[MAX_PATH] = {};
    const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return std::wstring();
    }
    std::wstring full(path, len);
    const std::size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring() : full.substr(0, slash + 1);
}

std::string Narrow(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    const int len = WideCharToMultiByte(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()),
                                        nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()),
                        &result[0], len, nullptr, nullptr);
    return result;
}

bool FileExists(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool SetString(HKEY key, const wchar_t* name, const std::wstring& value) {
    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    return RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
                          bytes) == ERROR_SUCCESS;
}

std::wstring ClassesKey(bool wow32) {
    return wow32 ? L"Software\\Classes\\WOW6432Node\\" : L"Software\\Classes\\";
}

bool RegisterCamera(const std::wstring& dllPath, bool wow32) {
    const std::wstring root = ClassesKey(wow32);
    const std::wstring clsidKey = root + L"CLSID\\" + kFilterClsid;
    const std::wstring inprocKey = clsidKey + L"\\InprocServer32";
    const std::wstring instanceKey =
        root + L"CLSID\\" + kCategoryClsid + L"\\Instance\\" + kFilterClsid;

    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, clsidKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr,
                        &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const bool named = SetString(key, nullptr, kFilterName);
    RegCloseKey(key);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, inprocKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr,
                        &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const bool located = SetString(key, nullptr, dllPath) &&
                         SetString(key, L"ThreadingModel", L"Both");
    RegCloseKey(key);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, instanceKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr,
                        &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const bool instanced =
        SetString(key, L"FriendlyName", kFilterName) &&
        SetString(key, L"CLSID", kFilterClsid) &&
        RegSetValueExW(key, L"FilterData", 0, REG_BINARY, kFilterData, sizeof(kFilterData)) ==
            ERROR_SUCCESS;
    RegCloseKey(key);
    return named && located && instanced;
}

void UnregisterCamera(bool wow32) {
    const std::wstring root = ClassesKey(wow32);
    const std::wstring clsidKey = root + L"CLSID\\" + kFilterClsid;
    const std::wstring instanceKey =
        root + L"CLSID\\" + kCategoryClsid + L"\\Instance\\" + kFilterClsid;
    RegDeleteTreeW(HKEY_CURRENT_USER, instanceKey.c_str());
    RegDeleteTreeW(HKEY_CURRENT_USER, clsidKey.c_str());
}

unsigned char* LoadPhoto(const std::string& dir, int* width, int* height, int* channels) {
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

using CreateCameraFn = scCamera(SOFTCAM_API*)(int, int, float);
using DeleteCameraFn = void(SOFTCAM_API*)(scCamera);
using SendFrameFn = void(SOFTCAM_API*)(scCamera, const void*);

struct SoftcamApi {
    HMODULE module = nullptr;
    CreateCameraFn create = nullptr;
    DeleteCameraFn destroy = nullptr;
    SendFrameFn send = nullptr;

    ~SoftcamApi() {
        if (module) {
            FreeLibrary(module);
        }
    }
};

bool LoadSoftcam(const std::string& dllPath, SoftcamApi* api) {
    api->module = LoadLibraryA(dllPath.c_str());
    if (!api->module) {
        std::fprintf(stderr, "Error: cannot load %s (Win32 error %lu).\n", dllPath.c_str(),
                     GetLastError());
        return false;
    }
    api->create = reinterpret_cast<CreateCameraFn>(GetProcAddress(api->module, "scCreateCamera"));
    api->destroy = reinterpret_cast<DeleteCameraFn>(GetProcAddress(api->module, "scDeleteCamera"));
    api->send = reinterpret_cast<SendFrameFn>(GetProcAddress(api->module, "scSendFrame"));
    if (!api->create || !api->destroy || !api->send) {
        std::fprintf(stderr, "Error: %s does not export the Softcam sender API.\n",
                     dllPath.c_str());
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    const std::wstring dirW = ExecutableDirectory();
    const std::string dirA = Narrow(dirW);
    const std::wstring dll64W = dirW + L"softcam.dll";
    const std::wstring dll86W = dirW + L"softcam_x86.dll";

    bool unregister = false;
    bool registerOnly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--unregister") == 0 || std::strcmp(argv[i], "-u") == 0) {
            unregister = true;
        } else if (std::strcmp(argv[i], "--register") == 0 || std::strcmp(argv[i], "-r") == 0) {
            registerOnly = true;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::printf("Usage: sender.exe [--register | --unregister]\n");
            return 0;
        }
    }

    if (unregister) {
        UnregisterCamera(false);
        UnregisterCamera(true);
        std::printf("DirectShow Softcam unregistered for this user.\n");
        return 0;
    }

    if (!FileExists(dll64W)) {
        std::fprintf(stderr, "Error: softcam.dll not found next to sender.exe.\n");
        return 1;
    }

    const bool registered = RegisterCamera(dll64W, false);
    if (FileExists(dll86W)) {
        RegisterCamera(dll86W, true);
    }

    if (registerOnly) {
        if (!registered) {
            std::fprintf(stderr, "Error: failed to register the camera (Win32 error %lu).\n",
                         GetLastError());
            return 1;
        }
        std::printf("DirectShow Softcam registered for this user.\n");
        return 0;
    }

    if (!registered) {
        std::fprintf(stderr,
                     "Warning: per-user camera registration failed (Win32 error %lu); "
                     "continuing in case it is already registered.\n",
                     GetLastError());
    }

    int iw = 0, ih = 0, n = 0;
    unsigned char* rgb = LoadPhoto(dirA, &iw, &ih, &n);
    if (!rgb) {
        char cwd[MAX_PATH] = {};
        GetCurrentDirectoryA(MAX_PATH, cwd);
        std::fprintf(stderr, "Error: fotoorang2.jpeg failed to load (%s).\n",
                     stbi_failure_reason());
        std::fprintf(stderr, "Place fotoorang2.jpeg next to sender.exe or in an assets\\ "
                             "subfolder.\n");
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

    SoftcamApi api;
    if (!LoadSoftcam(Narrow(dll64W), &api)) {
        return 1;
    }

    scCamera cam = api.create(W, H, FPS);
    if (!cam) {
        std::fprintf(stderr,
                     "scCreateCamera failed.\n"
                     "  - Close any other sender.exe instance (only one camera is allowed).\n"
                     "  - Make sure softcam.dll matches this executable (x64).\n");
        return 1;
    }

    std::signal(SIGINT, StopRunning);
    std::signal(SIGTERM, StopRunning);

    std::printf("Softcam is active. Select \"DirectShow Softcam\" as the camera in your app "
                "(not \"OBS Virtual Camera\"). Sending frames at %.1f FPS. Press Ctrl+C to stop.\n",
                FPS);
    std::fflush(stdout);

    while (g_running) {
        api.send(cam, bgr.data());
    }

    api.destroy(cam);
    return 0;
}
