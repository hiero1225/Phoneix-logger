//
// Created by xjsd on 2023/8/24.
//

#ifndef COMMON_CONSOLE_LOG_H
#define COMMON_CONSOLE_LOG_H
#define LOGD(A) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
#define LOGI(A) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGE(A) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

class console_log {

};


#endif //COMMON_CONSOLE_LOG_H
