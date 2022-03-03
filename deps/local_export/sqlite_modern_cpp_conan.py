from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class SqliteModernCppConan(ConanFile):
    name = "sqlite_modern_cpp"
    version = "3.2"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "compiler_args"
    # Sources are located in the same place as this recipe, copy them to the recipe
    #exports_sources = "*.c", "*.h"

    def source(self):
        self.run("git clone --branch v3.2 https://github.com/aminroosta/sqlite_modern_cpp.git")


    def package_id(self):
        self.info.header_only()

    def package(self):
        self.copy("*", "include", "sqlite_modern_cpp/hdr/")

    def package_info(self):
        self.cpp_info.includedirs = ['include']
