#!/bin/bash

buildType="Debug"
cleanBuild=""
buildunittest=false

function display_usage() {
    echo "Usage: $0 [-t <Debug/Release/Coverage/ClangTidy>] [-c] [-u]"
    echo "Options:"
    echo "  -t <type> : Specify the build type (Debug/Release/Coverage). Default is Debug."
    echo "  -c        : Clean the build before starting the build process."
    echo "  -u        : Enable Unit Tests (only applicable for Debug/Release builds)."
}

while getopts ":t:cu" opt; do
    case $opt in
        t)
            case "$OPTARG" in
                Debug|Release|Coverage|ClangTidy)
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
            rm -rf build
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



BUILD_CMD="conan build . "

if [ $buildunittest = true ]; then
    BUILD_CMD="$BUILD_CMD -o UNIT_TEST=True"
fi

if [ "$buildType" = "Debug" ]; then
    BUILD_CMD="$BUILD_CMD -s build_type=Debug"
elif [ "$buildType" = "Coverage" ]; then
    BUILD_CMD="$BUILD_CMD -o COV_BUILD=True -s build_type=Debug"
elif [ "$buildType" = "ClangTidy" ]; then
    BUILD_CMD="$BUILD_CMD -o CLANG_CHECK=True -s build_type=Release"
else
    BUILD_CMD="$BUILD_CMD -s build_type=Release"
fi

BUILD_CMD="$BUILD_CMD -o soci/4.0.3:with_mysql=True  --build=missing"

RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m'

printf  "\n${PURPLE}=====================> Start Building Engines & SO...${NC}\n"
printf "$BUILD_CMD"
$BUILD_CMD
printf  "\n${PURPLE}======================> Done Building Engines & SO...${NC}\n"
