//
// Created by xjsd on 2023/8/22.
//

#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H
#include <fstream>
#include<strstream>
#include <fcntl.h>
#include<sys/stat.h>
#include "file_utils.h"
#define TAG "NATIVE_LOGGER"

#include<sys/mman.h>
#include<android/log.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MAX_CACHE_SIZE 409600
typedef ::size_t ints;

//#define LOGV(A,...) std::printf(__VA_ARGS__)
//#define LOGD(A,...) std::printf(__VA_ARGS__)
//#define LOGI(A,...) std::printf(__VA_ARGS__)
//#define LOGW(A,...) std::printf(__VA_ARGS__)
//#define LOGE(A,...) std::printf(__VA_ARGS__)

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,__VA_ARGS__)

#define PRINTLN(m) std::cout<<"mapindex:"<<m<<std::endl

typedef   void(*callback)(char*); //定义了一个callback的函数指针


using namespace  std;

typedef enum LogLevel{
    LOG_VERBOSE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} level;




class Logger{
public:
      Logger(const char *name,const char* cacheDir,const char* logDir, int maxFileSize, const callback &callback);
     ~Logger();
     void release();
     void flush();
     void log(const char *const &msg);
     callback  oversize= nullptr;

private:
    int cacheFd=0;
    char* logPath;
    char* cacheDir;
    char* logDir;
    int8_t * ptr= nullptr;
    int mapped_size=0;
    int memPageIndex=0;
    int max_file_size=0;
    void flushInternal(bool force);
};






#endif //COMMON_LOGGER_H
