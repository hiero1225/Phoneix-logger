//
// Created by xjsd on 2023/8/25.
//

#ifndef COMMON_FILE_UTILS_H
#define COMMON_FILE_UTILS_H

#include "logger.h"
#include<cstdio>
#include<cstdlib>
#include <fcntl.h>
#include<unistd.h>
#include <sys/stat.h>
#include<fstream>
#include <iostream>
#include<cstring>
#include <chrono>
#include <stdio.h>     // printf
#include <stdlib.h>    // free
#include <string.h>    // memset, strcat, strlen
#include "../zstd/lib/zstd.h"      // presumes zstd library is installed

#define NAME_LENGTH 100
#define BUFFER_SIZE 1024
using namespace  std;


time_t GetCurrentTimeMsec();
size_t  getFileSize(int fd);
int trimFile(char* const  source,int size,const char* const cpyName);
int trimFile(int fd,int size,const char* dst);
char* getParentDir(char* & path);
int fsCopy(int cacheFd,const char* const logPath);
int fsCopyPlus(int cacheFd,int size,const char* const logPath);
void decompressFile(const char* fname,const char* oname);
 void compressFile(const char* fname, const char* outName, int cLevel,int nbThreads);

int copyFileFst( int fd,int start,int len,const char* dst,int toStart);

#endif //COMMON_FILE_UTILS_H
