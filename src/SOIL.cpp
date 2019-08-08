/*
        Jonathan Dummer
        2007-07-26-10.36

        Simple OpenGL Image Library

        Public Domain
        using Sean Barret's stb_image as a base

        Thanks to:
        * Sean Barret - for the awesome stb_image
        * Dan Venkitachalam - for finding some non-compliant DDS files, and
   patching some explicit casts
        * everybody at gamedev.net
*/

#define SOIL_CHECK_FOR_GL_ERRORS 0

#include <filesystem>

#include <GL/glew.h>

#include "SOIL.hpp"
#include "image_DXT.hpp"
#include "image_helper.hpp"
#include "stb_image_aug.hpp"

#include <stdlib.h>
#include <string.h>

namespace soil {

/*	error reporting	*/
static std::string last_result_description = "SOIL initialized";
;

/*	for loading cube maps	*/
enum LoadCapability { kUnknown = -1, kNone = 0, kPresent = 1 };

static LoadCapability has_cubemap_capability = LoadCapability::kUnknown;
LoadCapability GetCubemapCapability(void);
#define SOIL_TEXTURE_WRAP_R 0x8072
#define SOIL_CLAMP_TO_EDGE 0x812F
#define SOIL_NORMAL_MAP 0x8511
#define SOIL_REFLECTION_MAP 0x8512
#define SOIL_TEXTURE_CUBE_MAP 0x8513
#define SOIL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define SOIL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define SOIL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define SOIL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define SOIL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define SOIL_PROXY_TEXTURE_CUBE_MAP 0x851B
#define SOIL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
/*	for non-power-of-two texture	*/
static LoadCapability has_NPOT_capability = LoadCapability::kUnknown;
LoadCapability GetNpotCapability(void);
/*	for texture rectangles	*/
static LoadCapability has_tex_rectangle_capability = LoadCapability::kUnknown;
LoadCapability GetTexRectangleCapability(void);
#define SOIL_TEXTURE_RECTANGLE_ARB 0x84F5
#define SOIL_MAX_RECTANGLE_TEXTURE_SIZE_ARB 0x84F8
/*	for using DXT compression	*/
static LoadCapability has_DXT_capability = LoadCapability::kUnknown;
LoadCapability GetDxtCapability(void);
#define SOIL_RGB_S3TC_DXT1 0x83F0
#define SOIL_RGBA_S3TC_DXT1 0x83F1
#define SOIL_RGBA_S3TC_DXT3 0x83F2
#define SOIL_RGBA_S3TC_DXT5 0x83F3
std::optional<uint32_t> DirectLoadDds(std::string filename,
                                      unsigned int reuse_texture_ID, int flags,
                                      int loading_as_cubemap);
std::optional<uint32_t> DirectLoadDdsFromMemory(const uint8_t *const buffer,
                                                int buffer_length,
                                                unsigned int reuse_texture_ID,
                                                int flags,
                                                int loading_as_cubemap);
/*	other functions	*/
std::optional<uint32_t> CreateOglTextureInternal(
    const uint8_t *const data, int width, int height, ImageChannels channels,
    unsigned int reuse_texture_ID, unsigned int flags,
    unsigned int opengl_texture_type, unsigned int opengl_texture_target,
    unsigned int texture_check_size_enum);

/*	and the code magic begins here [8^)	*/
std::optional<uint32_t> LoadOglTexture(std::string filename,
                                       ImageChannels force_channels,
                                       unsigned int reuse_texture_ID,
                                       uint32_t flags) {
    uint8_t *img;
    int width, height;
    ImageChannels channels{ImageChannels::kAuto};
    std::optional<uint32_t> tex_id;
    //	does the user want direct uploading of the image as a DDS file?
    if (flags & Flags::kDdsLoadDirect) {
        // 1st try direct loading of the image as a DDS file
        // note: direct uploading will only load what is in the
        // DDS file, no MIPmaps will be generated, the image will
        // not be flipped, etc.
        tex_id = DirectLoadDds(filename, reuse_texture_ID, flags, 0);
        if (tex_id) {
            //	hey, it worked!!
            return tex_id;
        }
    }
    //	try to load the image
    img = LoadImage(filename, &width, &height, channels, force_channels);
    // channels holds the original number of channels, which may have been
    // forced
    if (force_channels != ImageChannels::kAuto) {
        channels = force_channels;
    }
    if (nullptr == img) {
        //	image loading failed
        last_result_description = stbi_failure_reason();
        return std::nullopt;
    }
    //	OK, make it a texture!
    tex_id = CreateOglTextureInternal(img, width, height, channels,
                                      reuse_texture_ID, flags, GL_TEXTURE_2D,
                                      GL_TEXTURE_2D, GL_MAX_TEXTURE_SIZE);
    //	and nuke the image data
    FreeImageData(img);
    //	and return the handle, such as it is
    return tex_id;
}

std::optional<uint32_t> LoadOglHdrTexture(std::string filename,
                                          HdrTypes fake_HDR_format,
                                          int rescale_to_max,
                                          unsigned int reuse_texture_ID,
                                          unsigned int flags) {
    uint8_t *img;
    int width, height, channels;
    std::optional<uint32_t> tex_id;
    // no direct uploading of the image as a DDS file
    // error check
    if ((fake_HDR_format != HdrTypes::kRgbe) &&
        (fake_HDR_format != HdrTypes::kRgbDivA) &&
        (fake_HDR_format != HdrTypes::kRgbDivA2)) {
        last_result_description = "Invalid fake HDR format specified";
        return std::nullopt;
    }
    //	try to load the image (only the HDR type)
    img = stbi_hdr_load_rgbe(filename.c_str(), &width, &height, &channels, 4);
    //	channels holds the original number of channels, which may have been
    // forced
    if (nullptr == img) {
        //	image loading failed
        last_result_description = stbi_failure_reason();
        return std::nullopt;
    }
    // the load worked, do I need to convert it?
    if (fake_HDR_format == HdrTypes::kRgbDivA) {
        RGBE_to_RGBdivA(img, width, height, rescale_to_max);
    } else if (fake_HDR_format == HdrTypes::kRgbDivA2) {
        RGBE_to_RGBdivA2(img, width, height, rescale_to_max);
    }
    //	OK, make it a texture!
    tex_id = CreateOglTextureInternal(img, width, height,
                                      static_cast<ImageChannels>(channels),
                                      reuse_texture_ID, flags, GL_TEXTURE_2D,
                                      GL_TEXTURE_2D, GL_MAX_TEXTURE_SIZE);
    //	and nuke the image data
    FreeImageData(img);
    //	and return the handle, such as it is
    return tex_id;
}

std::optional<uint32_t> LoadOglTextureFromMemory(const uint8_t *const buffer,
                                                 int buffer_length,
                                                 ImageChannels force_channels,
                                                 unsigned int reuse_texture_ID,
                                                 unsigned int flags) {
    uint8_t *img;
    int width, height;
    ImageChannels channels;
    std::optional<uint32_t> tex_id;
    //	does the user want direct uploading of the image as a DDS file?
    if (flags & Flags::kDdsLoadDirect) {
        // 1st try direct loading of the image as a DDS file
        // note: direct uploading will only load what is in the
        // DDS file, no MIPmaps will be generated, the image will
        // not be flipped, etc.
        tex_id = DirectLoadDdsFromMemory(buffer, buffer_length,
                                         reuse_texture_ID, flags, 0);
        if (tex_id) {
            //	hey, it worked!!
            return tex_id;
        }
    }
    //	try to load the image
    img = LoadImageFromMemory(buffer, buffer_length, &width, &height, channels,
                              force_channels);
    //	channels holds the original number of channels, which may have been
    // forced
    if (force_channels != ImageChannels::kAuto) {
        channels = force_channels;
    }
    if (NULL == img) {
        //	image loading failed
        last_result_description = stbi_failure_reason();
        return std::nullopt;
    }
    //	OK, make it a texture!
    tex_id = CreateOglTextureInternal(img, width, height, channels,
                                      reuse_texture_ID, flags, GL_TEXTURE_2D,
                                      GL_TEXTURE_2D, GL_MAX_TEXTURE_SIZE);
    //	and nuke the image data
    FreeImageData(img);
    //	and return the handle, such as it is
    return tex_id;
}

std::optional<uint32_t> LoadOglCubemap(
    std::string x_pos_file, std::string x_neg_file, std::string y_pos_file,
    std::string y_neg_file, std::string z_pos_file, std::string z_neg_file,
    ImageChannels force_channels, unsigned int reuse_texture_ID,
    unsigned int flags) {
    uint8_t *img;
    int width, height;
    ImageChannels channels;
    std::optional<uint32_t> tex_id;
    const std::pair<std::string, uint32_t> files_array[] = {
        {x_pos_file, SOIL_TEXTURE_CUBE_MAP_POSITIVE_X},
        {x_neg_file, SOIL_TEXTURE_CUBE_MAP_NEGATIVE_X},
        {y_pos_file, SOIL_TEXTURE_CUBE_MAP_POSITIVE_Y},
        {y_neg_file, SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
        {z_pos_file, SOIL_TEXTURE_CUBE_MAP_POSITIVE_Z},
        {z_neg_file, SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
    };
    // check fo errors
    for (const auto &file_pair : files_array) {
        if (!std::filesystem::exists(file_pair.first)) {
            last_result_description = "Invalid cube map files list";
            return std::nullopt;
        }
    }
    //	capability checking
    if (GetCubemapCapability() != LoadCapability::kPresent) {
        last_result_description = "No cube map capability present";
        return std::nullopt;
    }

    for (const auto &file_pair : files_array) {
        //	try to load the image
        img = LoadImage(file_pair.first, &width, &height, channels,
                        force_channels);
        //	channels holds the original number of channels, which may have
        // been forced
        if (force_channels != ImageChannels::kAuto) {
            channels = force_channels;
        }
        if (nullptr == img) {
            //	image loading failed
            last_result_description = stbi_failure_reason();
            return std::nullopt;
        }
        //	upload the texture, and create a texture ID if necessary
        tex_id = CreateOglTextureInternal(
            img, width, height, channels, reuse_texture_ID, flags,
            SOIL_TEXTURE_CUBE_MAP, file_pair.second,
            SOIL_MAX_CUBE_MAP_TEXTURE_SIZE);
        //	and nuke the image data
        FreeImageData(img);
    }
    //	and return the handle, such as it is
    return tex_id;
}

std::optional<uint32_t> LoadOglCubemapFromMemory(
    const std::vector<uint8_t> &x_pos_buffer,
    const std::vector<uint8_t> &x_neg_buffer,
    const std::vector<uint8_t> &y_pos_buffer,
    const std::vector<uint8_t> &y_neg_buffer,
    const std::vector<uint8_t> &z_pos_buffer,
    const std::vector<uint8_t> &z_neg_buffer, ImageChannels force_channels,
    unsigned int reuse_texture_ID, unsigned int flags) {
    uint8_t *img;
    int width, height;
    ImageChannels channels;
    std::optional<uint32_t> tex_id;
    const std::pair<const std::vector<uint8_t> &, uint32_t> buffers_array[] = {
        {x_pos_buffer, SOIL_TEXTURE_CUBE_MAP_POSITIVE_X},
        {x_neg_buffer, SOIL_TEXTURE_CUBE_MAP_NEGATIVE_X},
        {y_pos_buffer, SOIL_TEXTURE_CUBE_MAP_POSITIVE_Y},
        {y_neg_buffer, SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
        {z_pos_buffer, SOIL_TEXTURE_CUBE_MAP_POSITIVE_Z},
        {z_neg_buffer, SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
    };
    //	capability checking
    if (GetCubemapCapability() != LoadCapability::kPresent) {
        last_result_description = "No cube map capability present";
        return std::nullopt;
    }
    for (const auto &buffer_pair : buffers_array) {
        const auto &buffer = buffer_pair.first;
        const auto direction = buffer_pair.second;
        /*	1st face: try to load the image	*/
        img = LoadImageFromMemory(buffer.data(), buffer.size(), &width, &height,
                                  channels, force_channels);
        //	channels holds the original number of channels, which may have
        // been forced
        if (force_channels != ImageChannels::kAuto) {
            channels = force_channels;
        }
        if (nullptr == img) {
            //	image loading failed
            last_result_description = stbi_failure_reason();
            return std::nullopt;
        }
        //	upload the texture, and create a texture ID if necessary
        tex_id = CreateOglTextureInternal(
            img, width, height, channels, reuse_texture_ID, flags,
            SOIL_TEXTURE_CUBE_MAP, direction, SOIL_MAX_CUBE_MAP_TEXTURE_SIZE);
        //	and nuke the image data
        FreeImageData(img);
        //	continue?
        if (!tex_id) {
            return std::nullopt;
        }
    }
    //	and return the handle, such as it is
    return tex_id;
}

std::optional<uint32_t> LoadOglSingleCubemap(const char *filename,
                                             const char face_order[6],
                                             ImageChannels force_channels,
                                             unsigned int reuse_texture_ID,
                                             unsigned int flags) {
    /*	variables	*/
    uint8_t *img;
    int width, height, i;
    ImageChannels channels;
    std::optional<uint32_t> tex_id = std::nullopt;
    /*	error checking	*/
    if (filename == NULL) {
        last_result_description = "Invalid single cube map file name";
        return std::nullopt;
    }
    /*	does the user want direct uploading of the image as a DDS file?	*/
    if (flags & Flags::kDdsLoadDirect) {
        /*	1st try direct loading of the image as a DDS file
                note: direct uploading will only load what is in the
                DDS file, no MIPmaps will be generated, the image will
                not be flipped, etc.	*/
        tex_id = DirectLoadDds(filename, reuse_texture_ID, flags, 1);
        if (tex_id) {
            /*	hey, it worked!!	*/
            return tex_id;
        }
    }
    /*	face order checking	*/
    for (i = 0; i < 6; ++i) {
        if ((face_order[i] != 'N') && (face_order[i] != 'S') &&
            (face_order[i] != 'W') && (face_order[i] != 'E') &&
            (face_order[i] != 'U') && (face_order[i] != 'D')) {
            last_result_description = "Invalid single cube map face order";
            return std::nullopt;
        };
    }
    /*	capability checking	*/
    if (GetCubemapCapability() != LoadCapability::kPresent) {
        last_result_description = "No cube map capability present";
        return std::nullopt;
    }
    /*	1st off, try to load the full image	*/
    img = LoadImage(filename, &width, &height, channels, force_channels);
    /*	channels holds the original number of channels, which may have been
     * forced	*/
    if (force_channels != ImageChannels::kAuto) {
        channels = force_channels;
    }
    if (NULL == img) {
        /*	image loading failed	*/
        last_result_description = stbi_failure_reason();
        return std::nullopt;
    }
    /*	now, does this image have the right dimensions?	*/
    if ((width != 6 * height) && (6 * width != height)) {
        FreeImageData(img);
        last_result_description = "Single cubemap image must have a 6:1 ratio";
        return std::nullopt;
    }
    /*	try the image split and create	*/
    tex_id = CreateOglSingleCubemap(img, width, height, channels, face_order,
                                    reuse_texture_ID, flags);
    /*	nuke the temporary image data and return the texture handle	*/
    FreeImageData(img);
    return tex_id;
}

std::optional<uint32_t> LoadOglSingleCubemapFromMemory(
    const uint8_t *const buffer, int buffer_length, const char face_order[6],
    ImageChannels force_channels, unsigned int reuse_texture_ID,
    unsigned int flags) {
    /*	variables	*/
    uint8_t *img;
    int width, height, i;
    ImageChannels channels;
    std::optional<uint32_t> tex_id = std::nullopt;
    /*	error checking	*/
    if (buffer == NULL) {
        last_result_description = "Invalid single cube map buffer";
        return std::nullopt;
    }
    /*	does the user want direct uploading of the image as a DDS file?	*/
    if (flags & Flags::kDdsLoadDirect) {
        /*	1st try direct loading of the image as a DDS file
                note: direct uploading will only load what is in the
                DDS file, no MIPmaps will be generated, the image will
                not be flipped, etc.	*/
        tex_id = DirectLoadDdsFromMemory(buffer, buffer_length,
                                         reuse_texture_ID, flags, 1);
        if (tex_id) {
            /*	hey, it worked!!	*/
            return tex_id;
        }
    }
    /*	face order checking	*/
    for (i = 0; i < 6; ++i) {
        if ((face_order[i] != 'N') && (face_order[i] != 'S') &&
            (face_order[i] != 'W') && (face_order[i] != 'E') &&
            (face_order[i] != 'U') && (face_order[i] != 'D')) {
            last_result_description = "Invalid single cube map face order";
            return std::nullopt;
        };
    }
    /*	capability checking	*/
    if (GetCubemapCapability() != LoadCapability::kPresent) {
        last_result_description = "No cube map capability present";
        return std::nullopt;
    }
    /*	1st off, try to load the full image	*/
    img = LoadImageFromMemory(buffer, buffer_length, &width, &height, channels,
                              force_channels);
    /*	channels holds the original number of channels, which may have been
     * forced	*/
    if (force_channels != ImageChannels::kAuto) {
        channels = force_channels;
    }
    if (NULL == img) {
        /*	image loading failed	*/
        last_result_description = stbi_failure_reason();
        return std::nullopt;
    }
    /*	now, does this image have the right dimensions?	*/
    if ((width != 6 * height) && (6 * width != height)) {
        FreeImageData(img);
        last_result_description = "Single cubemap image must have a 6:1 ratio";
        return std::nullopt;
    }
    /*	try the image split and create	*/
    tex_id = CreateOglSingleCubemap(img, width, height, channels, face_order,
                                    reuse_texture_ID, flags);
    /*	nuke the temporary image data and return the texture handle	*/
    FreeImageData(img);
    return tex_id;
}  // namespace soil

std::optional<uint32_t> CreateOglSingleCubemap(const uint8_t *const data,
                                               int width, int height,
                                               ImageChannels channels,
                                               const char face_order[6],
                                               unsigned int reuse_texture_ID,
                                               unsigned int flags) {
    /*	variables	*/
    uint8_t *sub_img;
    int dw, dh, sz, i;
    std::optional<uint32_t> tex_id;
    int channels_count = static_cast<int>(channels);
    /*	error checking	*/
    if (data == NULL) {
        last_result_description = "Invalid single cube map image data";
        return std::nullopt;
    }
    /*	face order checking	*/
    for (i = 0; i < 6; ++i) {
        if ((face_order[i] != 'N') && (face_order[i] != 'S') &&
            (face_order[i] != 'W') && (face_order[i] != 'E') &&
            (face_order[i] != 'U') && (face_order[i] != 'D')) {
            last_result_description = "Invalid single cube map face order";
            return std::nullopt;
        };
    }
    /*	capability checking	*/
    if (GetCubemapCapability() != LoadCapability::kPresent) {
        last_result_description = "No cube map capability present";
        return std::nullopt;
    }
    /*	now, does this image have the right dimensions?	*/
    if ((width != 6 * height) && (6 * width != height)) {
        last_result_description = "Single cubemap image must have a 6:1 ratio";
        return std::nullopt;
    }
    /*	which way am I stepping?	*/
    if (width > height) {
        dw = height;
        dh = 0;
    } else {
        dw = 0;
        dh = width;
    }
    sz = dw + dh;
    sub_img = (uint8_t *)malloc(sz * sz * channels_count);
    /*	do the splitting and uploading	*/
    tex_id = reuse_texture_ID;
    for (i = 0; i < 6; ++i) {
        int x, y, idx = 0;
        unsigned int cubemap_target = 0;
        /*	copy in the sub-image	*/
        for (y = i * dh; y < i * dh + sz; ++y) {
            for (x = i * dw * channels_count;
                 x < (i * dw + sz) * channels_count; ++x) {
                sub_img[idx++] = data[y * width * channels_count + x];
            }
        }
        /*	what is my texture target?
                remember, this coordinate system is
                LHS if viewed from inside the cube!	*/
        switch (face_order[i]) {
            case 'N':
                cubemap_target = SOIL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                break;
            case 'S':
                cubemap_target = SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                break;
            case 'W':
                cubemap_target = SOIL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                break;
            case 'E':
                cubemap_target = SOIL_TEXTURE_CUBE_MAP_POSITIVE_X;
                break;
            case 'U':
                cubemap_target = SOIL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                break;
            case 'D':
                cubemap_target = SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                break;
        }
        /*	upload it as a texture	*/
        tex_id = CreateOglTextureInternal(sub_img, sz, sz, channels,
                                          tex_id.value(), flags,
                                          SOIL_TEXTURE_CUBE_MAP, cubemap_target,
                                          SOIL_MAX_CUBE_MAP_TEXTURE_SIZE);
    }
    /*	and nuke the image and sub-image data	*/
    FreeImageData(sub_img);
    /*	and return the handle, such as it is	*/
    return tex_id;
}

std::optional<uint32_t> CreateOglTexture(const uint8_t *const data, int width,
                                         int height, ImageChannels channels,
                                         unsigned int reuse_texture_ID,
                                         unsigned int flags) {
    /*	wrapper function for 2D textures	*/
    return CreateOglTextureInternal(data, width, height, channels,
                                    reuse_texture_ID, flags, GL_TEXTURE_2D,
                                    GL_TEXTURE_2D, GL_MAX_TEXTURE_SIZE);
}

#if SOIL_CHECK_FOR_GL_ERRORS
void check_for_GL_errors(const char *calling_location) {
    /*	check for errors	*/
    GLenum err_code = glGetError();
    while (GL_NO_ERROR != err_code) {
        printf("OpenGL Error @ %s: %i", calling_location, err_code);
        err_code = glGetError();
    }
}
#else
void check_for_GL_errors(const char *calling_location) {
    /*	no check for errors	*/
}
#endif

std::optional<uint32_t> CreateOglTextureInternal(
    const uint8_t *const data, int width, int height, ImageChannels channels,
    unsigned int reuse_texture_ID, unsigned int flags,
    unsigned int opengl_texture_type, unsigned int opengl_texture_target,
    unsigned int texture_check_size_enum) {
    uint8_t *img;
    std::optional<uint32_t> tex_id;
    unsigned int internal_texture_format = 0, original_texture_format = 0;
    int DXT_mode = LoadCapability::kUnknown;
    int max_supported_size;
    int channels_count = static_cast<int>(channels);
    //	If the user wants to use the texture rectangle I kill a few flags
    if (flags & Flags::kTextureRectangle) {
        //	well, the user asked for it, can we do that?
        if (GetTexRectangleCapability() == LoadCapability::kPresent) {
            //	only allow this if the user in _NOT_ trying to do a cubemap!
            if (opengl_texture_type == GL_TEXTURE_2D) {
                //	clean out the flags that cannot be used with texture
                // rectangles
                flags &= ~(Flags::kPowerOfTwo | Flags::kMipMaps |
                           Flags::kTextureRepeats);
                //	and change my target
                opengl_texture_target = SOIL_TEXTURE_RECTANGLE_ARB;
                opengl_texture_type = SOIL_TEXTURE_RECTANGLE_ARB;
            } else {
                /*	not allowed for any other uses (yes, I'm looking at you,
                 * cubemaps!)	*/
                flags &= ~Flags::kTextureRectangle;
            }
        } else {
            /*	can't do it, and that is a breakable offense (uv coords use
             * pixels instead of [0,1]!)	*/
            last_result_description = "Texture Rectangle extension unsupported";
            return std::nullopt;
        }
    }
    //	create a copy the image data
    img = (uint8_t *)malloc(width * height * channels_count);
    memcpy(img, data, width * height * channels_count);
    //	does the user want me to invert the image?
    if (flags & Flags::kInvertY) {
        int i, j;
        for (j = 0; j * 2 < height; ++j) {
            int index1 = j * width * channels_count;
            int index2 = (height - 1 - j) * width * channels_count;
            for (i = width * channels_count; i > 0; --i) {
                uint8_t temp = img[index1];
                img[index1] = img[index2];
                img[index2] = temp;
                ++index1;
                ++index2;
            }
        }
    }
    //	does the user want me to scale the colors into the NTSC safe RGB range?
    if (flags & Flags::kNtscSafeRgb) {
        scale_image_RGB_to_NTSC_safe(img, width, height, channels_count);
    }
    //	does the user want me to convert from straight to pre-multiplied alpha?
    //(and do we even _have_ alpha?)
    if (flags & Flags::kMultiplyAlpha) {
        int i;
        switch (channels_count) {
            case 2:
                for (i = 0; i < 2 * width * height; i += 2) {
                    img[i] = (img[i] * img[i + 1] + 128) >> 8;
                }
                break;
            case 4:
                for (i = 0; i < 4 * width * height; i += 4) {
                    img[i + 0] = (img[i + 0] * img[i + 3] + 128) >> 8;
                    img[i + 1] = (img[i + 1] * img[i + 3] + 128) >> 8;
                    img[i + 2] = (img[i + 2] * img[i + 3] + 128) >> 8;
                }
                break;
            default:
                //	no other number of channels contains alpha data
                break;
        }
    }
    // if the user can't support NPOT textures, make sure we force the POT
    // option
    if ((GetNpotCapability() == LoadCapability::kNone) &&
        !(flags & Flags::kTextureRectangle)) {
        //	add in the POT flag
        flags |= Flags::kPowerOfTwo;
    }
    /*	how large of a texture can this OpenGL implementation handle?	*/
    /*	texture_check_size_enum will be GL_MAX_TEXTURE_SIZE or
     * SOIL_MAX_CUBE_MAP_TEXTURE_SIZE	*/
    glGetIntegerv(texture_check_size_enum, &max_supported_size);
    /*	do I need to make it a power of 2?	*/
    if ((flags & Flags::kPowerOfTwo) || /*	user asked for it	*/
        (flags & Flags::kMipMaps) ||    /*	need it for the MIP-maps	*/
        (width >
         max_supported_size) || /*	it's too big, (make sure it's	*/
        (height >
         max_supported_size)) /*	2^n for later down-sampling)	*/
    {
        int new_width = 1;
        int new_height = 1;
        while (new_width < width) {
            new_width *= 2;
        }
        while (new_height < height) {
            new_height *= 2;
        }
        /*	still?	*/
        if ((new_width != width) || (new_height != height)) {
            /*	yep, resize	*/
            uint8_t *resampled =
                (uint8_t *)malloc(channels_count * new_width * new_height);
            up_scale_image(img, width, height, channels_count, resampled,
                           new_width, new_height);
            /*	OJO	this is for debug only!	*/
            /*
            SaveImage( "\\showme.bmp", SaveTypes::kBmp,
                                            new_width, new_height, channels,
                                            resampled );
            */
            /*	nuke the old guy, then point it at the new guy	*/
            FreeImageData(img);
            img = resampled;
            width = new_width;
            height = new_height;
        }
    }
    /*	now, if it is too large...	*/
    if ((width > max_supported_size) || (height > max_supported_size)) {
        /*	I've already made it a power of two, so simply use the
           MIPmapping code to reduce its size to the allowable maximum.	*/
        uint8_t *resampled;
        int reduce_block_x = 1, reduce_block_y = 1;
        int new_width, new_height;
        if (width > max_supported_size) {
            reduce_block_x = width / max_supported_size;
        }
        if (height > max_supported_size) {
            reduce_block_y = height / max_supported_size;
        }
        new_width = width / reduce_block_x;
        new_height = height / reduce_block_y;
        resampled = (uint8_t *)malloc(channels_count * new_width * new_height);
        /*	perform the actual reduction	*/
        mipmap_image(img, width, height, channels_count, resampled,
                     reduce_block_x, reduce_block_y);
        /*	nuke the old guy, then point it at the new guy	*/
        FreeImageData(img);
        img = resampled;
        width = new_width;
        height = new_height;
    }
    /*	does the user want us to use YCoCg color space?	*/
    if (flags & Flags::kCoCgY) {
        /*	this will only work with RGB and RGBA images */
        convert_RGB_to_YCoCg(img, width, height, channels_count);
        /*
        save_image_as_DDS( "kCoCgY.dds", width, height, channels, img );
        */
    }
    /*	create the OpenGL texture ID handle
    (note: allowing a forced texture ID lets me reload a texture)	*/
    tex_id = reuse_texture_ID;
    if (tex_id) {
        glGenTextures(1, &tex_id.value());
    }
    check_for_GL_errors("glGenTextures");
    /* Note: sometimes glGenTextures fails (usually no OpenGL context)	*/
    if (tex_id) {
        /*	and what type am I using as the internal texture format?
         */
        switch (channels_count) {
            case 1:
                original_texture_format = GL_LUMINANCE;
                break;
            case 2:
                original_texture_format = GL_LUMINANCE_ALPHA;
                break;
            case 3:
                original_texture_format = GL_RGB;
                break;
            case 4:
                original_texture_format = GL_RGBA;
                break;
        }
        internal_texture_format = original_texture_format;
        /*	does the user want me to, and can I, save as DXT?	*/
        if (flags & Flags::kCompressToDxt) {
            DXT_mode = GetDxtCapability();
            if (DXT_mode == LoadCapability::kPresent) {
                /*	I can use DXT, whether I compress it or OpenGL does
                 */
                if ((channels_count & 1) == 1) {
                    /*	1 or 3 channels = DXT1	*/
                    internal_texture_format = SOIL_RGB_S3TC_DXT1;
                } else {
                    /*	2 or 4 channels = DXT5	*/
                    internal_texture_format = SOIL_RGBA_S3TC_DXT5;
                }
            }
        }
        /*  bind an OpenGL texture ID	*/
        glBindTexture(opengl_texture_type, tex_id.value());
        check_for_GL_errors("glBindTexture");
        /*  upload the main image	*/
        if (DXT_mode == LoadCapability::kPresent) {
            /*	user wants me to do the DXT conversion!	*/
            int DDS_size;
            uint8_t *DDS_data = NULL;
            if ((channels_count & 1) == 1) {
                /*	RGB, use DXT1	*/
                DDS_data = convert_image_to_DXT1(img, width, height,
                                                 channels_count, &DDS_size);
            } else {
                /*	RGBA, use DXT5	*/
                DDS_data = convert_image_to_DXT5(img, width, height,
                                                 channels_count, &DDS_size);
            }
            if (DDS_data) {
                glCompressedTexImage2DARB(opengl_texture_target, 0,
                                          internal_texture_format, width,
                                          height, 0, DDS_size, DDS_data);
                check_for_GL_errors("glCompressedTexImage2D");
                FreeImageData(DDS_data);
                /*	printf( "Internal DXT compressor\n" );	*/
            } else {
                /*	my compression failed, try the OpenGL driver's version
                 */
                glTexImage2D(opengl_texture_target, 0, internal_texture_format,
                             width, height, 0, original_texture_format,
                             GL_UNSIGNED_BYTE, img);
                check_for_GL_errors("glTexImage2D");
                /*	printf( "OpenGL DXT compressor\n" );	*/
            }
        } else {
            /*	user want OpenGL to do all the work!	*/
            glTexImage2D(opengl_texture_target, 0, internal_texture_format,
                         width, height, 0, original_texture_format,
                         GL_UNSIGNED_BYTE, img);
            check_for_GL_errors("glTexImage2D");
            /*printf( "OpenGL DXT compressor\n" );	*/
        }
        /*	are any MIPmaps desired?	*/
        if (flags & Flags::kMipMaps) {
            int MIPlevel = 1;
            int MIPwidth = (width + 1) / 2;
            int MIPheight = (height + 1) / 2;
            uint8_t *resampled =
                (uint8_t *)malloc(channels_count * MIPwidth * MIPheight);
            while (((1 << MIPlevel) <= width) || ((1 << MIPlevel) <= height)) {
                /*	do this MIPmap level	*/
                mipmap_image(img, width, height, channels_count, resampled,
                             (1 << MIPlevel), (1 << MIPlevel));
                /*  upload the MIPmaps	*/
                if (DXT_mode == LoadCapability::kPresent) {
                    /*	user wants me to do the DXT conversion!	*/
                    int DDS_size;
                    uint8_t *DDS_data = NULL;
                    if ((channels_count & 1) == 1) {
                        /*	RGB, use DXT1	*/
                        DDS_data = convert_image_to_DXT1(
                            resampled, MIPwidth, MIPheight, channels_count,
                            &DDS_size);
                    } else {
                        /*	RGBA, use DXT5	*/
                        DDS_data = convert_image_to_DXT5(
                            resampled, MIPwidth, MIPheight, channels_count,
                            &DDS_size);
                    }
                    if (DDS_data) {
                        glCompressedTexImage2DARB(
                            opengl_texture_target, MIPlevel,
                            internal_texture_format, MIPwidth, MIPheight, 0,
                            DDS_size, DDS_data);
                        check_for_GL_errors("glCompressedTexImage2D");
                        FreeImageData(DDS_data);
                    } else {
                        /*	my compression failed, try the OpenGL driver's
                         * version	*/
                        glTexImage2D(opengl_texture_target, MIPlevel,
                                     internal_texture_format, MIPwidth,
                                     MIPheight, 0, original_texture_format,
                                     GL_UNSIGNED_BYTE, resampled);
                        check_for_GL_errors("glTexImage2D");
                    }
                } else {
                    /*	user want OpenGL to do all the work!	*/
                    glTexImage2D(opengl_texture_target, MIPlevel,
                                 internal_texture_format, MIPwidth, MIPheight,
                                 0, original_texture_format, GL_UNSIGNED_BYTE,
                                 resampled);
                    check_for_GL_errors("glTexImage2D");
                }
                /*	prep for the next level	*/
                ++MIPlevel;
                MIPwidth = (MIPwidth + 1) / 2;
                MIPheight = (MIPheight + 1) / 2;
            }
            FreeImageData(resampled);
            /*	instruct OpenGL to use the MIPmaps	*/
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            check_for_GL_errors("GL_TEXTURE_MIN/MAG_FILTER");
        } else {
            /*	instruct OpenGL _NOT_ to use the MIPmaps	*/
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);
            check_for_GL_errors("GL_TEXTURE_MIN/MAG_FILTER");
        }
        /*	does the user want clamping, or wrapping?	*/
        if (flags & Flags::kTextureRepeats) {
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
            if (opengl_texture_type == SOIL_TEXTURE_CUBE_MAP) {
                /*	SOIL_TEXTURE_WRAP_R is invalid if cubemaps aren't
                 * supported
                 */
                glTexParameteri(opengl_texture_type, SOIL_TEXTURE_WRAP_R,
                                GL_REPEAT);
            }
            check_for_GL_errors("GL_TEXTURE_WRAP_*");
        } else {
            /*	unsigned int clamp_mode = SOIL_CLAMP_TO_EDGE;	*/
            unsigned int clamp_mode = GL_CLAMP;
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_S, clamp_mode);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_T, clamp_mode);
            if (opengl_texture_type == SOIL_TEXTURE_CUBE_MAP) {
                /*	SOIL_TEXTURE_WRAP_R is invalid if cubemaps aren't
                 * supported
                 */
                glTexParameteri(opengl_texture_type, SOIL_TEXTURE_WRAP_R,
                                clamp_mode);
            }
            check_for_GL_errors("GL_TEXTURE_WRAP_*");
        }
        /*	done	*/
        last_result_description = "Image loaded as an OpenGL texture";
    } else {
        /*	failed	*/
        last_result_description =
            "Failed to generate an OpenGL texture name; missing OpenGL "
            "context?";
    }
    FreeImageData(img);
    return tex_id;
}

bool SaveScreenshot(std::string filename, int image_type, int x, int y,
                    int width, int height) {
    uint8_t *pixel_data;
    int i, j;
    bool save_result;

    /*	error checks	*/
    if ((width < 1) || (height < 1)) {
        last_result_description = "Invalid screenshot dimensions";
        return false;
    }
    if ((x < 0) || (y < 0)) {
        last_result_description = "Invalid screenshot location";
        return false;
    }

    /*  Get the data from OpenGL	*/
    pixel_data = (uint8_t *)malloc(3 * width * height);
    glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);

    /*	invert the image	*/
    for (j = 0; j * 2 < height; ++j) {
        int index1 = j * width * 3;
        int index2 = (height - 1 - j) * width * 3;
        for (i = width * 3; i > 0; --i) {
            uint8_t temp = pixel_data[index1];
            pixel_data[index1] = pixel_data[index2];
            pixel_data[index2] = temp;
            ++index1;
            ++index2;
        }
    }

    /*	save the image	*/
    save_result = SaveImage(filename, image_type, width, height,
                            ImageChannels::kRgb, pixel_data);

    /*  And free the memory	*/
    FreeImageData(pixel_data);
    return save_result;
}

uint8_t *LoadImage(std::string filename, int *width, int *height,
                   ImageChannels &channels, ImageChannels force_channels) {
    int int_channels = static_cast<int>(channels);
    uint8_t *result = stbi_load(filename.c_str(), width, height, &int_channels,
                                static_cast<int>(force_channels));
    if (result == nullptr) {
        last_result_description = stbi_failure_reason();
    } else {
        last_result_description = "Image loaded";
    }
    channels = static_cast<ImageChannels>(int_channels);
    return result;
}

uint8_t *LoadImageFromMemory(const uint8_t *const buffer, int buffer_length,
                             int *width, int *height, ImageChannels &channels,
                             ImageChannels force_channels) {
    int int_channels = static_cast<int>(channels);
    uint8_t *result =
        stbi_load_from_memory(buffer, buffer_length, width, height,
                              &int_channels, static_cast<int>(force_channels));
    if (result == NULL) {
        last_result_description = stbi_failure_reason();
    } else {
        last_result_description = "Image loaded from memory";
    }
    channels = static_cast<ImageChannels>(int_channels);
    return result;
}

bool SaveImage(std::string filename, int image_type, int width, int height,
               int channels, const uint8_t *const data) {
    bool save_result;

    /*	error check	*/
    if ((width < 1) || (height < 1) || (channels < 1) || (channels > 4) ||
        (data == NULL)) {
        return false;
    }
    if (image_type == SaveTypes::kBmp) {
        save_result = stbi_write_bmp(filename.c_str(), width, height, channels,
                                     (void *)data);
    } else if (image_type == SaveTypes::kTga) {
        save_result = stbi_write_tga(filename.c_str(), width, height, channels,
                                     (void *)data);
    } else if (image_type == SaveTypes::kDds) {
        save_result = save_image_as_DDS(filename.c_str(), width, height,
                                        channels, (const uint8_t *const)data);
    } else {
        save_result = false;
    }
    if (save_result == false) {
        last_result_description = "Saving the image failed";
    } else {
        last_result_description = "Image saved";
    }
    return save_result;
}

void FreeImageData(uint8_t *img_data) { free((void *)img_data); }

std::string GetLastResult(void) { return last_result_description; }

std::optional<uint32_t> DirectLoadDdsFromMemory(const uint8_t *const buffer,
                                                int buffer_length,
                                                unsigned int reuse_texture_ID,
                                                int flags,
                                                bool loading_as_cubemap) {
    //	variables
    DDS_header header;
    unsigned int buffer_index = 0;
    std::optional<uint32_t> tex_ID{std::nullopt};
    //	file reading variables
    unsigned int S3TC_type = 0;
    uint8_t *DDS_data;
    unsigned int DDS_main_size;
    unsigned int DDS_full_size;
    unsigned int width, height;
    int mipmaps, cubemap, uncompressed, block_size = 16;
    unsigned int flag;
    unsigned int cf_target, ogl_target_start, ogl_target_end;
    unsigned int opengl_texture_type;
    int i;
    //	1st off, does the filename even exist?
    if (NULL == buffer) {
        /*	we can't do it!	*/
        last_result_description = "NULL buffer";
        return std::nullopt;
    }
    if (buffer_length < sizeof(DDS_header)) {
        /*	we can't do it!	*/
        last_result_description =
            "DDS file was too small to contain the DDS header";
        return std::nullopt;
    }
    //	try reading in the header
    memcpy((void *)(&header), (const void *)buffer, sizeof(DDS_header));
    buffer_index = sizeof(DDS_header);
    //	guilty until proven innocent
    last_result_description = "Failed to read a known DDS header";
    //	validate the header (warning, "goto"'s ahead, shield your eyes!!)
    flag = ('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24);
    if (header.dwMagic != flag) {
        goto quick_exit;
    }
    if (header.dwSize != 124) {
        goto quick_exit;
    }
    //	I need all of these
    flag = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    if ((header.dwFlags & flag) != flag) {
        goto quick_exit;
    }
    // According to the MSDN spec, the dwFlags should contain DDSD_LINEARSIZE if
    // it's compressed, or DDSD_PITCH if uncompressed.  Some DDS writers do not
    // conform to the spec, so I need to make my reader more tolerant
    // I needone of these
    flag = DDPF_FOURCC | DDPF_RGB;
    if ((header.sPixelFormat.dwFlags & flag) == 0) {
        goto quick_exit;
    }
    if (header.sPixelFormat.dwSize != 32) {
        goto quick_exit;
    }
    if ((header.sCaps.dwCaps1 & DDSCAPS_TEXTURE) == 0) {
        goto quick_exit;
    }
    //	make sure it is a type we can upload
    if ((header.sPixelFormat.dwFlags & DDPF_FOURCC) &&
        !((header.sPixelFormat.dwFourCC ==
           (('D' << 0) | ('X' << 8) | ('T' << 16) | ('1' << 24))) ||
          (header.sPixelFormat.dwFourCC ==
           (('D' << 0) | ('X' << 8) | ('T' << 16) | ('3' << 24))) ||
          (header.sPixelFormat.dwFourCC ==
           (('D' << 0) | ('X' << 8) | ('T' << 16) | ('5' << 24))))) {
        goto quick_exit;
    }
    //	OK, validated the header, let's load the image data
    last_result_description = "DDS header loaded and validated";
    width = header.dwWidth;
    height = header.dwHeight;
    uncompressed =
        1 - (header.sPixelFormat.dwFlags & DDPF_FOURCC) / DDPF_FOURCC;
    cubemap = (header.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP) / DDSCAPS2_CUBEMAP;
    if (uncompressed) {
        S3TC_type = GL_RGB;
        block_size = 3;
        if (header.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
            S3TC_type = GL_RGBA;
            block_size = 4;
        }
        DDS_main_size = width * height * block_size;
    } else {
        //	can we even handle direct uploading to OpenGL DXT compressed
        // images?
        if (GetDxtCapability() != LoadCapability::kPresent) {
            //	we can't do it!
            last_result_description =
                "Direct upload of S3TC images not supported by the OpenGL "
                "driver";
            return std::nullopt;
        }
        //	well, we know it is DXT1/3/5, because we checked above
        switch ((header.sPixelFormat.dwFourCC >> 24) - '0') {
            case 1:
                S3TC_type = SOIL_RGBA_S3TC_DXT1;
                block_size = 8;
                break;
            case 3:
                S3TC_type = SOIL_RGBA_S3TC_DXT3;
                block_size = 16;
                break;
            case 5:
                S3TC_type = SOIL_RGBA_S3TC_DXT5;
                block_size = 16;
                break;
        }
        DDS_main_size = ((width + 3) >> 2) * ((height + 3) >> 2) * block_size;
    }
    if (cubemap) {
        // does the user want a cubemap?
        if (!loading_as_cubemap) {
            //	we can't do it!
            last_result_description = "DDS image was a cubemap";
            return std::nullopt;
        }
        //	can we even handle cubemaps with the OpenGL driver?
        if (GetCubemapCapability() != LoadCapability::kPresent) {
            //	we can't do it!
            last_result_description =
                "Direct upload of cubemap images not supported by the OpenGL "
                "driver";
            return std::nullopt;
        }
        ogl_target_start = SOIL_TEXTURE_CUBE_MAP_POSITIVE_X;
        ogl_target_end = SOIL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        opengl_texture_type = SOIL_TEXTURE_CUBE_MAP;
    } else {
        // does the user want a non-cubemap?
        if (loading_as_cubemap) {
            //	we can't do it!
            last_result_description = "DDS image was not a cubemap";
            return std::nullopt;
        }
        ogl_target_start = GL_TEXTURE_2D;
        ogl_target_end = GL_TEXTURE_2D;
        opengl_texture_type = GL_TEXTURE_2D;
    }
    if ((header.sCaps.dwCaps1 & DDSCAPS_MIPMAP) && (header.dwMipMapCount > 1)) {
        int shift_offset;
        mipmaps = header.dwMipMapCount - 1;
        DDS_full_size = DDS_main_size;
        if (uncompressed) {
            /*	uncompressed DDS, simple MIPmap size calculation	*/
            shift_offset = 0;
        } else {
            /*	compressed DDS, MIPmap size calculation is block based	*/
            shift_offset = 2;
        }
        for (i = 1; i <= mipmaps; ++i) {
            int w, h;
            w = width >> (shift_offset + i);
            h = height >> (shift_offset + i);
            if (w < 1) {
                w = 1;
            }
            if (h < 1) {
                h = 1;
            }
            DDS_full_size += w * h * block_size;
        }
    } else {
        mipmaps = 0;
        DDS_full_size = DDS_main_size;
    }
    DDS_data = (uint8_t *)malloc(DDS_full_size);
    //	got the image data RAM, create or use an existing OpenGL texture handle
    tex_ID = reuse_texture_ID;
    if (tex_ID == 0) {
        glGenTextures(1, &tex_ID.value());
    }
    //  bind an OpenGL texture ID
    glBindTexture(opengl_texture_type, tex_ID.value());
    //	do this for each face of the cubemap!
    for (cf_target = ogl_target_start; cf_target <= ogl_target_end;
         ++cf_target) {
        if (buffer_index + DDS_full_size <= buffer_length) {
            unsigned int byte_offset = DDS_main_size;
            memcpy((void *)DDS_data, (const void *)(&buffer[buffer_index]),
                   DDS_full_size);
            buffer_index += DDS_full_size;
            //	upload the main chunk
            if (uncompressed) {
                //	and remember, DXT uncompressed uses BGR(A),so swap to
                // RGB(A) for ALL MIPmap levels
                for (i = 0; i < DDS_full_size; i += block_size) {
                    uint8_t temp = DDS_data[i];
                    DDS_data[i] = DDS_data[i + 2];
                    DDS_data[i + 2] = temp;
                }
                glTexImage2D(cf_target, 0, S3TC_type, width, height, 0,
                             S3TC_type, GL_UNSIGNED_BYTE, DDS_data);
            } else {
                glCompressedTexImage2DARB(cf_target, 0, S3TC_type, width,
                                          height, 0, DDS_main_size, DDS_data);
            }
            //	upload the mipmaps, if we have them
            for (i = 1; i <= mipmaps; ++i) {
                int w, h, mip_size;
                w = width >> i;
                h = height >> i;
                if (w < 1) {
                    w = 1;
                }
                if (h < 1) {
                    h = 1;
                }
                //	upload this mipmap
                if (uncompressed) {
                    mip_size = w * h * block_size;
                    glTexImage2D(cf_target, i, S3TC_type, w, h, 0, S3TC_type,
                                 GL_UNSIGNED_BYTE, &DDS_data[byte_offset]);
                } else {
                    mip_size = ((w + 3) / 4) * ((h + 3) / 4) * block_size;
                    glCompressedTexImage2DARB(cf_target, i, S3TC_type, w, h, 0,
                                              mip_size, &DDS_data[byte_offset]);
                }
                //	and move to the next mipmap
                byte_offset += mip_size;
            }
            //	it worked!
            last_result_description = "DDS file loaded";
        } else {
            glDeleteTextures(1, &tex_ID.value());
            tex_ID = 0;
            cf_target = ogl_target_end + 1;
            last_result_description =
                "DDS file was too small for expected image data";
        }
    }  // end reading each face
    FreeImageData(DDS_data);
    if (tex_ID) {
        //	did I have MIPmaps?
        if (mipmaps > 0) {
            //	instruct OpenGL to use the MIPmaps
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
        } else {
            //	instruct OpenGL _NOT_ to use the MIPmaps
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);
        }
        //	does the user want clamping, or wrapping?
        if (flags & Flags::kTextureRepeats) {
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(opengl_texture_type, SOIL_TEXTURE_WRAP_R,
                            GL_REPEAT);
        } else {
            //	unsigned int clamp_mode = SOIL_CLAMP_TO_EDGE;
            unsigned int clamp_mode = GL_CLAMP;
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_S, clamp_mode);
            glTexParameteri(opengl_texture_type, GL_TEXTURE_WRAP_T, clamp_mode);
            glTexParameteri(opengl_texture_type, SOIL_TEXTURE_WRAP_R,
                            clamp_mode);
        }
    }

quick_exit:
    //	report success or failure
    return tex_ID;
}

std::optional<uint32_t> DirectLoadDds(std::string filename,
                                      unsigned int reuse_texture_ID, int flags,
                                      bool loading_as_cubemap) {
    FILE *f;
    uint8_t *buffer;
    size_t buffer_length, bytes_read;
    std::optional<uint32_t> tex_ID{std::nullopt};
    //	error checks
    if (!std::filesystem::exists(filename)) {
        last_result_description = "NULL filename";
        return std::nullopt;
    }
    f = fopen(filename.c_str(), "rb");
    if (NULL == f) {
        //	the file doesn't seem to exist (or be open-able)
        last_result_description = "Can not find DDS file";
        return std::nullopt;
    }
    fseek(f, 0, SEEK_END);
    buffer_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = (uint8_t *)malloc(buffer_length);
    if (NULL == buffer) {
        last_result_description = "malloc failed";
        fclose(f);
        return std::nullopt;
    }
    bytes_read = fread((void *)buffer, 1, buffer_length, f);
    fclose(f);
    if (bytes_read < buffer_length) {
        //	huh?
        buffer_length = bytes_read;
    }
    //	now try to do the loading
    tex_ID =
        DirectLoadDdsFromMemory((const uint8_t *const)buffer, buffer_length,
                                reuse_texture_ID, flags, loading_as_cubemap);
    FreeImageData(buffer);
    return tex_ID;
}

LoadCapability GetNpotCapability(void) {
    //	check for the capability
    if (has_NPOT_capability == LoadCapability::kUnknown) {
        //	we haven't yet checked for the capability, do so
        if (GLEW_ARB_texture_non_power_of_two) {
            //	it's there!
            has_NPOT_capability = LoadCapability::kPresent;
        } else {
            //	not there, flag the failure
            has_NPOT_capability = LoadCapability::kNone;
        }
    }
    //	let the user know if we can do non-power-of-two textures or not
    return has_NPOT_capability;
}

LoadCapability GetTexRectangleCapability(void) {
    //	check for the capability
    if (has_tex_rectangle_capability == LoadCapability::kUnknown) {
        //	we haven't yet checked for the capability, do so
        if (GLEW_ARB_texture_rectangle && GLEW_EXT_texture_rectangle &&
            GLEW_NV_texture_rectangle) {
            //	it's there!
            has_tex_rectangle_capability = LoadCapability::kPresent;
        } else {
            //	not there, flag the failure
            has_tex_rectangle_capability = LoadCapability::kNone;
        }
    }
    //	let the user know if we can do texture rectangles or not
    return has_tex_rectangle_capability;
}

LoadCapability GetCubemapCapability(void) {
    //	check for the capability
    if (has_cubemap_capability == LoadCapability::kUnknown) {
        //	we haven't yet checked for the capability, do so
        if (GLEW_ARB_texture_cube_map && GLEW_EXT_texture_cube_map) {
            //	it's there!
            has_cubemap_capability = LoadCapability::kPresent;
        } else {
            has_cubemap_capability = LoadCapability::kNone;
        }
    }
    //	let the user know if we can do cubemaps or not
    return has_cubemap_capability;
}

LoadCapability GetDxtCapability(void) {
    //	check for the capability
    if (has_DXT_capability == LoadCapability::kUnknown) {
        //	we haven't yet checked for the capability, do so
        if (!GLEW_EXT_texture_compression_s3tc) {
            //	not there, flag the failure
            has_DXT_capability = LoadCapability::kNone;
        } else {
            //	all's well!
            has_DXT_capability = LoadCapability::kPresent;
        }
    }
    //	let the user know if we can do DXT or not
    return has_DXT_capability;
}
}  // namespace soil
