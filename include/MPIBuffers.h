//
// Created by aryaman on 23.04.24.
//

#ifndef MPIBUFFERS_H
#define MPIBUFFERS_H

struct MPIBuffers {
    void * allToAllColorPointer;
    void * allToAllDepthPointer;
    void * allToAllPrefixPointer;
    void * gatherColorPointer;
    void * gatherDepthPointer;
};

#endif //MPIBUFFERS_H
