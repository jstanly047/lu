import os
from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMakeToolchain,CMakeDeps, CMake, cmake_layout

class snapafwRecipe(ConanFile):
    name = "lu_platform"
    version = "1.0"
    package_type = "library"

    # Optional metadata
    license = "All right reserved to Lu"
    author = "Joseph Stanly R."
    url = ""
    description = "Application framework for Lu"

    settings ="build_type"

    options = {
    "shared": [True, False],
    "UNIT_TEST": [True, False],
    "COV_BUILD": [True, False],
    "CLANG_CHECK":[True, False]
    }

    default_options = {
    "shared": False,
    "UNIT_TEST": False,
    "COV_BUILD": False,
    "CLANG_CHECK":False
    }

    def config_options(self):
        if self.options.COV_BUILD:
            self.options.UNIT_TEST = True
            self.settings.build_type = "Debug"
        elif self.settings.build_type == "Debug":
            self.options.UNIT_TEST = True

    def configure(self):
        pass

    def layout(self):
        cmake_layout(self)
        self.cpp.source.includedirs = ["include"]
        #self.cpp.build.libdirs = ["dist/lib"]
        #self.cpp.build.bindirs = ["dist/bin"]

    def requirements(self):
        if self.options.UNIT_TEST:
            self.test_requires("gtest/1.14.0")
        self.requires("glog/0.6.0")
        self.requires("soci/4.0.3")
        self.requires("libmysqlclient/8.1.0")
        # there is version conflict for this library so it override the requirement#
        self.requires("zlib/1.2.13", override=True)
        

    def generate(self):
        cmake = CMakeDeps(self)
        cmake.generate()
        tc = CMakeToolchain(self)
        if self.options.UNIT_TEST:
            tc.variables["UNIT_TEST"] = True
        if self.options.COV_BUILD:
            tc.variables["COV_BUILD"] = True
        if self.options.CLANG_CHECK:
            tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.parallel = True
        cmake.configure()
        
        if self.options.CLANG_CHECK:
            self.runClangTidy()
            return
        
        cmake.build()
        self.run_tests()

    def package(self):
        copy(self, "*.h", os.path.join(self.source_folder, "include/common"), os.path.join(self.package_folder, "include/common"), keep_path=True)
        copy(self, "*.h", os.path.join(self.source_folder, "include/platform"), os.path.join(self.package_folder, "include/platform"), keep_path=True)
        #copy(self, "*.h", os.path.join(self.source_folder, "include/reflection"), os.path.join(self.package_folder, "include/reflection"), keep_path=True)
        copy(self, "*.h", os.path.join(self.source_folder, "include/storage"), os.path.join(self.package_folder, "include/storage"), keep_path=True)
        copy(self, "*.h", os.path.join(self.source_folder, "include/utils"), os.path.join(self.package_folder, "include/utils"), keep_path=True)
        #copy(self, "*.h", os.path.join(self.source_folder, "include/thread"), os.path.join(self.package_folder, "include/thread"), keep_path=True)
        #copy(self, "*.h", os.path.join(self.source_folder, "include/queue"), os.path.join(self.package_folder, "include/queue"), keep_path=True)
        copy(self, "*.a", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=True)

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "lu_platform")
        self.cpp_info.set_property("cmake_file", "lu_platform-config.cmake")
        self.cpp_info.libs = ['liblucommon.a', 'libutils.a', 'libluSocket.a', 'libluPlatform.a', 'libluSocketDataHandler.a', 'libluThread.a', 'libluThreadChannel.a', 'libluStorageDB.a']
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.names["cmake_find_package"] = "lu_platform"
        self.cpp_info.names["cmake_find_package_multi"] = "lu_platform"

        '''
        if self.options.shared:
            self.cpp_info.libs = ["LuTests"]
        else:
            self.cpp_info.libs = ["LuTests"]
        '''

    #################Custom private functions##################
    def run_tests(self):
        unitTestResultPath=os.path.join(self.build_folder, "unitTestResult")
        os.environ['GTEST_OUTPUT'] = "xml:" + unitTestResultPath + "/Result.xml"
        coverageBaseLineFile=self.build_folder + "/coverage.baseline "

        if self.options.COV_BUILD:
            self.run("lcov -c -i -b . -d " + self.build_folder + " -o " + coverageBaseLineFile)

        if self.options.UNIT_TEST:
            self.run(os.path.join(self.cpp.build.bindirs[0], "LuTests"))
        
        if self.options.COV_BUILD:
            coverageOutFile=self.build_folder + "/coverage.out "
            coverageCombinedFile=self.build_folder + "/coverage.combined "
            coverageReportPath=self.build_folder + "/coverage "
            print(coverageBaseLineFile)
            print(coverageOutFile)
            print(coverageCombinedFile)
            print(coverageReportPath)
            self.run("lcov -c -d " + self.build_folder + " -b . -o " + coverageOutFile)
            self.run("lcov -a " + coverageBaseLineFile + " -a " + coverageOutFile + "  -o  " + coverageCombinedFile)
            self.run("lcov -r " + coverageCombinedFile + " '/usr/include/*' '*usr/lib/*' '*test*'  '*.conan2*' *spinlock.cpp *barrier.cpp -o " + coverageCombinedFile)
            self.run("genhtml -o " + coverageReportPath + coverageCombinedFile)

    
    #################Custom private functions##################
    def runClangTidy(self):
        unitTestResultPath=os.path.join(self.build_folder, "clangTidyReport.out")
        srcFiles=self.source_folder + "/src"
        include=self.source_folder + "/include"
        print(self.source_folder)
        self.run("run-clang-tidy -p " + self.build_folder + " -checks=bugprone*,cppcoreguidelines*,concurrency*,hicpp*,performance*,readability*,clang-analyzer*,misc* " + srcFiles + " " + include + " -fix > " + unitTestResultPath )