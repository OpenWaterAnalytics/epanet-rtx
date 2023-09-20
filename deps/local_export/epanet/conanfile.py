import os
import os
from conan import ConanFile
from conan.tools.files import copy
from conan.tools.scm import Git
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import get

class EpanetConan(ConanFile):
    name = "epanet"
    version = "2.3"
    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    # generators = "CMakeToolchain", "CMakeDeps"
    # Sources are located in the same place as this recipe, copy them to the recipe
    #exports_sources = "*.c", "*.h"

    def source(self):
        git = Git(self)
        print(os.getcwd())
        git.clone(url="https://github.com/samhatchett/epanet.git", target=".")
        git.checkout("owa_netBuilder")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["epanet"]
