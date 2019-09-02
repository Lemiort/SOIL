from conans import ConanFile, CMake, tools
import os


class SoilTestConan(ConanFile):
    # TBD settings = "os", "compiler", "build_type", "arch", "os_build", "arch_build"
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake"
    build_requires = "glfw/3.2.1.20180327@bincrafters/stable", "glew/2.1.0@bincrafters/stable"

    def build(self):
        cmake = CMake(self)
        # Current dir is "test_package/build/<build_id>" and CMakeLists.txt is in "test_package"
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy('*.so*', dst='bin', src='lib')

    def test(self):
        if not tools.cross_building(self.settings):
            os.chdir("bin")
            self.run(".%ssoilcppTest" % os.sep)
