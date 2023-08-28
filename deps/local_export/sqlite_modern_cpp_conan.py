from conan import ConanFile
from conan.tools.scm import Git
from conan.tools.files import copy
from os import path

class SqliteModernCppConan(ConanFile):
    name = "sqlite_modern_cpp"
    version = "3.2"
    package_type = "header-library"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/SqliteModernCpp/sqlite_modern_cpp.git", target=".")

    def package_id(self):
        self.info.clear()

    def package(self):
        copy(self, "*", path.join(self.source_folder, "hdr"), path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "sqlite_modern_cpp")        
        self.cpp_info.set_property("cmake_target_name", self.name + "::" + self.name)
        self.cpp_info.bindirs = []
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = []
        self.cpp_info.libs = [self.name]
