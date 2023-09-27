#include"include/logger.h"
#include "include/dateutil.h"
#include <pthread.h>
#include "zstd/lib/zstd.h"

#define DEBUG 0

const char* format_file="%04d%02d%02d %02d_%02d_%02d %03ld";

int lines = 0;
pthread_mutex_t synchronizer;
pthread_mutexattr_t recursive_attr;


//void *call(void *pVoid) {
//    auto *m = static_cast<Context* >(pVoid);
//    m->logger->oversize(m->bmk);
//    LOGI(TAG, "over size call with temp file %s ",m->bmk);
//    return nullptr;
//}



::int8_t *mmap(int fd, int pageStep) {
    if (fd < 0) {
        LOGE(TAG, "open file failed with cacheFd %d", fd);
        return nullptr;
    }
    auto *result = static_cast<int8_t *>(mmap(nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                              MAP_SHARED, fd, pageStep * PAGE_SIZE));
    LOGI(TAG, "allocate mem,mapped with pageIndex %d ,with ptr:%d,cacheFd %d,lines %d,page step:%d",
         pageStep, result, fd, lines, pageStep);
    return result;
}


::size_t map_msg(const char *const msg, int fd, int8_t *&ptr) {
    if (fd < 0) {
        LOGE(TAG, "open file failed with cacheFd %d", fd);
        return 0;
    }
    ::size_t size = strlen(msg);
    memcpy(ptr, msg, size);
//    msync(ptr, size, MS_SYNC);
    ptr += size;
    lines++;
    return size;
}




Logger::Logger(const char *logName, const char *cacheDir, const char *logDir, int maxFileSize,
               const callback &mc) {
    LOGI(TAG, "PHOENIX-LOGGER 1.3");
    LOGI(TAG, "constructor,logName %s,logD:%s,maxFileSize %d,cacheDir:%s", logName, logDir,
         maxFileSize, cacheDir);
    this->max_file_size = maxFileSize;
    this->oversize = mc;
    ints len = strlen(logName);
    ints lenD = strlen(cacheDir);
    ints lenLD = strlen(logDir);
    this->cacheDir = static_cast<char *>(::malloc(lenD));
    this->logDir = static_cast<char *>(::malloc(lenLD));
    this->logPath = static_cast<char *>(::malloc(lenLD + len + 1));

    strcpy(this->logDir, logDir);
    strcpy(this->cacheDir, cacheDir);
    //build logPath
    string logPathTmp = logDir;
    logPathTmp.append("/");
    logPathTmp.append(logName);
    ::strcpy(this->logPath, logPathTmp.data());

    string s = this->cacheDir;
    s.append("/pmmap.log");
    char *cacheFilePath = const_cast<char *>(s.data());

    mode_t attr;
    attr = S_IRWXU | S_IRWXG | S_IRWXO;
    this->cacheFd = ::open(cacheFilePath, O_RDWR | O_CREAT, attr);
    if (cacheFd < 0) {
        LOGE(TAG, "init failed ,file %s  create failed!! plz check your storage permission!!",
             cacheFilePath);
        return;
    }
    LOGI(TAG, "constructor,cachePath:%s", cacheFilePath);
    pthread_mutexattr_init(&recursive_attr);
    pthread_mutexattr_settype(&recursive_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&synchronizer, &recursive_attr);
    int cacheFileSize = getFileSize(cacheFd);
    this->memPageIndex = cacheFileSize == 0 ? 0 : cacheFileSize / PAGE_SIZE;
    int offset = cacheFileSize % PAGE_SIZE;
    if (offset > 0) {
        //first in
        LOGI(TAG, "overlay file size %d,%d", offset, memPageIndex);
        char pageOverlay[offset];
        lseek(cacheFd, -offset, SEEK_END);
        ssize_t size = read(cacheFd, pageOverlay, offset);
        LOGI(TAG, "cpy overlay msg %s ,read size:%zd", pageOverlay, size);
        log(pageOverlay);
    }

    LOGI(TAG,
         "init log finished,with log file with path %s,with pageIndex:%d ,cacheFd %d, lines %d,offset %d",
         cacheFilePath, memPageIndex, cacheFd, lines, offset);
}


void Logger::flushInternal(bool force) {
    LOGI(TAG, "flushInternal(%d),release mem page,ptr %d,%d", force, ptr, mapped_size);
    munmap(ptr - mapped_size, PAGE_SIZE);
    ptr = nullptr;
    ::size_t fz = getFileSize(cacheFd);
    if (force || fz >= MAX_CACHE_SIZE) {
        LOGI(TAG, "reach max_cache %d,%d", fz, MAX_CACHE_SIZE);
        int reduce = PAGE_SIZE - mapped_size;
        int real = fz - reduce;
        LOGI(TAG, "remove null ptr ,file %d ，real %d,reduced: %d ,map %d ", fz, real, reduce,
             mapped_size);
        //删除cache空白符号
        ftruncate(cacheFd, real);
        //追加写入到log.log
        int logFileSize = fsCopy(cacheFd, logPath);
        // int logFileSize = trimFile(cacheFd, real,logPath);
        //清空cache
        ftruncate(cacheFd, 0);
        memPageIndex = 0;
        if (logFileSize >= max_file_size||force) {
            string cmpCpyPath = logDir;
            cmpCpyPath.append("/");
            auto ts=getDateTimeFromTS(format_file);
            cmpCpyPath.append(ts);
            cmpCpyPath.append(".dst");
            char *dst = const_cast<char *>(cmpCpyPath.data());
            compressFile(logPath, dst, 5, 1);
            int ret = ::remove(logPath);
            LOGI(TAG, "delete file %s with ret:%d", logPath, ret);
            if (DEBUG) {
                //decompress
                string decompressPath = logDir;
                decompressPath.append("/");
                decompressPath.append(ts);
                decompressPath.append(".txt");
                char *decompressPathChars = const_cast<char *>(decompressPath.data());
                decompressFile(dst, decompressPathChars);
            }
        }
    }
    mapped_size = 0;
    LOGI(TAG, "flush internal finished");
}




void Logger::flush() {
    LOGD(TAG, "flush");
    pthread_mutex_lock(&synchronizer);
    if (ptr) {
        flushInternal(true);
    } else {
        LOGE(TAG, "flush failed!!");
    }
    pthread_mutex_unlock(&synchronizer);
}


void Logger::release() {
    int ret0 = pthread_mutexattr_destroy(&recursive_attr);
    int ret1 = pthread_mutex_destroy(&synchronizer);
    LOGI(TAG, "release mutex ret %d ,%d", ret0, ret1);
    if (cacheFd >= 0) {
        close(cacheFd);
        LOGD(TAG, "close cacheFd");
    }
    LOGD(TAG, "release logger");
}


Logger::~Logger() {
    ::free(logPath);
    ::free(logDir);
    ::free(cacheDir);
    LOGI(TAG, "logger destroy");
}


void Logger::log(const char *const &msg) {
    if (cacheFd < 0) return;
    pthread_mutex_lock(&synchronizer);
    ::size_t strSize = ::strlen(msg);
    mapped_size += strSize;
    if (mapped_size > PAGE_SIZE) {
        int overlay = mapped_size - PAGE_SIZE;
        mapped_size = mapped_size - strSize;
        LOGI(TAG, "lines %d,over size %d", lines, overlay);
        int cpySize = strSize - overlay;
        string s = msg;
        s = s.substr(0, cpySize);
        const char *const cpyMsg = (char *) s.data();
        log(cpyMsg);
        char *left;
        string lefts = msg;
        lefts = lefts.substr(cpySize, overlay);
        left = (char *) lefts.data();
        log(left);
    } else {
        if (!ptr) {
            ftruncate(cacheFd, (memPageIndex + 1) * PAGE_SIZE);
            ptr = mmap(cacheFd, memPageIndex);
            if (ptr == MAP_FAILED) {
                LOGE(TAG, "mem map failed!!!");
                ::exit(-1);
            }
            memPageIndex++;
            LOGI(TAG, "ptr assign %s,will move  step %d", ptr, memPageIndex);
        }

        map_msg(msg, cacheFd, ptr);
//        LOGI(TAG, "lines :%d,space left:%d,cpySize:%d,pages %d ,map_size: %d,strlen:%s", lines, PAGE_SIZE-mapped_size, strSize, memPageIndex, mapped_size,msg);

        if (mapped_size == PAGE_SIZE) {
            LOGI(TAG, "flushInternal,page full,flush...");
            flushInternal(false);
        }

    }
    pthread_mutex_unlock(&synchronizer);
}


