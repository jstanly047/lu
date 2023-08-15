import os
from conan import ConanFile
from conan.tools.files import copy

class snapafwRecipe(ConanFile):
    name = "lu_platform"
    version = "1.0"
    package_type = "library"

    # Optional metadata
    license = "All right reserved to Lu"
    author = "Joseph Stanly R."
    url = ""
    description = "Application framework for Lu"

    options = {
    "shared": [True, False],
    "fPIC": [True, False],
    }

    default_options = {
    "fPIC": False,
    "shared": False,
    }

    def layout(self):
        self.cpp.source.includedirs = ["include"]
        self.cpp.build.libdirs = ["dist/lib"]

    def package(self):
        local_include_folder = os.path.join(self.source_folder, self.cpp.source.includedirs[0])
        local_lib_folder = os.path.join(self.build_folder, self.cpp.build.libdirs[0])
        copy(self, "*.h", local_include_folder, os.path.join(self.package_folder, "include"), keep_path=True)
        copy(self, "*.a", local_lib_folder, os.path.join(self.package_folder, "lib"), keep_path=True)

    def package_info(self):
        self.cpp_info.libs = ["lu_platform"]
