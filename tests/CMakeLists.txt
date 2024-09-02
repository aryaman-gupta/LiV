cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 17)
enable_testing()

find_package(GTest REQUIRED)

set(ENV{JAVA_HOME} /home/aryaman/.jdks/temurin-21.0.3/)

find_package(Java)
find_package(JNI)

add_executable(LiV_tests LiVTests.cpp)

target_link_libraries(LiV_tests GTest::GTest GTest::Main)
target_include_directories(LiV_tests PUBLIC ${JNI_INCLUDE_DIRS} ${ICET_INCLUDE_DIRS} ${MPI_C_INCLUDE_PATH} ../include)


add_test(NAME my_test COMMAND my_test)