//
// Created by aryaman on 22.04.24.
//
#include <iostream>
#include "../include/MPINatives.h"

#include <mpi.h>

void compositePlaceholder(JNIEnv *e, jobject clazzObject, jobject subImage, jint myRank, jint commSize, jfloatArray camPos, jlong imagePointer) {
    std::cout << "In the composite placeholder function." << std::endl;
}

void setPointerAddresses(const JVMData& jvmData, const MPIBuffers& mpiBuffers, MPI_Comm& comm) {

    int commSize;
    MPI_Comm_size(comm, &commSize);

    MPI_Comm * mpiPointer = &comm;

    jfieldID allC = jvmData.env->GetFieldID(jvmData.clazz, "allToAllColorPointer", "J");
    jvmData.env->SetLongField(jvmData.obj, allC, reinterpret_cast<long>(mpiBuffers.allToAllColorPointer));

    jfieldID allD = jvmData.env->GetFieldID(jvmData.clazz, "allToAllDepthPointer", "J");
    jvmData.env->SetLongField(jvmData.obj, allD, reinterpret_cast<long>(mpiBuffers.allToAllDepthPointer));

    jfieldID allP = jvmData.env->GetFieldID(jvmData.clazz, "allToAllPrefixPointer", "J");
    jvmData.env->SetLongField(jvmData.obj, allP, reinterpret_cast<long>(mpiBuffers.allToAllPrefixPointer));

    jfieldID gatherC = jvmData.env->GetFieldID(jvmData.clazz, "gatherColorPointer", "J");
    jvmData.env->SetLongField(jvmData.obj, gatherC, reinterpret_cast<long>(mpiBuffers.gatherColorPointer));

    jfieldID gatherD = jvmData.env->GetFieldID(jvmData.clazz, "gatherDepthPointer", "J");
    jvmData.env->SetLongField(jvmData.obj, gatherD, reinterpret_cast<long>(mpiBuffers.gatherDepthPointer));

    jfieldID mpiPtr = jvmData.env->GetFieldID(jvmData.clazz, "mpiPointer", "J");
    jvmData.env->SetLongField(jvmData.obj, mpiPtr, reinterpret_cast<long>(mpiPointer));

    if(jvmData.env->ExceptionOccurred()) {
        jvmData.env->ExceptionDescribe();
        jvmData.env->ExceptionClear();
    }
}

void registerNativeFunctions(const JVMData& jvmData, const MPIBuffers& mpiBuffers, MPI_Comm& comm) {
    JNINativeMethod methods[] {
        { (char *)"compositeImages", (char *)"(Ljava/nio/ByteBuffer;II[FJ)V", (void *) &compositePlaceholder },
    };

    int ret = jvmData.env->RegisterNatives(jvmData.clazz, methods, 1);
    if(ret < 0) {
        if( jvmData.env->ExceptionOccurred() ) {
            jvmData.env->ExceptionDescribe();
        } else {
            std::cerr << "ERROR: Could not register native functions on the JVM." <<std::endl;
        }
    } else {
        std::cout<<"Natives registered. The return value is: "<< ret <<std::endl;
    }

    setPointerAddresses(jvmData, mpiBuffers, comm);
}

void setMPIParams(JVMData jvmData , int rank, int node_rank, int commSize) {
    jfieldID rankField = jvmData.env->GetFieldID(jvmData.clazz, "rank", "I");
    jvmData.env->SetIntField(jvmData.obj, rankField, rank);

    jfieldID nodeRankField = jvmData.env->GetFieldID(jvmData.clazz, "nodeRank", "I");
    jvmData.env->SetIntField(jvmData.obj, nodeRankField, node_rank);

    jfieldID sizeField = jvmData.env->GetFieldID(jvmData.clazz, "commSize", "I");
    jvmData.env->SetIntField(jvmData.obj, sizeField, commSize);
}