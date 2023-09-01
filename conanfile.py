import os
from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

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
        if self.settings.build_type == "Release":
            self.options.COV_BUILD = False
        elif self.settings.build_type == "Coverage":
            self.options.UNIT_TEST = True
            self.options.COV_BUILD = True

    def configure(self):
        pass

    def layout(self):
        cmake_layout(self)
        #self.cpp.source.includedirs = ["include"]
        #self.cpp.build.libdirs = ["dist/lib"]
        #self.cpp.build.bindirs = ["dist/bin"]

    def requirements(self):
        if self.options.UNIT_TEST:
            self.test_requires("gtest/1.14.0")
    
    def generate(self):
        tc = CMakeToolchain(self)
        if self.options.UNIT_TEST:
            tc.variables["UNIT_TEST"] = True
        if self.options.COV_BUILD:
            tc.variables["COV_BUILD"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        #if not self.conf.get("tools.build:skip_test", default=False):
        #    self.run(os.path.join(self.cpp.build.bindirs[0], "LuTests"))

    def package(self):
        print("sdfsdf")
        copy(self, "*.h", self.cpp.source.includedirs[0], os.path.join(self.package_folder, "include"), keep_path=True)
        copy(self, "*.a", self.cpp.build.libdirs[0], os.path.join(self.package_folder, "lib"), keep_path=True)

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