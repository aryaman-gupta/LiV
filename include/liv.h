#ifndef LIV_LIBRARY_H
#define LIV_LIBRARY_H

#include<mpi.h>
#include<iostream>
#include <jni.h>
#include <dirent.h>

#include "JVMData.h"
#include "MPIBuffers.h"
#include "MPINatives.h"
#include "ManageRendering.h"
#include "utils/JVMUtils.h"

#define NUM_SUPERSEGMENTS 20
#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

namespace liv {

    class LiVEngine {
    private:
        MPI_Comm setupCommunicators();

        template <typename T>
        void createVolume(float * position, int * dimensions, int volumeID) const;

        template <typename T>
        void updateVolume(T * buffer, long int buffer_size, int volumeID) const;
        int wWidth;
        int wHeight;
    public:
        JVMData* jvmData;
        RenderingManager* renderingManager;
        MPIBuffers mpiBuffers{};
        MPI_Comm livComm;
        MPI_Comm applicationComm;

        LiVEngine();

        LiVEngine(int windowWidth, int windowHeight);

        static LiVEngine initialize(int windowWidth, int windowHeight);

        void doRender() const;

        void setSceneConfigured() {
            renderingManager->setSceneConfigured();
        }

        template <typename T>
        friend class Volume;
    };

    inline MPI_Comm LiVEngine::setupCommunicators() {
        //TODO: insert logic for splitting the proccesses into different communicators
        applicationComm = MPI_COMM_WORLD;
        return MPI_COMM_WORLD;
    }

    inline LiVEngine::LiVEngine() : LiVEngine(DEFAULT_WIDTH, DEFAULT_HEIGHT) {}

    inline LiVEngine::LiVEngine(int windowWidth, int windowHeight) : wWidth(windowWidth), wHeight(windowHeight) {
        std::cout << "Entering LiVEngine constructor" << std::endl;
        jvmData = new JVMData(windowWidth, windowHeight);
        renderingManager = new RenderingManager(jvmData);
        std::cout << "Initialized jvmData" << std::endl;
        mpiBuffers = MPIBuffers();
        std::cout << "Initialized mpiBuffers" << std::endl;
        livComm = nullptr;
        applicationComm = MPI_COMM_WORLD;
        std::cout << "Exiting LiVEngine constructor" << std::endl;
    }

    inline LiVEngine LiVEngine::initialize(int windowWidth, int windowHeight) {

        int provided;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_SERIALIZED, &provided);

        std::cout << "Got MPI thread level: " << provided << std::endl;

        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        int num_processes;
        MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

        MPI_Comm nodeComm;
        MPI_Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank,
                             MPI_INFO_NULL, &nodeComm );

        int node_rank;
        MPI_Comm_rank(nodeComm,&node_rank);

        auto liv = LiVEngine(windowWidth, windowHeight);

        liv.livComm = liv.setupCommunicators();

        //replace with new
        liv.mpiBuffers.allToAllColorPointer = malloc(windowHeight * windowWidth * NUM_SUPERSEGMENTS * 4 * 4);
        liv.mpiBuffers.allToAllDepthPointer = malloc(windowWidth * windowHeight * NUM_SUPERSEGMENTS * 4 * 2);
        liv.mpiBuffers.allToAllPrefixPointer = malloc(windowWidth * windowHeight * 4);
        liv.mpiBuffers.gatherColorPointer = malloc(windowHeight * windowWidth * NUM_SUPERSEGMENTS * 4 * 4);
        liv.mpiBuffers.gatherDepthPointer = malloc(windowHeight * windowWidth * NUM_SUPERSEGMENTS * 4 * 2);

        registerNativeFunctions(*liv.jvmData, liv.mpiBuffers, liv.livComm);

        setMPIParams(*liv.jvmData, rank, node_rank, num_processes);

        // setupICET(windowWidth, windowHeight, liv.livComm);

        return liv;
    }


    template<typename T>
    void LiVEngine::createVolume(float *position, int *dimensions, int volumeID) const {

        if(!renderingManager->isRendererConfigured()) {
            std::cout << "Waiting for renderer to be configured" << std::endl;
            renderingManager->waitRendererConfigured();
        }

        JNIEnv *env = jvmData->env;

        jmethodID addVolumeMethod = findJvmMethod(env, jvmData->clazz, "addVolume", "(I[I[FZ)V");

        jintArray jdims = env->NewIntArray(3);
        jfloatArray jpos = env->NewFloatArray(3);

        env->SetIntArrayRegion(jdims, 0, 3, dimensions);
        env->SetFloatArrayRegion(jpos, 0, 3, position);

        invokeVoidJvmMethod(env, jvmData->obj, addVolumeMethod, volumeID, jdims, jpos, sizeof(T) == 2);
    }


    template <typename T>
    void LiVEngine::updateVolume(T * buffer, long int buffer_size, int volumeID) const {
        JNIEnv *env = jvmData->env;
        jmethodID updateVolumeMethod = findJvmMethod(env, jvmData->clazz, "updateVolume", "(ILjava/nio/ByteBuffer;)V");

        jobject jbuffer = env->NewDirectByteBuffer(buffer, buffer_size);
        if(env->ExceptionOccurred()) {
            std::cerr << __FILE__ << ":" << __LINE__ << " Error in allocating jbuffer." << std::endl;
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        std::cout << "volume id is: " << volumeID << std::endl;
        invokeVoidJvmMethod(env, jvmData->obj, updateVolumeMethod, volumeID, jbuffer);
    }

    inline void LiVEngine::doRender() const {
        std::cout << "In doRender function!" << std::endl;
        renderingManager->doRender();
    }




    template <typename T>
    class Volume {
    private:
        static_assert(std::is_same<T, unsigned short>::value || std::is_same<T, char>::value,
                      "Volume can only be instantiated with unsigned short or char types");

        float position[3]{};
        int dimensions[3]{};

        LiVEngine* livEngine;
        int id;

    public:

        static int currentID;

        // we don't want to allow default constructor
        Volume() = delete;

        Volume(const float *pos, const int *dims, LiVEngine* _livEngine);
        void update(T * buffer, long int buffer_size) const;

        [[nodiscard]] int getId() const {
            return id;
        }
    };

    template <typename T>
    int Volume<T>::currentID = 0;

    template <typename T>
    Volume<T>::Volume(const float *pos, const int *dims, LiVEngine* _livEngine): livEngine(_livEngine){
        position[0] = pos[0];
        position[1] = pos[1];
        position[2] = pos[2];

        dimensions[0] = dims[0];
        dimensions[1] = dims[1];
        dimensions[2] = dims[2];

        id = currentID;

        if(livEngine != nullptr) {
            livEngine->createVolume<T>(position, dimensions, id);
        } else {
            std::cerr << __FILE__ << __LINE__ << "ERROR: LiVEngine is not correctly initialized. The volume will "
                                                 "not be updated in the rendering scenegraph" << std::endl;
        }

        currentID++;
    }

    template <typename T>
    void Volume<T>::update(T * buffer, long int buffer_size) const {

        std::cout << "Buffer size is: " << buffer_size << std::endl;

        if(buffer_size != dimensions[0] * dimensions[1] * dimensions[2] * sizeof(T)) {
            std::cerr << __FILE__ << __LINE__
            << "ERROR: Buffer size does not match volume dimensions!" << std::endl;
            return;
        }

        if(livEngine != nullptr) {
            livEngine->updateVolume(buffer, buffer_size, id);
        } else {
            std::cerr << __FILE__ << __LINE__ << "ERROR: LiVEngine is not correctly initialized. Please make sure that"
                                                 "LiVEngine is correctly passed to the createVolume function" << std::endl;
        }
    }

    template <typename T>
    Volume<T> createVolume(const float * position, const int * dimensions, LiVEngine* livEngine) {
        return Volume<T>(position, dimensions, livEngine);
    }
} // namespace liv

#endif //LIV_LIBRARY_H
