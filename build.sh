#!/bin/bash

buildType="Debug"
cleanBuild=""
buildunittest=false
exportPackage=false

function display_usage() {
    echo "Usage: $0 [-t <Debug/Release/Coverage/ClangTidy>] [-c] [-u] [-e]"
    echo "Options:"
    echo "  -t <type> : Specify the build type (Debug/Release/Coverage). Default is Debug."
    echo "  -c        : Clean the build before starting the build process."
    echo "  -u        : Enable Unit Tests (only applicable for Debug/Release builds)."
    echo "  -e        : Export the build to local conan package"
}

while getopts ":t:cue" opt; do
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
        e)
            exportPackage=true
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
EXPORT_CMD="conan export-pkg . --user snapdev_core --channel stable"

if [ $buildunittest = true ] || [ "$buildType" = "Coverage" ]; then
    BUILD_CMD="$BUILD_CMD -o UNIT_TEST=True"
fi

if [ "$buildType" = "Debug" ]; then
    BUILD_CMD="$BUILD_CMD -s build_type=Debug"
    EXPORT_CMD="$EXPORT_CMD -s build_type=Debug"
elif [ "$buildType" = "Coverage" ]; then
    BUILD_CMD="$BUILD_CMD -o COV_BUILD=True -s build_type=Debug"
elif [ "$buildType" = "ClangTidy" ]; then
    BUILD_CMD="$BUILD_CMD -o CLANG_CHECK=True -s build_type=Release"
else
    BUILD_CMD="$BUILD_CMD -s build_type=Release"
    EXPORT_CMD="$EXPORT_CMD -s build_type=Release"
fi

EXPORT_CMD="$EXPORT_CMD  -o soci/4.0.3:with_mysql=True -o soci/4.0.3:with_sqlite3=True"
BUILD_CMD="$BUILD_CMD -o soci/4.0.3:with_mysql=True -o soci/4.0.3:with_sqlite3=True --build=missing"

RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m'

printf  "\n${PURPLE}=====================> Start Building Engines & SO...${NC}\n"
printf "$BUILD_CMD"
$BUILD_CMD
printf  "\n${PURPLE}======================> Done Building Engines & SO...${NC}\n"

#conan export-pkg . --user snapdev_core --channel stable
#conan upload   lu_platform/1.0@snapdev_core/stable  -r snap_innovations

#conan export-pkg . --user snapdev_core --channel stable  -s build_type=Debug -o soci/4.0.3:with_mysql=True -o soci/4.0.3:with_sqlite3=True
#conan export-pkg . --user snapdev_core --channel stable  -s build_type=Release -o soci/4.0.3:with_mysql=True -o soci/4.0.3:with_sqlite3=True


if [ $exportPackage = true ]; then
    printf  "\n${BOLD_BLUE}=====================> Exporting to local conan...${NC}\n"
    conan remove -c lu_pl*
    printf "$EXPORT_CMD"
    $EXPORT_CMD
    printf  "\n${BOLD_BLUE}=====================> Done exporting to local conan...${NC}\n"
fi
