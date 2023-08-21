#include <cstdio>
#include <vector>

#define TAG "NativeDsp"

//#define LOG_VERBOSE

#ifdef __ANDROID__

#include <android/log.h>

#ifdef LOG_VERBOSE
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#else
#define ALOGV(...) do { } while (0)
#endif

#ifdef NDEBUG
#define ALOGD(...) do { } while (0)
#else
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#endif

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN,  TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#else

#ifdef LOG_VERBOSE
#define ALOGV(...) std::printf("[VERBOSE] " fmt, __VA_ARGS__)
#else
#define ALOGV(...) do { } while (0)
#endif

#ifdef NDEBUG
#define ALOGD(fmt, ...) do { } while (0)
#else
#define ALOGD(fmt, ...) std::printf("[DEBUG] " fmt, __VA_ARGS__)
#endif

#define ALOGI(fmt, ...) std::printf("[INFO] " fmt,  __VA_ARGS__)
#define ALOGW(fmt, ...) std::printf("[WARN] " fmt,  __VA_ARGS__)
#define ALOGE(fmt, ...) std::printf("[ERROR] " fmt, __VA_ARGS__)

#endif

namespace fxdsp {

void print_floats(const std::vector<float>& values);

}
