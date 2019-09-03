#include <filesystem>
#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.hpp>

int main() {
    constexpr int width = 512;
    constexpr int height = 512;
    // glfwWindowHint(GLFW_SAMPLES, 4);                // 4x antialiasing
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // We want OpenGL 3.3
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   GLFW_OPENGL_CORE_PROFILE);  // We don't want the old OpenGL
    GLFWwindow* window;
    // glfwSetErrorCallback(reinterpret_cast<GLFWerrorfun>(&ErrorCallback));
    // glewExperimental = true; // Needed for core profile
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    window = glfwCreateWindow(width, height, "Test window",
                              /*glfwGetPrimaryMonitor()*/ nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return 1;
    } else {
        printf("\nGLEW status is %d \n", res);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);

    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    // glDisable( GL_BLEND );
    //	straight alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //	premultiplied alpha (remember to do the same in glColor!!)
    // glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    //	do I want alpha thresholding?
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);

    //	log what the use is asking us to load
    std::string load_me{""};
    if (load_me.length() > 2) {
        // load_me = load_me.substr( 1, load_me.length() - 2 );
        load_me = load_me.substr(0, load_me.length() - 0);
    } else {
        // load_me = "img_test_uncompressed.dds";
        // load_me = "img_test_indexed.tga";
        // load_me = "img_test.dds";
        load_me = "../images/img_test.png";
        // load_me = "odd_size.jpg";
        // load_me = "img_cheryl.jpg";
        // load_me = "oak_odd.png";
        // load_me = "field_128_cube.dds";
        // load_me = "field_128_cube_nomip.dds";
        // load_me = "field_128_cube_uc.dds";
        // load_me = "field_128_cube_uc_nomip.dds";
        // load_me = "Goblin.dds";
        // load_me = "parquet.dds";
        // load_me = "stpeters_probe.hdr";
        // load_me = "VeraMoBI_sdf.png";

        //	for testing the texture rectangle code
        // load_me = "test_rect.png";
    }
    std::cout << "'" << load_me << "'" << std::endl;
    if (!std::filesystem::exists(load_me)) {
        std::cout << "The file dosn't exist in "
                  << std::filesystem::current_path() << std::endl;
    }

    //	1st try to load it as a single-image-cubemap
    //	(note, need DDS ordered faces: "EWUDNS")
    std::optional<GLuint> tex_ID;
    int time_me;

    std::cout << "Attempting to load as a cubemap" << std::endl;
    time_me = clock();
    tex_ID = soil::LoadOglSingleCubemap(
        load_me, soil::constants::kDdsCubemapFaceOrder,
        soil::ImageChannels::kAuto, soil::constants::kCreateNewId,
        soil::Flags::kPowerOfTwo |
            soil::Flags::kMipMaps
            //| SOIL_FLAG_COMPRESS_TO_DXT
            //| SOIL_FLAG_TEXTURE_REPEATS
            //| SOIL_FLAG_INVERT_Y
            | soil::Flags::kDdsLoadDirect);

    time_me = clock() - time_me;
    std::cout << "the load time was " << 0.001f * time_me
              << " seconds (warning: low resolution timer)" << std::endl;
    if (tex_ID) {
        glEnable(GL_TEXTURE_CUBE_MAP);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_GEN_R);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tex_ID.value());
        //	report
        std::cout << "the loaded single cube map ID was " << tex_ID.value()
                  << std::endl;
        // std::cout << "the load time was " << 0.001f * time_me << " seconds
        // (warning: low resolution timer)" << std::endl;
    } else {
        std::cout << "Attempting to load as a HDR texture" << std::endl;
        time_me = clock();
        tex_ID = soil::LoadOglHdrTexture(
            load_me,
            // SOIL_HDR_RGBE,
            // SOIL_HDR_RGBdivA,
            soil::HdrTypes::kRgbDivA2, 0, soil::constants::kCreateNewId,
            soil::Flags::kPowerOfTwo | soil::Flags::kMipMaps
            //| SOIL_FLAG_COMPRESS_TO_DXT
        );
        time_me = clock() - time_me;
        std::cout << "the load time was " << 0.001f * time_me
                  << " seconds (warning: low resolution timer)" << std::endl;

        //	did I fail?
        if (!tex_ID) {
            //	loading of the single-image-cubemap failed, try it as a simple
            // texture
            std::cout << "Attempting to load as a simple 2D texture"
                      << std::endl;
            //	load the texture, if specified
            time_me = clock();
            tex_ID = soil::LoadOglTexture(load_me, soil::ImageChannels::kAuto,
                                          soil::constants::kCreateNewId,
                                          soil::Flags::kPowerOfTwo |
                                              soil::Flags::kMipMaps
                                              //| SOIL_FLAG_MULTIPLY_ALPHA
                                              //| SOIL_FLAG_COMPRESS_TO_DXT
                                              | soil::Flags::kDdsLoadDirect
                                          //| SOIL_FLAG_NTSC_SAFE_RGB
                                          //| SOIL_FLAG_CoCg_Y
                                          //| SOIL_FLAG_TEXTURE_RECTANGLE
            );
            time_me = clock() - time_me;
            std::cout << "the load time was " << 0.001f * time_me
                      << " seconds (warning: low resolution timer)"
                      << std::endl;
        }

        if (tex_ID) {
            //	enable texturing
            glEnable(GL_TEXTURE_2D);
            // glEnable( 0x84F5 );// enables texture rectangle
            //  bind an OpenGL texture ID
            glBindTexture(GL_TEXTURE_2D, tex_ID.value());
            //	report
            std::cout << "the loaded texture ID was " << tex_ID.value()
                      << std::endl;
            // std::cout << "the load time was " << 0.001f * time_me << "
            // seconds (warning: low resolution timer)" << std::endl;
        } else {
            //	loading of the texture failed...why?
            glDisable(GL_TEXTURE_2D);
            std::cout << "Texture loading failed: '" << soil::GetLastResult()
                      << "'" << std::endl;
        }
    }

    // program main loop
    const float ref_mag = 0.1f;
    float theta = 0.0;

    // draw a cube
    {
        // OpenGL animation code goes here
        theta = clock() * 0.1;

        float tex_u_max = 1.0f;  // 0.2f;
        float tex_v_max = 1.0f;  // 0.2f;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        glScalef(0.8f, 0.8f, 0.8f);
        // glRotatef(-0.314159f*theta, 0.0f, 0.0f, 1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glNormal3f(-ref_mag, -ref_mag, 1.0f);
        glTexCoord2f(0.0f, tex_v_max);
        glVertex3f(-1.0f, -1.0f, -0.1f);

        glNormal3f(ref_mag, -ref_mag, 1.0f);
        glTexCoord2f(tex_u_max, tex_v_max);
        glVertex3f(1.0f, -1.0f, -0.1f);

        glNormal3f(ref_mag, ref_mag, 1.0f);
        glTexCoord2f(tex_u_max, 0.0f);
        glVertex3f(1.0f, 1.0f, -0.1f);

        glNormal3f(-ref_mag, ref_mag, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, 1.0f, -0.1f);
        glEnd();
        glPopMatrix();

        tex_u_max = 1.0f;
        tex_v_max = 1.0f;
        glPushMatrix();
        glScalef(0.8f, 0.8f, 0.8f);
        glRotatef(theta, 0.0f, 0.0f, 1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, tex_v_max);
        glVertex3f(0.0f, 0.0f, 0.1f);
        glTexCoord2f(tex_u_max, tex_v_max);
        glVertex3f(1.0f, 0.0f, 0.1f);
        glTexCoord2f(tex_u_max, 0.0f);
        glVertex3f(1.0f, 1.0f, 0.1f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.1f);
        glEnd();
        glPopMatrix();

        {
            /*	check for errors	*/
            GLenum err_code = glGetError();
            while (GL_NO_ERROR != err_code) {
                printf("OpenGL Error @ %s: %i", "drawing loop", err_code);
                err_code = glGetError();
            }
        }
    }

    //	and show off the screenshot capability
    /*
    load_me += "-screenshot.tga";
    SOIL_save_screenshot( load_me.c_str(), SOIL_SAVE_TYPE_TGA, 0, 0, 512, 512 );
    //*/
    //*
    load_me += "-screenshot.bmp";
    soil::SaveScreenshot(load_me, soil::SaveTypes::kBmp, 0, 0, 512, 512);
    //*/
    /*
    load_me += "-screenshot.dds";
    SOIL_save_screenshot( load_me.c_str(), SOIL_SAVE_TYPE_DDS, 0, 0, 512, 512 );
    //*/

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
