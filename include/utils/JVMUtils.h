/**
 * @file JVMUtils.h
 * @brief This file contains the declarations of utility functions for interacting with the JVM.
 */

#ifndef JVMUTILS_H
#define JVMUTILS_H

#include <jni.h>

/**
 * @brief Get the JNIEnv pointer for the given JavaVM.
 *
 * If the current thread is not attached to the JVM, it attaches it and returns the JNIEnv pointer.
 *
 * @param jvm A pointer to the JavaVM.
 * @return A pointer to the JNIEnv interface structure for the current thread.
 */
JNIEnv* getJNIEnv(JavaVM *jvm);

/**
 * @brief Find a JVM method with the given name and signature in the specified class.
 *
 * It uses the JNIEnv pointer to interact with the JVM and get the method ID.
 * If the method is not found or an exception occurs, it prints an error message and returns nullptr.
 *
 * @param env A pointer to the JNIEnv interface structure. It is a primary way to interact with the JVM.
 * @param clazz A jclass representing the class in which to find the method.
 * @param name A const char pointer representing the name of the method to find.
 * @param sig A const char pointer representing the signature of the method to find.
 * @return The ID of the JVM method if found, nullptr otherwise.
 */
jmethodID findJvmMethod(JNIEnv *env, jclass clazz, const char* name, const char* sig);

/**
 * @brief Invoke a JVM method with the given method ID on the specified object.
 *
 * It uses the JNIEnv pointer to interact with the JVM and invoke the method.
 * If an exception occurs during the method invocation, it prints an error message and describes the exception.
 *
 * @param env A pointer to the JNIEnv interface structure. It is a primary way to interact with the JVM.
 * @param obj A jobject representing the object on which to invoke the method.
 * @param methodID A jmethodID representing the ID of the method to invoke.
 * @param ... A variable number of arguments to pass to the method.
 */
void invokeVoidJvmMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...);

bool createJavaVM(JavaVM **jvm, JNIEnv **env, JavaVMOption *options, int nOptions);


#endif //JVMUTILS_H
