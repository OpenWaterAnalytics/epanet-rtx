from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.scm import Git
from conan.tools.files import copy
from os import path

class GeoHashConan(ConanFile):
    name = "geohash"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain"

    def source(self):
        git = Git(self)
        git.clone(url = "https://github.com/simplegeo/libgeohash.git", target = ".")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
         self.run("clang -fPIC -c geohash.c -o geohash.o")
         self.run("ar -rcs libgeohash.a geohash.o")

    def package(self):
        copy(self, "*.h", self.source_folder, path.join(self.package_folder, "include"))
        copy(self, "*.a", self.build_folder, path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", self.name)
        
        self.cpp_info.names["cmake_find_package"] = self.name
        self.cpp_info.names["cmake_find_package_multi"] = self.name
        self.cpp_info.set_property("cmake_target_name", self.name + "::" + self.name)
        self.cpp_info.bindirs = []
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = [self.name]