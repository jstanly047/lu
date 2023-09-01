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
    "COV_BUILD": [True, False]
    }

    default_options = {
    "shared": False,
    "UNIT_TEST": False,
    "COV_BUILD": False
    }

    def config_options(self):
        if self.settings.COV_BUILD:
            self.options.UNIT_TEST = True
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
    
    def generate(self):
        cmake = CMakeDeps(self)
        cmake.generate()
        tc = CMakeToolchain(self)
        if self.options.UNIT_TEST:
            tc.variables["UNIT_TEST"] = True
        if self.options.COV_BUILD:
            tc.variables["COV_BUILD"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.parallel = True
        cmake.configure()
        cmake.build()
        if self.options.UNIT_TEST:
            self.run(os.path.join(self.cpp.build.bindirs[0], "LuTests"))

    def package(self):
        print(os.path.join(self.source_folder, "include"))
        print(self.build_folder)
        copy(self, "*.h", os.path.join(self.source_folder, "include"), os.path.join(self.package_folder, "include"), keep_path=True)
        copy(self, "*.a", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=True)

    def package_info(self):
        self.cpp_info.libs = ["lu_platform"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.includedirs = ["include"]

        '''
        if self.options.shared:
            self.cpp_info.libs = ["LuTests"]
        else:
            self.cpp_info.libs = ["LuTests"]
        '''