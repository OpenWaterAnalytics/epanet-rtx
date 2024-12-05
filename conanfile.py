import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class EpanetRtx(ConanFile):
    name = "epanetrtx"
    version = "2.0.2"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps", "XcodeDeps", "XcodeToolchain"

    exports_sources = "CMakeLists.txt", "src/*", "test/*"

    default_options = {
        "shared": False,
        "fPIC": True,
        "boost*:without_container": True,
        "boost*:without_context": True,
        "boost*:without_contract": True,
        "boost*:without_coroutine": True,
        "boost*:without_date_time": True,
        "boost*:without_exception": False,
        "boost*:without_fiber": True,
        "boost*:without_filesystem": False,
        "boost*:without_graph": False,
        "boost*:without_json": True,
        "boost*:without_locale": True,
        "boost*:without_log": True,
        "boost*:without_math": False,
        "boost*:without_nowide": True,
        "boost*:without_program_options": False,
        "boost*:without_python": True,
        "boost*:without_random": False,
        "boost*:without_regex": False,
        "boost*:without_serialization": False,
        "boost*:without_stacktrace": True,
        "boost*:without_system": False,
        "boost*:without_test": False,
        "boost*:without_thread": True,
        "boost*:without_timer": False,
        "boost*:without_type_erasure": True,
        "boost*:without_url": True,
        "boost*:without_wave": True
    }

    def requirements(self):
        self.requires("zlib/1.2.13")
        self.requires("openssl/3.1.2")
        self.requires("oatpp/1.3.0")
        self.requires("oatpp-openssl/1.3.0")
        self.requires("boost/1.83.0")
        self.requires("nlohmann_json/3.10.5")
        self.requires("libcurl/7.80.0")
        # self.requires("sqlite3/3.45.3")
        self.requires("sqlite_modern_cpp/3.2")
        self.requires("tsflib/1.0")
        self.requires("epanet/2.2")


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



