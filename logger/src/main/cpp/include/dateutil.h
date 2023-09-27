//
// Created by xjsd on 2023/9/8.
//

#ifndef COMMON_DATEUTIL_H
#define COMMON_DATEUTIL_H

#include <ctime>
#include <iostream>
using namespace  std;

//@brief：时间戳转日期时间
static inline char*  getDateTimeFromTS(const char* format) {
 struct  timeval timeval{};
 gettimeofday(&timeval, nullptr);
 ::time_t  ts= timeval.tv_sec;
 if(ts<0) {
  return nullptr;
 }
 struct tm* tm ;
 struct tm tms{};
 tm= localtime_r(&ts,&tms);
 static char time_str[32]{0};
 snprintf(time_str, sizeof(time_str), format,tm->tm_year+1900,tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, timeval.tv_usec / 1000);
 return time_str;
}



#endif //COMMON_DATEUTIL_H
