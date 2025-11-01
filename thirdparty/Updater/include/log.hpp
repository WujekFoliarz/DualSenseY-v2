#ifndef LOG_HPP
#define LOG_HPP

#ifdef __ANDROID__
// LOGS ANDROID
#           include <android/log.h>
#           define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE,"APP",__VA_ARGS__)
#           define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  ,"APP",__VA_ARGS__)
#           define LOGI(...) __android_log_print(ANDROID_LOG_INFO   ,"APP",__VA_ARGS__)
#           define LOGW(...) __android_log_print(ANDROID_LOG_WARN   ,"APP",__VA_ARGS__)
#           define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  ,"APP",__VA_ARGS__)
#           define LOGSIMPLE(...)
#       else
#           include <stdio.h>
#		define LOGV(...) do { printf("[VERBOSE] " __VA_ARGS__); printf("\n"); } while (0)
#		define LOGD(...) do { printf("[DEBUG] " __VA_ARGS__); printf("\n"); } while (0)
#		define LOGI(...) do { printf("[INFO] " __VA_ARGS__); printf("\n"); } while (0)
#		define LOGW(...) do { printf("[WARNING] " __VA_ARGS__); printf("\n"); } while (0)
#		define LOGE(...) do { printf("[ERROR] " __VA_ARGS__); printf("\n"); } while (0)
#		define LOGSIMPLE(...) do { printf(__VA_ARGS__); printf("\n"); } while (0)
#       endif 

#endif // LOG_HPP