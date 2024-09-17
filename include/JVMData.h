//
// Created by aryaman on 22.04.24.
//

#ifndef DISTRIBUTEDVIS_JVMDATA_HPP
#define DISTRIBUTEDVIS_JVMDATA_HPP

#include <jni.h>
#include <dirent.h>
#include <iostream>
#include <string>

class JVMData {
public:
    JavaVM *jvm;
    jclass clazz;
    jobject obj;
    JNIEnv *env;

    explicit JVMData();
};

inline JVMData::JVMData() {
    std::string className = "DistributedVolumeRenderer";

    DIR *dir;
    struct dirent *ent;
    std::string classPath = "-Djava.class.path=";
    //"/home/aryaman/Repositories/LiV-renderer/build/libs/";
    std::string directory = getenv("SCENERY_CLASS_PATH");

    if ((dir = opendir (directory.c_str())) != nullptr) {
        while ((ent = readdir (dir)) != nullptr) {
            classPath.append(directory);
            classPath.append(ent->d_name);
            classPath.append(":");
        }
        closedir (dir);
    } else {
        /* could not open directory */
        std::cerr << "ERROR: Could not open directory "<< directory <<std::endl;
        std::exit(EXIT_FAILURE);
    }

    className = "graphics/scenery/" + className;

    JavaVMInitArgs vm_args;                        // Initialization arguments
    auto *options = new JavaVMOption[7];   // JVM invocation options
    options[0].optionString = (char *)classPath.c_str();

    options[1].optionString = (char *)
            "-Dscenery.VulkanRenderer.EnableValidations=false";

    options[2].optionString = (char *)
            "-Dorg.lwjgl.system.stackSize=1000";

    options[3].optionString = (char *)
            "-Dscenery.Headless=false";

    options[4].optionString = (char *)
            "-Dscenery.LogLevel=info";

    //    options[4].optionString = (char *)
    //                                      "-Dscenery.LogLevel=debug";

    std::string option1 = std::string("-Dorg.lwjgl.system.SharedLibraryExtractPath=") + getenv("LWJGL_SHARED_PATH");
    std::string option2 = std::string("-Dorg.lwjgl.librarypath=") + getenv("LWJGL_LIBRARY_PATH");

    options[5].optionString = (char *)
            (option1).c_str();
    options[6].optionString = (char *)
            (option2).c_str();


    vm_args.version = JNI_VERSION_21;
    vm_args.nOptions = 7;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    jint rc = JNI_CreateJavaVM(&jvm, (void **) &env, &vm_args);

    std::cout<<"Requested JNI version 21"<<std::endl;

    delete[] options;

    if (rc != JNI_OK) {
        // TODO: error processing...
        std::cin.get();
        std::exit(EXIT_FAILURE);
    }

    std::cout << "JVM load succeeded: Version " << std::endl;
    jint ver = env->GetVersion();
    std::cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << std::endl;
    std::cout << "Raw returned version: " << ver << std::endl;


    jclass localClass = env->FindClass(className.c_str());  // try to find the class
    if(localClass == nullptr) {
        if( env->ExceptionOccurred() ) {
            env->ExceptionDescribe();
        } else {
            std::cerr << "ERROR: class "<< className << " not found !" <<std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    std::cout << "Class found " << className << std::endl;

    jmethodID constructor = env->GetMethodID(localClass, "<init>", "()V");  // find constructor
    if (constructor == nullptr) {
        if( env->ExceptionOccurred() ) {
            env->ExceptionDescribe();
        } else {
            std::cerr << "ERROR: constructor not found !" <<std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    std::cout << "Constructor found for " << className << std::endl;

    //if constructor found, continue
    jobject localObj;
    localObj = env->NewObject(localClass, constructor);
    if(env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (!localObj) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: class object is null !";
            std::exit(EXIT_FAILURE);
        }
    }

    std::cout << "Object of class has been constructed" << std::endl;

    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
    }

    clazz = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
    obj = reinterpret_cast<jobject>(env->NewGlobalRef(localObj));

    env->DeleteLocalRef(localClass);
    env->DeleteLocalRef(localObj);
}

#endif //DISTRIBUTEDVIS_JVMDATA_HPP
