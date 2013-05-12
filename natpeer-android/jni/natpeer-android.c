#include <android/log.h>
#include <stdio.h>
#include <unistd.h>
#include "ee_ut_cs_mc_natpeer_jni_NativeLibrary.h"

#define TAG "NATPeerAndroid"
#define DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

JNIEXPORT void JNICALL Java_ee_ut_cs_mc_natpeer_jni_NativeLibrary_injectFrom
    (JNIEnv *env, jclass class, jstring jlocal_addr, jint local_port,
     jstring jinterface, jstring jremote_addr, jint remote_port, jlong seq,
     jlong ts_val, jboolean nat_enabled)
{
    const char *local_addr  = (*env)->GetStringUTFChars(env, jlocal_addr, 0);
    const char *remote_addr = (*env)->GetStringUTFChars(env, jremote_addr, 0);
    const char *interface   = (*env)->GetStringUTFChars(env, jinterface, 0);
    char cmd[768];
    char *nat = nat_enabled  ? "--nat" : "";
    sprintf(cmd,
        "su -c '/system/bin/natpeer-android --response -fS -D %s -Dp %d "
        "-S %s -Sp %u -seq %lld -tv %lld -if %s %s > /sdcard/log.txt'",
        local_addr, local_port, remote_addr, remote_port, seq, ts_val,
        interface, nat);
    system(cmd);
    DEBUG(cmd);
    (*env)->ReleaseStringUTFChars(env, jlocal_addr, local_addr);
    (*env)->ReleaseStringUTFChars(env, jremote_addr, remote_addr);
    (*env)->ReleaseStringUTFChars(env, jinterface, interface);
}
