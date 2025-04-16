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

    try {
        jint rc = JNI_CreateJavaVM(jvm, (void **)env, &vm_args);
        if (rc != JNI_OK) {
            std::string errorMessage = "Failed to create Java VM. Error code: " + std::to_string(rc);
            if (rc == JNI_ERR) {
                errorMessage += " General JVM initialization error.";
            } else if (rc == JNI_EVERSION) {
                errorMessage += " Unsupported JNI version: " + std::to_string(vm_args.version);
            } else if (rc == JNI_ENOMEM) {
                errorMessage += " Not enough memory to create JVM.";
            } else if (rc == JNI_EEXIST) {
                errorMessage += " JVM already exists in this process.";
            } else if (rc == JNI_EINVAL) {
                errorMessage += " Invalid arguments provided to JVM initialization.";
            }
            throw std::runtime_error(errorMessage);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
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
    env->CallVoidMethodV(obj, methodID, args);
    if(env->ExceptionOccurred()) {
        std::cerr << "Error in invoking JVM method." << std::endl;
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    va_end(args);
}