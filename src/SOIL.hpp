/**
        @mainpage SOIL

        Jonathan Dummer
        2007-07-26-10.36

        Simple OpenGL Image Library

        A tiny c library for uploading images as
        textures into OpenGL.  Also saving and
        loading of images is supported.

        I'm using Sean's Tool Box image loader as a base:
        http://www.nothings.org/

        I'm upgrading it to load TGA and DDS files, and a direct
        path for loading DDS files straight into OpenGL textures,
        when applicable.

        Image Formats:
        - BMP		load & save
        - TGA		load & save
        - DDS		load & save
        - PNG		load
        - JPG		load

        OpenGL Texture Features:
        - resample to power-of-two sizes
        - MIPmap generation
        - compressed texture S3TC formats (if supported)
        - can pre-multiply alpha for you, for better compositing
        - can flip image about the y-axis (except pre-compressed DDS files)

        Thanks to:
        * Sean Barret - for the awesome stb_image
        * Dan Venkitachalam - for finding some non-compliant DDS files, and
patching some explicit casts
        * everybody at gamedev.net
**/

#ifndef HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY
#define HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY

#include <optional>

namespace soil {

/**
 * @brief The format of images that may be loaded (force_channels).
 */
enum LoadFormats {
    kAuto = 0,           /// leaves the image in whatever format it was found
    kLuminous = 1,       /// forces the image to load as Luminous (greyscale)
    kLuminousAlpha = 2,  /// forces the image to load as Luminous with Alpha
    kRgb = 3,            /// forces the image to load as Red Green Blue
    kRgba = 4            /// forces the image to load as Red Green Blue Alpha
};

/**
 * @brief Passed in as reuse_texture_ID, will cause SOIL to register a new
 * texture ID using glGenTextures(). If the value passed into reuse_texture_ID >
 * 0 then SOIL will just re-use that texture ID (great for reloading image
 * assets in-game!)
 */
constexpr uint8_t kSoilCreateNewId = 0;

/**
 * @brief flags you can pass into LoadOglTexture() and CreateOglTexture().
 *
 * @note  if kDdsLoadDirect is used the rest of the flags with the exception of
 * kTextureRepeats will be ignored while loading already-compressed DDS files.
 */
enum Flags {
    kPowerOfTwo = 1,      /// force the image to be POT
    kMipMaps = 2,         /// generate mipmaps for the texture
    kTextureRepeats = 4,  /// otherwise will clamp
    kMultiplyAlpha = 8,   /// for using (GL_ONE,GL_ONE_MINUS_SRC_ALPHA) blending
    kInvertY = 16,        /// flip the image vertically
    kCompressToDxt = 32,  /// if the card can display them, will convert RGB to
                          /// DXT1, RGBA to DXT5
    kDdsLoadDirect = 64,  /// will load DDS files directly without _ANY_
                          /// additional processing
    kNtscSafeRgb = 128,   /// clamps RGB components to the range [16,235]
    kCoCgY = 256,         /// Google YCoCg; RGB=>CoYCg,RGBA=>CoCgAY
    kTextureRectangle = 512  /// uses ARB_texture_rectangle; pixel indexed & no
                             /// repeat or MIPmaps or cubemaps
};

/**
 * @brief The types of images that may be saved.
 */
enum SaveTypes {
    kTga = 0,  /// (TGA supports uncompressed RGB / RGBA)
    kBmp = 1,  /// (BMP supports uncompressed RGB)
    kDds = 2   /// (DDS supports DXT1 and DXT5)
};

/**
        Defines the order of faces in a DDS cubemap.
        I recommend that you use the same order in single
        image cubemap files, so they will be interchangeable
        with DDS cubemaps when using SOIL.
**/
#define SOIL_DDS_CUBEMAP_FACE_ORDER "EWUDNS"

/**
 * @brief The types of internal fake HDR representations
 */
enum HdrTypes {
    kRgbe = 0,     /// RGB * pow( 2.0, A - 128.0 )
    kRgbDivA = 1,  /// RGB / A
    kRgbDivA2 = 2  /// RGB / (A*A)
};

/**
 * @brief Loads an image from disk into an OpenGL texture.
 *
 * @param filename filename the name of the file to upload as a texture
 * @param force_channels force_channels 0-image format, 1-luminous,
 * 2-luminous/alpha, 3-RGB, 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of kPowerOfTwo | kMipMaps | kTextureRepeats |
 * kMultiplyAlpha | kInvertY | kCompressToDxt | kDdsLoadDirect
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglTexture(const char *filename, int force_channels,
                            unsigned int reuse_texture_ID, unsigned int flags);

/**
 * @brief Loads 6 images from disk into an OpenGL cubemap texture.
 *
 * @param x_pos_file the name of the file to upload as the +x cube face
 * @param x_neg_file the name of the file to upload as the -x cube face
 * @param y_pos_file the name of the file to upload as the +y cube face
 * @param y_neg_file the name of the file to upload as the -y cube face
 * @param z_pos_file the name of the file to upload as the +z cube face
 * @param z_neg_file the name of the file to upload as the -z cube face
 * @param force_channels force_channels force_channels 0-image format,
 * 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglCubemap(const char *x_pos_file, const char *x_neg_file,
                            const char *y_pos_file, const char *y_neg_file,
                            const char *z_pos_file, const char *z_neg_file,
                            int force_channels, unsigned int reuse_texture_ID,
                            unsigned int flags);

/**
 * @brief Loads 1 image from disk and splits it into an OpenGL cubemap texture.
 *
 * @param filename the name of the file to upload as a texture
 * @param face_order face_order the order of the faces in the file, any
 * combination of NSWEUD, for North, South, Up, etc.
 * @param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB,
 * 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglSingleCubemap(const char *filename,
                                  const char face_order[6], int force_channels,
                                  unsigned int reuse_texture_ID,
                                  unsigned int flags);

/**
 * @brief Loads an HDR image from disk into an OpenGL texture.
 *
 * @param filename the name of the file to upload as a texture
 * @param fake_HDR_format kRgbe, kRgbDivA, kRgbDivA2
 * @param rescale_to_max
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglHdrTexture(const char *filename, int fake_HDR_format,
                               int rescale_to_max,
                               unsigned int reuse_texture_ID,
                               unsigned int flags);

/**
 * @brief Loads an image from RAM into an OpenGL texture.
 *
 * @param buffer the image data in RAM just as if it were still in a file
 * @param buffer_length the size of the buffer in bytes
 * @param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB,
 * 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglTextureFromMemory(const unsigned char *const buffer,
                                      int buffer_length, int force_channels,
                                      unsigned int reuse_texture_ID,
                                      unsigned int flags);

/**
 * @brief Loads 6 images from memory into an OpenGL cubemap texture.
 *
 * @param x_pos_buffer the image data in RAM to upload as the +x cube face
 * @param x_pos_buffer_length the size of the above buffer
 * @param x_neg_buffer the image data in RAM to upload as the -x cube face
 * @param x_neg_buffer_length the size of the above buffer
 * @param y_pos_buffer the image data in RAM to upload as the +y cube face
 * @param y_pos_buffer_length the size of the above buffer
 * @param y_neg_buffer the image data in RAM to upload as the -y cube face
 * @param y_neg_buffer_length the size of the above buffer
 * @param z_pos_buffer the image data in RAM to upload as the +z cube face
 * @param z_pos_buffer_length the size of the above buffer
 * @param z_neg_buffer the image data in RAM to upload as the -z cube face
 * @param z_neg_buffer_length the size of the above buffer
 * @param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB,
 * 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglCubemapFromMemory(
    const unsigned char *const x_pos_buffer, int x_pos_buffer_length,
    const unsigned char *const x_neg_buffer, int x_neg_buffer_length,
    const unsigned char *const y_pos_buffer, int y_pos_buffer_length,
    const unsigned char *const y_neg_buffer, int y_neg_buffer_length,
    const unsigned char *const z_pos_buffer, int z_pos_buffer_length,
    const unsigned char *const z_neg_buffer, int z_neg_buffer_length,
    int force_channels, unsigned int reuse_texture_ID, unsigned int flags);

/**
 * @brief Loads 1 image from RAM and splits it into an OpenGL cubemap texture.
 *
 * @param buffer the image data in RAM just as if it were still in a file
 * @param buffer_length the size of the buffer in bytes
 * @param face_order the order of the faces in the file, and combination of
 * NSWEUD, for North, South, Up, etc.
 * @param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB,
 * 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int LoadOglSingleCubemapFromMemory(const unsigned char *const buffer,
                                            int buffer_length,
                                            const char face_order[6],
                                            int force_channels,
                                            unsigned int reuse_texture_ID,
                                            unsigned int flags);

/**
 * @brief Creates a 2D OpenGL texture from raw image data.
 *
 * @note the raw data is _NOT_ freed after the upload (so the user can load
 * various versions)
 *
 * @param data the raw data to be uploaded as an OpenGL texture
 * @param width the width of the image in pixels
 * @param height the height of the image in pixels
 * @param channels the number of channels: 1-luminous, 2-luminous/alpha, 3-RGB,
 * 4-RGBA
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int CreateOglTexture(const unsigned char *const data, int width,
                              int height, int channels,
                              unsigned int reuse_texture_ID,
                              unsigned int flags);

/**
 * @brief Creates an OpenGL cubemap texture by splitting up 1 image into 6
 * parts.
 *
 * @param data the raw data to be uploaded as an OpenGL texture
 * @param width the width of the image in pixels
 * @param height the height of the image in pixels
 * @param channels the number of channels: 1-luminous, 2-luminous/alpha, 3-RGB,
 * 4-RGBA
 * @param face_order the order of the faces in the file, and combination of
 * NSWEUD, for North, South, Up, etc.
 * @param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the
 * texture ID (overwriting the old texture)
 * @param flags can be any of soil::Flags
 * @return unsigned int 0-failed, otherwise returns the OpenGL texture handle
 */
unsigned int CreateOglSingleCubemap(const unsigned char *const data, int width,
                                    int height, int channels,
                                    const char face_order[6],
                                    unsigned int reuse_texture_ID,
                                    unsigned int flags);

/**
 * @brief Captures the OpenGL window (RGB) and saves it to disk
 *
 * @param filename
 * @param image_type
 * @param x
 * @param y
 * @param width
 * @param height
 * @return int 0 if it failed, otherwise returns 1
 */
int SaveScreenshot(const char *filename, int image_type, int x, int y,
                   int width, int height);

/**
 * @brief Loads an image from disk into an array of unsigned chars.
 *
 * @note *channels return the original channel count of the image.  If
 * force_channels was other than LoadFormats::kAuto, the resulting image has
 * force_channels, but *channels may be different (if the original image had a
 * different channel count).
 *
 * @param filename
 * @param width
 * @param height
 * @param channels
 * @param force_channels
 * @return unsigned char*
 */
unsigned char *LoadImage(const char *filename, int *width, int *height,
                         int *channels, int force_channels);

/**
 * @brief Loads an image from memory into an array of unsigned chars.
 *
 * @note *channels return the original channel count of the
        image.  If force_channels was other than LoadFormats::kAuto,
        the resulting image has force_channels, but *channels may be
        different (if the original image had a different channel
        count).
 *
 * @param buffer
 * @param buffer_length
 * @param width
 * @param height
 * @param channels
 * @param force_channels
 * @return unsigned char*
 */
unsigned char *LoadImageFromMemory(const unsigned char *const buffer,
                                   int buffer_length, int *width, int *height,
                                   int *channels, int force_channels);

/**
 * @brief Saves an image from an array of unsigned chars (RGBA) to disk
 *
 * @param filename
 * @param image_type
 * @param width
 * @param height
 * @param channels
 * @param data
 * @return int 0 if failed, otherwise returns 1
 */
int SaveImage(const char *filename, int image_type, int width, int height,
              int channels, const unsigned char *const data);

/**
 * @brief  Frees the image data
 * @note, this is just C's "free()"...this function is present mostly so C++
 * programmers don't forget to use "free()" and call "delete []" instead [8^)
 *
 * @param img_data
 */
void FreeImageData(unsigned char *img_data);

/**
 * @brief This function resturn a pointer to a string describing the last thing
 * that happened inside SOIL. It can be used to determine why an image failed to
 * load.
 *
 * @return const char*
 */
const char *GetLastResult(void);

}  // namespace soil

#endif /* HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY	*/