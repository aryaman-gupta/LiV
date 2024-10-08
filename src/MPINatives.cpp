//
// Created by aryaman on 22.04.24.
//
#include <iostream>
#include "../include/MPINatives.h"
#include <cmath>

#include <mpi.h>
#include <vector>
#include <algorithm>
#include <chrono>

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

    MPI_Alltoall(counts, 1, MPI_INT, countsRecv, 1, MPI_INT, libLiV::visualizationComm);

    //set up the AllToAllv
    int displacementSendSum = 0;
    int displacementSend[commSize];

    int displacementRecvSum = 0;
    int displacementRecv[commSize];

    int rank;
    MPI_Comm_rank(libLiV::visualizationComm, &rank);

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

    MPI_Alltoallv(sendBuf, counts, displacementSend, MPI_BYTE, recvBuf, countsRecv, displacementRecv, MPI_BYTE, libLiV::visualizationComm);

    return displacementRecvSum;
}

void distributeDenseVDIs(JNIEnv *e, jobject clazzObject, jobject colorVDI, jobject depthVDI, jobject prefixSums, jintArray supersegmentCounts, jint commSize, jlong colPointer, jlong depthPointer, jlong prefixPointer, jlong mpiPointer) {
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

    MPI_Alltoall(ptrPrefix, windowWidth * windowHeight * 4 / commSize, MPI_BYTE, recvBufPrefix, windowWidth * windowHeight * 4 / commSize, MPI_BYTE, libLiV::visualizationComm);

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

void compositeImages(JNIEnv *e, jobject clazzObject, jobject subImage, jint myRank, jint commSize, jfloatArray camPos, jlong imagePointer) {
#if VERBOSE
    std::cout<<"In image compositing function. Comm size is "<<commSize<<std::endl;
    IceTInt global_viewport[4];
    icetGetIntegerv(ICET_GLOBAL_VIEWPORT, global_viewport);

    std::cout<<"The global viewport is: width: "<<global_viewport[2] << " height: " << global_viewport[3] <<std::endl;
#endif

    IceTFloat background_color[4] = { 0.0, 0.0, 0.0, 0.0 };

    void *imageBuffer = e->GetDirectBufferAddress(subImage);
    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }

    std::cout<< "got the image buffer" << std::endl;

    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);

    icetStrategy(ICET_STRATEGY_SEQUENTIAL);
//    icetSingleImageStrategy(ICET_SINGLE_IMAGE_STRATEGY_RADIXKR)

    IceTImage image;

    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
    icetEnable(ICET_ORDERED_COMPOSITE);

    int order[commSize];

    std::cout << "Fetching the camera coordinates" << std::endl;

    float * cam = e->GetFloatArrayElements(camPos, NULL);
    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }

    std::cout << "Got the camera coordinates!" << std::endl;

    std::vector<float> distances;
    std::vector<int> procs;

    for(int i = 0; i < commSize; i++) {
        float distance = std::sqrt(
                (proc_positions[i][0] - cam[0])*(proc_positions[i][0] - cam[0]) +
                (proc_positions[i][1] - cam[1])*(proc_positions[i][1] - cam[1]) +
                (proc_positions[i][2] - cam[2])*(proc_positions[i][2] - cam[2])
        );
        distances.push_back(distance);

        procs.push_back(i);
    }

#if VERBOSE
    std::cout << "Cam pos: " << cam[0] << " " << cam[1] << " " << cam[2] << std::endl;

    if(myRank == 0) {
        for(int k = 0; k < commSize; k++) {
            std::cout << "Proc: " << k << " has centroid: " << proc_positions[k][0] << " " << proc_positions[k][1] << " " << proc_positions[k][2] << std::endl;
            std::cout << "Distance of proc " << k << ": " << distances[k] << std::endl;
        }
    }
#endif

    std::sort(procs.begin(), procs.end(), [&](int i, int j){return distances[i] < distances[j];});


    for(int i = 0; i < commSize; i++) {
        order[i] = procs[i];
    }


    icetCompositeOrder(order);

    auto begin_comp = std::chrono::high_resolution_clock::now();

    image = icetCompositeImage(
            imageBuffer,
            NULL,
            NULL,
            NULL,
            NULL,
            background_color
    );

#if PROFILING
    auto end_comp = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end_comp - begin_comp);
    auto elapsed_frame = std::chrono::duration_cast<std::chrono::nanoseconds>(end_comp - begin);
#endif
    if(myRank == 0) {
#if PROFILING
        auto compTime = (elapsed.count()) * 1e-9;
        auto frameTime = (elapsed_frame.count()) * 1e-9;

        std :: cout << "Frame time: " << frameTime << " of which compositing time: " << compTime << " so dvr time should be: " << (frameTime-compTime) << std::endl;
#endif
        const char *color_buffer = (char *)icetImageGetColorcui(image);

        IceTSizeType width;
        IceTSizeType height;
        width = icetImageGetWidth(image);
        height = icetImageGetHeight(image);

#if VERBOSE
        std::cout << "Composited the image with dimensions: " << width << " " << height << std::endl;
#endif

        std::string dataset = datasetName;

        dataset += "_" + std::to_string(commSize) + "_" + std::to_string(myRank);

        std::string basePath = "/home/aryaman/TestingData/";

        if (!benchmarking && (count % 10) == 0) {

            std::cout << "Writing the composited image " << count << " now" << std::endl;

            std::string filename = basePath + dataset + "FinalImage_" + std::to_string(count) + ".raw";

            std::ofstream b_stream(filename.c_str(),
                                   std::fstream::out | std::fstream::binary);
            if (b_stream) {
                b_stream.write(static_cast<const char *>(color_buffer),
                               windowHeight * windowWidth * 4);

                if (b_stream.good()) {
                    std::cout << "Writing was successful" << std::endl;
                }
            }
        }
        count++;

        jclass clazz = e->GetObjectClass(clazzObject);
        jmethodID displayMethod = e->GetMethodID(clazz, "displayComposited", "(Ljava/nio/ByteBuffer;)V");

        jobject bbCcomposited = e->NewDirectByteBuffer((void *)color_buffer, windowHeight * windowWidth * 4);
        if(e->ExceptionOccurred()) {
            e->ExceptionDescribe();
            e->ExceptionClear();
        }

        e->CallVoidMethod(clazzObject, displayMethod, bbCcomposited);
        if(e->ExceptionOccurred()) {
            e->ExceptionDescribe();
            e->ExceptionClear();
        }
    }
#if PROFILING
    begin = std::chrono::high_resolution_clock::now();
#endif
}