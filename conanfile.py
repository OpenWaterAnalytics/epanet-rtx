import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class EpanetRtx(ConanFile):
    name = "epanetrtx"
    version = "1.1"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps"

    default_options = {
		"shared": False,
		"fPIC": True
    }

    def requirements(self):
        self.requires("zlib/1.2.13")
        self.requires("openssl/3.1.2")
        self.requires("oatpp/1.3.0")
        self.requires("oatpp-openssl/1.3.0")
        self.requires("boost/1.83.0")
        self.requires("nlohmann_json/3.10.5")
        self.requires("libcurl/7.80.0")
        self.requires("sqlite3/3.43.1")
        self.requires("sqlite_modern_cpp/3.2")
        self.requires("epanet/2.3")


    def build_requirements(self):
        pass

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["epanetrtx"]



