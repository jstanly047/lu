mkdir -p dist/result
export GTEST_OUTPUT="xml:dist/result/Result.xml"


rm -rf dist/coverage
mkdir dist/coverage
lcov -c -d ./build -o ./build/coverage.info
lcov -r ./build/coverage.info '/usr/*' '*test*'  -o  ./build/coverage.info
genhtml -o dist/coverage ./build/coverage.info