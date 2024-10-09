//
// Created by aryaman on 22.04.24.
//
#include <iostream>
#include "MPINatives.h"
#include <cmath>

#include <mpi.h>
#include <vector>
#include <algorithm>
#include <chrono>

MPI_Comm visualizationComm = MPI_COMM_WORLD; //TODO: Change this to the actual communicator

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

    // setPointerAddresses(jvmData, mpiBuffers, comm);
}

void setMPIParams(JVMData jvmData , int rank, int node_rank, int commSize) {
    jfieldID rankField = jvmData.env->GetFieldID(jvmData.clazz, "rank", "I");
    jvmData.env->SetIntField(jvmData.obj, rankField, rank);

    jfieldID nodeRankField = jvmData.env->GetFieldID(jvmData.clazz, "nodeRank", "I");
    jvmData.env->SetIntField(jvmData.obj, nodeRankField, node_rank);

    jfieldID sizeField = jvmData.env->GetFieldID(jvmData.clazz, "commSize", "I");
    jvmData.env->SetIntField(jvmData.obj, sizeField, commSize);
}

int distributeVariable(int *counts, int *countsRecv, void * sendBuf, void * recvBuf, int commSize, const std::string& purpose = "") {

#if VERBOSE
    std::cout<<"Performing distribution of " << purpose <<std::endl;
#endif

    MPI_Alltoall(counts, 1, MPI_INT, countsRecv, 1, MPI_INT, visualizationComm);

    //set up the AllToAllv
    int displacementSendSum = 0;
    int displacementSend[commSize];

    int displacementRecvSum = 0;
    int displacementRecv[commSize];

    int rank;
    MPI_Comm_rank(visualizationComm, &rank);

    for( int i = 0 ; i < commSize ; i ++){
        displacementSend[i] = displacementSendSum;
        displacementSendSum += counts[i];

        displacementRecv[i] = displacementRecvSum;
        displacementRecvSum += countsRecv[i];
    }

    if(recvBuf == nullptr) {
        std::cout<<"This is an error! Receive buffer needs to be preallocated with sufficient size"<<std::endl;
        int sum = 0;
        for( int i = 0 ; i < commSize ; i++) {
            sum += countsRecv[i];
        }
        recvBuf = malloc(sum);
    }

    MPI_Alltoallv(sendBuf, counts, displacementSend, MPI_BYTE, recvBuf, countsRecv, displacementRecv, MPI_BYTE, visualizationComm);

    return displacementRecvSum;
}

void distributeDenseVDIs(JNIEnv *e, jobject clazzObject, jobject colorVDI, jobject depthVDI, jobject prefixSums, jintArray supersegmentCounts, jint commSize, jlong colPointer, jlong depthPointer, jlong prefixPointer, jlong mpiPointer, jint windowWidth, jint windowHeight) {
#if VERBOSE
    std::cout<<"In distribute dense VDIs function. Comm size is "<<commSize<<std::endl;
#endif

    int *supsegCounts = e->GetIntArrayElements(supersegmentCounts, NULL);

    void *ptrCol = e->GetDirectBufferAddress(colorVDI);
    void *ptrDepth = e->GetDirectBufferAddress(depthVDI);

    void * recvBufCol;
    recvBufCol = reinterpret_cast<void *>(colPointer);

    void * recvBufDepth;
    recvBufDepth = reinterpret_cast<void *>(depthPointer);

    int * colorCounts = new int[commSize];
    int * depthCounts = new int[commSize];

    for(int i = 0; i < commSize; i++) {
        colorCounts[i] = supsegCounts[i] * 4 * 4;
        depthCounts[i] = supsegCounts[i] * 4 * 2;
    }

    int * colorCountsRecv = new int[commSize];
    int * depthCountsRecv = new int[commSize];

#if PROFILING
    MPI_Barrier(libLiV::visualizationComm);
    begin = std::chrono::high_resolution_clock::now();
    begin_whole_compositing = std::chrono::high_resolution_clock::now();
#endif

    int totalRecvdColor = distributeVariable(colorCounts, colorCountsRecv, ptrCol, recvBufCol, commSize, "color");
    int totalRecvdDepth = distributeVariable(depthCounts, depthCountsRecv, ptrDepth, recvBufDepth, commSize, "depth");

#if VERBOSE
    std::cout << "total bytes recvd: color: " << totalRecvdColor << " depth: " << totalRecvdDepth << std::endl;
#endif

    void * recvBufPrefix = reinterpret_cast<void *>(prefixPointer);
    void *ptrPrefix = e->GetDirectBufferAddress(prefixSums);

    //Distribute the prefix sums
    MPI_Alltoall(ptrPrefix, windowWidth * windowHeight * 4 / commSize, MPI_BYTE, recvBufPrefix, windowWidth * windowHeight * 4 / commSize, MPI_BYTE, visualizationComm);

#if PROFILING
    {
        end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);

        double local_alltoall = (elapsed.count()) * 1e-9;

        double global_sum;

        MPI_Reduce(&local_alltoall, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, libLiV::visualizationComm);

        double global_alltoall = global_sum / commSize;

        if (num_alltoall > warm_up_iterations) {
            total_alltoall += global_alltoall;

            distributeTimes.push_back((float)local_alltoall);
            globalDistributeTimes.push_back((float)global_alltoall);
        }

        num_alltoall++;

        int rank;
        MPI_Comm_rank(libLiV::visualizationComm, &rank);

#if VERBOSE
        std::cout << "Distribute time (dense) at process " << rank << " was " << local_alltoall << std::endl;

        if(rank == 0) {
            std::cout << "global alltoall time: " << global_alltoall << std::endl;
        }
#endif

        if (((num_alltoall % 50) == 0) && (rank == 0)) {
            int iterations = num_alltoall - warm_up_iterations;
            double average_alltoall = total_alltoall / (double) iterations;
            std::cout << "Number of alltoalls: " << num_alltoall << " average alltoall time so far: "
                      << average_alltoall << std::endl;
        }

    }
#endif

#if VERBOSE
    printf("Finished both alltoalls for the dense VDIs\n");
#endif

    jclass clazz = e->GetObjectClass(clazzObject);
    jmethodID compositeMethod = e->GetMethodID(clazz, "uploadForCompositingDense", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;[I[I)V");

    int supsegsRecvd = totalRecvdColor / (4 * 4);

#if PROFILING
    {
        long global_sum;
        long local = (long)supsegsRecvd;

        numSupsegsGenerated.push_back(local);

        MPI_Reduce(&local, &global_sum, 1, MPI_LONG, MPI_SUM, 0, libLiV::visualizationComm);

        long global_avg = global_sum/commSize;

        globalNumSupsegsGenerated.push_back(global_avg);

        int rank;
        MPI_Comm_rank(libLiV::visualizationComm, &rank);
        if(num_alltoall % 50 == 0) {
            if(rank == 0) {
                std::cout << "The average number of supersegments generated per PE " << global_avg << std::endl;
            }

            std::cout << "Number of supersegments received by this process: " << supsegsRecvd << std::endl;
        }
    }
#endif

    long supsegsInBuffer = 512 * 512 * (std::max((long)ceil((double)supsegsRecvd / (512.0*512.0)), 2L));

#if VERBOSE
    std::cout << "The number of supsegs recvd: " << supsegsRecvd << " and stored: " << supsegsInBuffer << std::endl;
#endif

    jobject bbCol = e->NewDirectByteBuffer(recvBufCol, supsegsInBuffer * 4 * 4);

    jobject bbDepth = e->NewDirectByteBuffer(recvBufDepth, supsegsInBuffer * 4 * 2);

    jobject bbPrefix = e->NewDirectByteBuffer( recvBufPrefix, windowWidth * windowHeight * 4);

    jintArray javaColorCounts = e->NewIntArray(commSize);
    e->SetIntArrayRegion(javaColorCounts, 0, commSize, colorCountsRecv);

    jintArray javaDepthCounts = e->NewIntArray(commSize);
    e->SetIntArrayRegion(javaDepthCounts, 0, commSize, depthCountsRecv);

    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }

#if VERBOSE
    std::cout<<"Finished distributing the VDIs. Calling the dense Composite method now!"<<std::endl;
#endif

    e->CallVoidMethod(clazzObject, compositeMethod, bbCol, bbDepth, bbPrefix, javaColorCounts, javaDepthCounts);
    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }
}