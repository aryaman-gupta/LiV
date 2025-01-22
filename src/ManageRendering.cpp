//
// Created by aryaman on 24.04.24.
//

#include "ManageRendering.h"
#include <iostream>
#include <vector>

namespace liv {

    void RenderingManager::setupICET() {
        //TODO: Implement this function

        std::cout << "setupICET" << std::endl;
    }

    void RenderingManager::doRender() {
        std::cout << "doRender" << std::endl;

        JNIEnv *env;
        //    jvmData.jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
        jvmData->jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

        std::cout << "Looking for main function!" << std::endl;

        jmethodID mainMethod = env->GetMethodID(jvmData->clazz, "main", "()V");
        if (mainMethod == nullptr) {
            if (env->ExceptionOccurred()) {
                std::cout << "ERROR in finding main!" << std::endl;
                env->ExceptionDescribe();
            } else {
                std::cout << "ERROR: function main not found!" << std::endl;
                //            std::exit(EXIT_FAILURE);
            }
        }
        env->CallVoidMethod(jvmData->obj, mainMethod);
        if (env->ExceptionOccurred()) {
            std::cout << "ERROR in calling main!" << std::endl;
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        jvmData->jvm->DetachCurrentThread();
    }

    void RenderingManager::setVolumeDimensions(const std::vector<int>& dimensions) {
        if (dimensions.size() != 3) {
            std::cerr << "ERROR: Dimensions vector must contain exactly 3 elements." << std::endl;
            return;
        }

        JNIEnv *env;
        jvmData->jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

        jclass superClass = env->GetSuperclass(jvmData->clazz);
        jmethodID setVolumeDimensionsMethod = findJvmMethod(env, superClass, "setVolumeDimensions", "([I)V");

        auto jDimensions = env->NewIntArray(3);
        env->SetIntArrayRegion(jDimensions, 0, 3, dimensions.data());

        invokeVoidJvmMethod(env, jvmData->obj, setVolumeDimensionsMethod, jDimensions);

        if (env->ExceptionOccurred()) {
            std::cerr << "ERROR in calling setVolumeDimensions!" << std::endl;
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        jvmData->jvm->DetachCurrentThread();
    }

    float RenderingManager::getVolumeScaling() {
        JNIEnv *env;
        jvmData->jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

        jclass superClass = env->GetSuperclass(jvmData->clazz);
        jmethodID getVolumeScalingMethod = findJvmMethod(env, superClass, "getVolumeScaling", "()F");

        jfloat scaling = env->CallFloatMethod(jvmData->obj, getVolumeScalingMethod);
        if (env->ExceptionOccurred()) {
            std::cerr << "ERROR in calling getVolumeScaling!" << std::endl;
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        jvmData->jvm->DetachCurrentThread();
        return scaling;
    }

    void RenderingManager::addProcessorData(int processorID, const std::vector<float>& origin, const std::vector<float>& dimensions) {
        if (origin.size() != 3 || dimensions.size() != 3) {
            std::cerr << "ERROR: Both float arrays must contain exactly 3 elements." << std::endl;
            return;
        }

        JNIEnv *env;
        jvmData->jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

        jmethodID addProcessorDataMethod = findJvmMethod(env, jvmData->clazz, "addProcessorData", "(I[F[F)V");

        auto jArray1 = env->NewFloatArray(3);
        env->SetFloatArrayRegion(jArray1, 0, 3, origin.data());

        auto jArray2 = env->NewFloatArray(3);
        env->SetFloatArrayRegion(jArray2, 0, 3, dimensions.data());

        env->CallVoidMethod(jvmData->obj, addProcessorDataMethod, processorID, jArray1, jArray2);

        if (env->ExceptionOccurred()) {
            std::cerr << "ERROR in calling addProcessorData!" << std::endl;
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        env->DeleteLocalRef(jArray1);
        env->DeleteLocalRef(jArray2);

        jvmData->jvm->DetachCurrentThread();
    }

    void RenderingManager::setSceneConfigured() {
        JNIEnv *env;
        jvmData->jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

        jboolean ready = true;
        jfieldID readyField = env->GetFieldID(jvmData->clazz, "rendererConfigured", "Z");
        env->SetBooleanField(jvmData->obj, readyField, ready);
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }

    void RenderingManager::waitRendererConfigured() {
        JNIEnv *env;
        env = jvmData->env;

        jmethodID rendererMethod = findJvmMethod(env, jvmData->clazz, "waitRendererReady", "()V");

        invokeVoidJvmMethod(env, jvmData->obj, rendererMethod);
        rendererConfigured = true;
    }

    void RenderingManager::stopRendering() {
        JNIEnv *env;
        jvmData->jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

        jmethodID terminateMethod = findJvmMethod(env, jvmData->clazz, "stopRendering", "()V");

        invokeVoidJvmMethod(env, jvmData->obj, terminateMethod);

        jvmData->jvm->DetachCurrentThread();
    }

    bool RenderingManager::isRendererConfigured() const {
        return rendererConfigured;
    }
}