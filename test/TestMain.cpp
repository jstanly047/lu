#include <gtest/gtest.h>
#include <glog/logging.h>

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_minloglevel = google::WARNING;
    ::testing::InitGoogleTest(&argc, argv);
    int testResult = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return testResult;
}