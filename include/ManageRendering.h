//
// Created by aryaman on 24.04.24.
//

#ifndef MANAGERENDERING_H
#define MANAGERENDERING_H

#include "JVMData.h"
#include <mpi.h>

void doRender(const JVMData& jvmData);
void setSceneConfigured(const JVMData& jvmData);
void waitRendererConfigured(const JVMData& jvmData);
void stopRendering(const JVMData& jvmData);
void setupICET(int windowWidth, int windowHeight, MPI_Comm comm);

#endif //MANAGERENDERING_H
