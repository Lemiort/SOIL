from conans import ConanFile, CMake, tools


class SoilConan(ConanFile):
    name = "soilcpp"
    version = "0.1.0"
    license = "Public Domain"
    author = "Lemiort lemiort@gmail.com"
    scm = {
        "type": "git",
        "subfolder": ".",
        "url": "auto",
        "revision": "auto"
    }
    description = "Conan package for Simple Opengl Image Library"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    exports_sources = "src/*", "FindSOILCPP.cmake", "CMakeLists.txt"
    build_requires = "glew/2.1.0@bincrafters/stable"

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/soil %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*SOIL.hpp", dst="include/SOIL", src="src", keep_path=False)
        self.copy("*soilcpp.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)
        self.copy("FindSOILCPP.cmake", ".", ".")

    def package_info(self):
        self.cpp_info.libs = ["soilcpp"]
