#!/bin/bash

buildType="Debug"
cleanBuild=""
buildunittest=false

function display_usage() {
    echo "Usage: $0 [-t <Debug/Release/Coverage>] [-c] [-u]"
    echo "Options:"
    echo "  -t <type> : Specify the build type (Debug/Release/Coverage). Default is Debug."
    echo "  -c        : Clean the build before starting the build process."
    echo "  -u        : Enable Unit Tests (only applicable for Debug/Release builds)."
}

while getopts ":t:cu" opt; do
    case $opt in
        t)
            case "$OPTARG" in
                Debug|Release|Coverage)
                    buildType="$OPTARG"
                    ;;
                *)
                    echo "Invalid build type. Supported types are: Debug, Release, or Coverage."
                    display_usage
                    exit 1
                    ;;
            esac
            ;;
        c)
            cleanBuild="--clean-first"
            ;;
        u)
            buildunittest=true
            ;;
        \?)
            echo "Invalid option: -$OPTARG"
            display_usage
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument."
            display_usage
            exit 1
            ;;
    esac
done



BUILD_CMD="-DCMAKE_BUILD_TYPE=$buildType"

if [ $buildunittest = true ]; then
    BUILD_CMD="$BUILD_CMD -DUNIT_TEST=ON"
fi

if [ "$buildType" = "Coverage" ]; then
    BUILD_CMD="$BUILD_CMD -DCOV_BUILD=ON  -DUNIT_TEST=ON"

elif [ $buildunittest = true ]; then
    BUILD_CMD="$BUILD_CMD -DUNIT_TEST=ON"
fi

RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m'

NUM_OF_CPU_AVL=`grep -c ^processor /proc/cpuinfo`

#use maiximum 6 parallel build
if [ $NUM_OF_CPU_AVL -gt 4 ]
then
    NUM_OF_CPU_AVL=4
fi

#protoc --cpp_out=src/message --proto_path=src/protomsg src/protomsg/*.proto
mkdir -p build
cd build
#conan install ../
#cmake ../ -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake &> /dev/null
cmake ../ $BUILD_CMD
cd ..

printf  "\n${PURPLE}=====================> Start Building Engines & SO...${NC}\n"
echo "cmake $BUILD_CMD"
echo "cmake --build ./build $cleanBuild -- -j $NUM_OF_CPU_AVL -i"
cmake --build ./build $cleanBuild -- -j $NUM_OF_CPU_AVL -i 
printf  "\n${PURPLE}======================> Done Building Engines & SO...${NC}\n"