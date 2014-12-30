#include <jni.h>
#include "example_com_ndkdemo_MainActivity.h"
#include <opencv2/core/core.hpp>
#include <tesseract/baseapi.h>

JNIEXPORT jstring JNICALL Java_example_com_ndkdemo_MainActivity_hello
  (JNIEnv * env, jobject obj){
    return (*env)->NewStringUTF(env, "Hello from JNI");
  }
