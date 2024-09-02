//
// Created by aryaman on 24.04.24.
//

#include "include/ManageRendering.h"
#include <iostream>
#include "utils/JVMUtils.h"

void setupICET(int windowWidth, int windowHeight, MPI_Comm comm) {
    //TODO: Implement this function

    std::cout << "setupICET" << std::endl;
}

void doRender(JVMData& jvmData) {
    //TODO: Implement this function
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