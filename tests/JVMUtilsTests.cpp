#include <thread>
#include "gtest/gtest.h"
#include "utils/JVMUtils.h"

class JVMUtilsTest : public ::testing::Test {
protected:
    JavaVM *jvm;
    JNIEnv *env;
    JavaVMOption options[1];

    void SetUp() override {
        options[0].optionString = const_cast<char*>("-Djava.class.path=.");
    }

    void TearDown() override {
        if (jvm) {
            jvm->DestroyJavaVM();
        }
    }
};

TEST_F(JVMUtilsTest, CreatesJavaVMSuccessfully) {
    ASSERT_TRUE(createJavaVM(&jvm, &env, options, 1));
}

TEST_F(JVMUtilsTest, EnvIsFunctionalAfterJavaVMCreation) {
    ASSERT_TRUE(createJavaVM(&jvm, &env, options, 1));
    ASSERT_NE(env, nullptr);

    jclass objectClass = env->FindClass("java/lang/Object");
    ASSERT_NE(objectClass, nullptr);
}

TEST_F(JVMUtilsTest, FailsToCreateJavaVMWithInvalidOptions) {
    JavaVMOption invalidOptions[1];
    invalidOptions[0].optionString = const_cast<char*>("-invalidOption");
    ASSERT_FALSE(createJavaVM(&jvm, &env, invalidOptions, 1));
}

TEST_F(JVMUtilsTest, FailsToCreateJavaVMWithNullOptions) {
    ASSERT_FALSE(createJavaVM(&jvm, &env, nullptr, 0));
}