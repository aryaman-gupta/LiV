//
// Created by aryaman on 22.04.24.
//

#ifndef MPINATIVES_H
#define MPINATIVES_H

#include <mpi.h>

#include "MPIBuffers.h"
#include "JVMData.h"

void registerNativeFunctions(const JVMData& jvmData, const MPIBuffers& mpiBuffers, MPI_Comm& comm);
void setMPIParams(JVMData jvmData , int rank, int node_rank, int commSize);

#endif //MPINATIVES_H
