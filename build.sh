BUILD_TYPE=$1
export BUILD_TYPE
#Build Engine
cleanBuild=""
if [ "$1" = "clean" ] || [ "$2" = "clean" ];then
    cleanBuild="--clean-first"
fi

if [ -z "$BUILD_TYPE" ] || [ "$1" = "clean" ];then
    BUILD_TYPE="Debug"
fi

RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m'

#protoc --cpp_out=src/message --proto_path=src/protomsg src/protomsg/*.proto
mkdir -p build
cd build
#conan install ../
#cmake ../ -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake &> /dev/null
cmake ../ -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DUNIT_TEST=ON
cd ..
printf  "\n${PURPLE}=====================> Start Building Engines & SO...${NC}\n"
cmake --build ./build $cleanBuild -- -j 1 -i 
printf  "\n${PURPLE}======================> Done Building Engines & SO...${NC}\n"