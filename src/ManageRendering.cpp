//
// Created by aryaman on 24.04.24.
//

#include "../include/ManageRendering.h"
#include <iostream>
#include "utils/JVMUtils.h"

void setupICET(int windowWidth, int windowHeight, MPI_Comm comm) {
    //TODO: Implement this function

    std::cout << "setupICET" << std::endl;
}

void doRender(JVMData& jvmData) {
    std::cout << "doRender" << std::endl;

    JNIEnv *env;
    //    jvmData.jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    jvmData.jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    std::cout << "Looking for main function!" << std::endl;

    jmethodID mainMethod = env->GetMethodID(jvmData.clazz, "main", "()V");
    if(mainMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            std::cout << "ERROR in finding main!" << std::endl;
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function main not found!" << std::endl;
            //            std::exit(EXIT_FAILURE);
        }
    }
    env->CallVoidMethod(jvmData.obj, mainMethod);
    if(env->ExceptionOccurred()) {
        std::cout << "ERROR in calling main!" << std::endl;
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    jvmData.jvm->DetachCurrentThread();
}

void setSceneConfigured(JVMData& jvmData) {
    JNIEnv *env;
    jvmData.jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    jboolean ready = true;
    jfieldID readyField = env->GetFieldID(jvmData.clazz, "rendererConfigured", "Z");
    env->SetBooleanField(jvmData.obj, readyField, ready);
    if(env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void waitRendererConfigured(const JVMData& jvmData) {

    JNIEnv *env;
    jvmData.jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    jmethodID rendererMethod = findJvmMethod(env, jvmData.clazz, "rendererReady", "()V");

    invokeVoidJvmMethod(env, jvmData.obj, rendererMethod);

    jvmData.jvm->DetachCurrentThread();
}

void stopRendering(const JVMData& jvmData) {
    JNIEnv *env;
    jvmData.jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    jmethodID terminateMethod = findJvmMethod(env, jvmData.clazz, "stopRendering", "()V");

    invokeVoidJvmMethod(env, jvmData.obj, terminateMethod);

    jvmData.jvm->DetachCurrentThread();
}