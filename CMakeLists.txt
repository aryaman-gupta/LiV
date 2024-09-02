cmake_minimum_required(VERSION 3.28)
project(LiV)

set(CMAKE_CXX_STANDARD 17)

set(ENV{JAVA_HOME} /home/aryaman/.jdks/temurin-21.0.3/)

find_package(Java)
find_package(JNI)
find_package(MPI)

add_library(LiV SHARED library.cpp
        include/JVMData.h
        include/MPINatives.h
        MPINatives.cpp
        utils/JVMUtils.h
        utils/JVMUtils.cpp
        include/MPIBuffers.h
        include/ManageRendering.h
        ManageRendering.cpp)

target_include_directories(LiV PUBLIC ${JNI_INCLUDE_DIRS} ${ICET_INCLUDE_DIRS} ${MPI_C_INCLUDE_PATH})

target_link_libraries(LiV ${JNI_LIBRARIES} ${MPI_CXX_LIBRARIES} ${ICET_CORE_LIBS} ${ICET_GL_LIBS} ${ICET_MPI_LIBS})

add_subdirectory(tests)