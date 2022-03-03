from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class GeoHashConan(ConanFile):
    name = "geohash"
    version = "1.0"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "compiler_args"
    # Sources are located in the same place as this recipe, copy them to the recipe
    #exports_sources = "*.c", "*.h"

    def source(self):
        self.run("git clone https://github.com/simplegeo/libgeohash.git")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
         command = 'cd libgeohash && clang -fPIC -c geohash.c -o geohash.o'
         self.run(command)
         self.run("cd libgeohash && ar -rcs libgeohash.a geohash.o")

    def package(self):
        self.copy("*.h", "include", "")
        self.copy("*.a", "lib", "", keep_path=False)

    def package_info(self):
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = ["geohash"]
        self.cpp_info.includedirs = ['include']
