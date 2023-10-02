import os
from conan import ConanFile
from conan.tools.files import copy
from conan.tools.scm import Git

class SqliteModernCppConan(ConanFile):
    name = "sqlite_modern_cpp"
    version = "3.2"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps"
    # Sources are located in the same place as this recipe, copy them to the recipe
    #exports_sources = "*.c", "*.h"

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/aminroosta/sqlite_modern_cpp.git", target=".")
        git.checkout("v3.2")

    def package_id(self):
        self.info.clear()

    def package(self):
        copy(self, pattern="*", src=os.path.join(self.source_folder, "hdr"), dst=os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.includedirs = ['include']
