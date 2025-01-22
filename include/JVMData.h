//
// Created by aryaman on 22.04.24.
//

#ifndef DISTRIBUTEDVIS_JVMDATA_HPP
#define DISTRIBUTEDVIS_JVMDATA_HPP

#include <jni.h>
#include <dirent.h>
#include <iostream>
#include <string>
#include <utils/JVMUtils.h>

static const char* getEnvVar(const char* varName, const bool null_is_error = true) {
    const char* value = getenv(varName);
    if (value == nullptr && null_is_error) {
        std::cerr << "ERROR: " << varName << " environment variable is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return value;
}

class JVMData {
public:
    JavaVM *jvm;
    jclass clazz;
    jobject obj;
    JNIEnv *env;

    explicit JVMData(int windowWidth, int windowHeight, int rank, int commSize, int nodeRank);
};

inline JVMData::JVMData(int windowWidth, int windowHeight, int rank, int commSize, int nodeRank) {
    std::string className = "ConvexVolumesInterface";

    DIR *dir;
    struct dirent *ent;
    std::string classPath = "-Djava.class.path=";
    //"/home/aryaman/Repositories/LiV-renderer/build/libs/";
    std::string directory = getEnvVar("SCENERY_CLASS_PATH");
    // std::string directory = "/home/aryaman/Repositories/LiV-renderer/build/libs/";


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

    className = "graphics/scenery/tests/interfaces/" + className;

    JavaVMInitArgs vm_args;                // Initialization arguments
    int num_options = 7;
    auto *options = new JavaVMOption[7];   // JVM invocation options
    options[0].optionString = (char *)classPath.c_str();

    options[1].optionString = (char *)
            "-Dscenery.VulkanRenderer.EnableValidations=false";

    options[2].optionString = (char *)
            "-Dorg.lwjgl.system.stackSize=1000";

    //doing headless rendering by default
    auto headless = (getEnvVar("SCENERY_HEADLESS", false) == nullptr) ? "true" : getEnvVar("SCENERY_HEADLESS");

    std::string headlessOption = "-Dscenery.Headless=" + std::string(headless);

    options[3].optionString = (char *)
            headlessOption.c_str();

    options[4].optionString = (char *)
            "-Dscenery.LogLevel=info";

    //    options[4].optionString = (char *)
    //                                      "-Dscenery.LogLevel=debug";

    auto lwjgl_shared_path = (getEnvVar("LWJGL_SHARED_PATH", false) == nullptr) ? "/tmp/" : getEnvVar("LWJGL_SHARED_PATH");
    auto lwjgl_library_path = (getEnvVar("LWJGL_LIBRARY_PATH", false) == nullptr) ? "/tmp/" : getEnvVar("LWJGL_LIBRARY_PATH");

    std::string option1 = std::string("-Dorg.lwjgl.system.SharedLibraryExtractPath=") + lwjgl_shared_path;
    std::string option2 = std::string("-Dorg.lwjgl.librarypath=") + lwjgl_library_path;

    options[5].optionString = (char *)
            (option1).c_str();
    options[6].optionString = (char *)
            (option2).c_str();


    vm_args.version = JNI_VERSION_21;
    vm_args.nOptions = num_options;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    // jint rc = JNI_CreateJavaVM(&jvm, (void **) &env, &vm_args);

    std::cout<<"Requesting JNI version 21"<<std::endl;

    if (!createJavaVM(&jvm, &env, options, vm_args.nOptions)) {
        // TODO: error processing...
        std::cerr << "ERROR: JVM load failed" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    delete[] options;

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

    jmethodID constructor = env->GetMethodID(localClass, "<init>", "(IIIII)V");  // find constructor
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
    localObj = env->NewObject(localClass, constructor, windowWidth, windowHeight, rank, commSize, nodeRank);
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
