cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 17)
enable_testing()

find_package(GTest REQUIRED)

set(ENV{JAVA_HOME} /home/aryaman/.jdks/temurin-21.0.3/)

find_package(Java)
find_package(JNI)

add_executable(LiV_tests LiVTests.cpp)
add_executable(JVMUtils_tests JVMUtilsTests.cpp)

target_link_libraries(LiV_tests GTest::GTest GTest::Main)
target_link_libraries(JVMUtils_tests GTest::GTest GTest::Main ${PROJECT_NAME})
target_include_directories(LiV_tests PUBLIC ${JNI_INCLUDE_DIRS} ${ICET_INCLUDE_DIRS} ${MPI_C_INCLUDE_PATH} ../include)
target_include_directories(JVMUtils_tests PUBLIC ${JNI_INCLUDE_DIRS} ../include)

add_test(NAME LiV_tests COMMAND LiV_tests)
add_test(NAME JVMUtils_tests COMMAND JVMUtils_tests)