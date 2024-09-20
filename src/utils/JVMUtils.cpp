//
// Created by aryaman on 23.04.24.
//

#include "utils/JVMUtils.h"

#include <iostream>

bool createJavaVM(JavaVM **jvm, JNIEnv **env, JavaVMOption *options, int nOptions) {
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_21;
    vm_args.nOptions = nOptions;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    jint rc = JNI_CreateJavaVM(jvm, (void **)env, &vm_args);

    return rc == JNI_OK;
}

JNIEnv* getJNIEnv(JavaVM *jvm) {
    JNIEnv *env;
    jint res = jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
    if(res == JNI_EDETACHED) {
        std::cout << "Attaching to JVM" << std::endl;
        jvm->AttachCurrentThread((void**)&env, nullptr);
    }
    return env;
}

jmethodID findJvmMethod(JNIEnv *env, jclass clazz, const char* name, const char* sig) {
    jmethodID methodID = env->GetMethodID(clazz, name, sig);
    if(methodID == nullptr) {
        if (env->ExceptionOccurred()) {
            std::cerr << "Error in searching for JVM method: " << name << std::endl;
            env->ExceptionDescribe();
        } else {
            std::cerr << "JVM function " << name << " not found." << std::endl;
        }
    }
    return methodID;
}

void invokeVoidJvmMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
    va_list args;
    va_start(args,methodID);
    std::cout << "arguments are: " << args << std::endl;
    env->CallVoidMethod(obj, methodID, args);
    if(env->ExceptionOccurred()) {
        std::cerr << "Error in invoking JVM method." << std::endl;
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    va_end(args);
}