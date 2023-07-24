mkdir -p dist/result
export GTEST_OUTPUT="xml:dist/result/Result.xml"

./dist/bin/LuTests 2> /dev/null

rm -rf dist/coverage
mkdir dist/coverage
lcov -c -d ./build -o ./build/coverage.info
lcov -r ./build/coverage.info '/usr/*' '*Platform*' '*Scoket*' '*Thread*' '*common' '*utils' '*test*'  -o  ./build/coverage.info
genhtml -o dist/coverage ./build/coverage.info