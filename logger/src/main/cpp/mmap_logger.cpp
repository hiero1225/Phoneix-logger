//
// Created by xjsd on 2023/8/24.
//

#include "include/logger.h"
#include "dateutil.h"
#include <jni.h>

const LogLevel verbose=LOG_VERBOSE;
const LogLevel debug=LOG_DEBUG;
const LogLevel info=LOG_INFO;
const LogLevel error=LOG_ERROR;
const LogLevel warn=LOG_WARN;

Logger* mlogPtr= nullptr;
JavaVM * jvm= nullptr;
jobject  globalref= nullptr;
const char* format_ts="%04d%02d%02d %02d:%02d:%02d %03ld";

bool isAttachedCurrentThread(JNIEnv**  env ){
    int ret1=jvm->GetEnv(reinterpret_cast<void **>(env), JNI_VERSION_1_6);
    if(ret1<0){
       int ret =jvm->AttachCurrentThread(env, nullptr);
        LOGI(TAG,"attached with ret %d,%d",ret,ret1);
        return true;
    }
    return false;
}


void oversize_call(char* bmkPath){
    LOGI(TAG,"oversize callback invoke %s",bmkPath);
    JNIEnv *env= nullptr;
    bool attached= isAttachedCurrentThread(&env);
    if(!globalref){
        LOGE(TAG,"global ref is null");
        return;
    }
    if(!env){
        LOGE(TAG,"ENV is null");
        return;
    }

    jclass  jclz=env->GetObjectClass(globalref);

    if(jclz== nullptr){
        LOGE(TAG,"jclz is null");
    }

    jmethodID  jcallback=env->GetMethodID(jclz,"onShrink", "(Ljava/lang/String;)V");

    if(jcallback== nullptr){
        LOGE(TAG,"method onShrink ID not found!!");
        ::exit(11);
    }

    jstring p=env->NewStringUTF(bmkPath);
    env->CallVoidMethod(globalref,jcallback,p);

    if(attached){
        ::jvm->DetachCurrentThread();
    }

}



inline void write(JNIEnv *env,const LogLevel& leve,jstring& tag,jstring& extra,jstring& msg){
    if(!mlogPtr){
        LOGE(TAG,"not init yet!!!");
        return;
    }
    char* chars= const_cast<char *>(env->GetStringUTFChars(msg, nullptr));
    char* tags= const_cast<char *>(env->GetStringUTFChars(tag, nullptr));
    char* extras= const_cast<char *>(env->GetStringUTFChars(extra, nullptr));
    string s;
    s.append(getDateTimeFromTS(format_ts));
    s.append(" tid@");
    __thread_id tid= this_thread::get_id();
    auto i=*(unsigned int*)&tid;
    s.append(to_string(i));
    s.append(" ");
    s.append(tags);
    s.append(" ");
    s.append(extras);
    s.append(" ");
    s.append(chars);
    s.append("\n");
    char * formatted=const_cast<char*>(s.data());
    switch (leve) {
        case LOG_VERBOSE:
            LOGV(tags,"[%s][%s]",extras,chars);
            break;
        case LOG_DEBUG:
            LOGD(tags,"[%s][%s]",extras,chars);
            break;
        case LOG_INFO:
            LOGI(tags,"[%s][%s]",extras,chars);
            break;
        case LOG_WARN:
            LOGW(tags,"[%s][%s]",extras,chars);
            break;
        case LOG_ERROR:
            LOGE(tags,"[%s][%s]",extras,chars);
            break;
    }
    mlogPtr->log(formatted);
    env->ReleaseStringUTFChars(msg,chars);
    env->ReleaseStringUTFChars(tag,tags);
    env->ReleaseStringUTFChars(extra,extras);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_e(JNIEnv *env, jobject thiz, jstring tag, jstring extra, jstring msg) {
    write(env,error,tag,extra,msg);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_d(JNIEnv *env, jobject thiz, jstring tag, jstring extra, jstring msg) {
    write(env,debug,tag,extra,msg);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_v(JNIEnv *env, jobject thiz, jstring tag, jstring extra, jstring msg) {
    write(env,verbose,tag,extra,msg);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_i(JNIEnv *env, jobject thiz, jstring tag, jstring extra, jstring msg) {
    write(env,info,tag,extra,msg);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_w(JNIEnv *env, jobject thiz, jstring tag, jstring extra, jstring msg) {
    write(env,warn,tag,extra,msg);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_init(JNIEnv *env, jobject thiz, jstring name, jstring cacheDir,jstring logDir,jint max_file_size, jobject shrink_callback) {
    if(mlogPtr){
        LOGE(TAG,"warning!!logger not released!!!");
        mlogPtr->release();
        mlogPtr= nullptr;
        delete mlogPtr;
    }
      env->GetJavaVM(&jvm);
      globalref=env->NewGlobalRef(shrink_callback);
      const char* named=env->GetStringUTFChars(name, nullptr);
      const char* cached=env->GetStringUTFChars(cacheDir, nullptr);
      const char* logd=env->GetStringUTFChars(logDir, nullptr);
      mlogPtr=new  Logger(named,cached,logd,max_file_size,oversize_call);
      env->ReleaseStringUTFChars(name,named);
      env->ReleaseStringUTFChars(logDir,logd);
      env->ReleaseStringUTFChars(cacheDir,cached);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_uninit(JNIEnv *env, jobject thiz) {
     if(mlogPtr){
         mlogPtr->release();
         delete mlogPtr;
         mlogPtr= nullptr;
         LOGD(TAG,"release logger finished");
     }
    if(globalref){
      env->DeleteGlobalRef(globalref);
      globalref= nullptr;
      LOGD(TAG,"release globalref finished");
    }
    LOGI(TAG,"unit finished...");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_happiness_hiero_base_logger_core_PhoenixLogger_flush(JNIEnv *env, jobject thiz) {
    if(mlogPtr){
        LOGE(TAG,"flush api invoked");
        mlogPtr->flush();
    }
}