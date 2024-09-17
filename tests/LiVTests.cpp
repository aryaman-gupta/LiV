//
// Created by aryaman on 24.04.24.
//
#include <gtest/gtest.h>
#include "liv.h"

TEST(VolumeTest, UniqueIdTest) {
    float position[3] = {0.0f, 0.0f, 0.0f};
    int dimensions[3] = {1, 1, 1};

    auto volume1 = liv::createVolume<unsigned short>(position, dimensions, nullptr);
    auto volume2 = liv::createVolume<unsigned short>(position, dimensions, nullptr);
    auto volume3 = liv::createVolume<unsigned short>(position, dimensions, nullptr);

    ASSERT_NE(volume1.getId(), volume2.getId());
    ASSERT_NE(volume1.getId(), volume3.getId());
    ASSERT_NE(volume2.getId(), volume3.getId());
}