mkdir -p dist/result
export GTEST_OUTPUT="xml:dist/result/Result.xml"
rm -rf dist/coverage
mkdir dist/coverage
lcov -c -i -b . -d ./build -o ./build/coverage.baseline
./dist/bin/LuTests 2> /dev/null
lcov -c -d ./build -b . -o ./build/coverage.out
lcov -a ./build/coverage.baseline -a ./build/coverage.out -o ./build/coverage.combined
lcov  -r ./build/coverage.combined  '/usr/include/*' '*usr/lib/*' '*test*'  -o  ./build/coverage.combined
genhtml -o dist/coverage ./build/coverage.combined