# Example project using libLiV.

cmake_minimum_required(VERSION 3.28)

project(SAMPLE_LIBLiV LANGUAGES C CXX)

# Use pkg-config to find libjxl.
find_package(PkgConfig)
pkg_check_modules(LiV REQUIRED IMPORTED_TARGET libliv)

# Build the example encoder/decoder binaries using the default shared libraries
# installed.
add_executable(decode_oneshot distVolumeRendering.cpp)
target_link_libraries(decode_oneshot PkgConfig::LiV)
