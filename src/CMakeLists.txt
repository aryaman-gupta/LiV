# Collect all source files
file(GLOB_RECURSE LIB_SOURCES "*.cpp")

find_package(Java REQUIRED)
if (NOT Java_FOUND)
    message(FATAL_ERROR "Java not found. Please set the JAVA_HOME environment variable.")
endif()

find_package(JNI REQUIRED)
if (NOT JNI_FOUND)
    message(FATAL_ERROR "JNI not found. Please set the JAVA_HOME environment variable.")
endif()

find_package(MPI)

# Create library target (static or shared)
add_library(${PROJECT_NAME} SHARED ${LIB_SOURCES})

#add_library(LiV SHARED
#        include/JVMData.h
#        include/MPINatives.h
#        src/MPINatives.cpp
#        src/utils/JVMUtils.h
#        src/utils/JVMUtils.cpp
#        include/MPIBuffers.h
#        include/ManageRendering.h
#        src/ManageRendering.cpp)

# Include directories for public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
        ${JNI_INCLUDE_DIRS}
        $<IF:$<VERSION_LESS:${CMAKE_VERSION},3.10>,
                ${MPI_C_INCLUDE_PATH},
                ${MPI_CXX_INCLUDE_DIRS}
        >
)

target_link_libraries(${PROJECT_NAME} ${JNI_LIBRARIES} ${MPI_CXX_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

# Installation rules for the library
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
)
