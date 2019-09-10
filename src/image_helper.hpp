/*
    Jonathan Dummer

    Image helper functions

    MIT license
*/

#ifndef HEADER_IMAGE_HELPER
#define HEADER_IMAGE_HELPER

#include <cstdint>
#include <vector>

namespace soil::internal {

/**
        This function upscales an image.
        Not to be used to create MIPmaps,
        but to make it square,
        or to make it a power-of-two sized.
**/
bool UpscaleImage(const std::vector<uint8_t>& orig, size_t width, size_t height,
                  size_t channels, std::vector<uint8_t>& resampled,
                  size_t resampled_width, size_t resampled_height);

/**
        This function downscales an image.
        Used for creating MIPmaps,
        the incoming image should be a
        power-of-two sized.
**/
bool MipmapImage(const std::vector<uint8_t>& orig, size_t width, size_t height,
                 size_t channels, std::vector<uint8_t>& resampled,
                 size_t block_size_x, size_t block_size_y);

/**
        This function takes the RGB components of the image
        and scales each channel from [0,255] to [16,235].
        This makes the colors "Safe" for display on NTSC
        displays.  Note that this is _NOT_ a good idea for
        loading images like normal- or height-maps!
**/
bool ScaleImageRgbToNtscSafe(std::vector<uint8_t>& orig, size_t width,
                             size_t height, size_t channels);

/**
        This function takes the RGB components of the image
        and converts them into YCoCg.  3 components will be
        re-ordered to CoYCg (for optimum DXT1 compression),
        while 4 components will be ordered CoCgAY (for DXT5
        compression).
**/
bool ConvertRgbToYcocg(std::vector<uint8_t>& orig, size_t width, size_t height,
                       size_t channels);

/**
        This function takes the YCoCg components of the image
        and converts them into RGB.  See above.
**/
bool ConvertYcocgToRgb(std::vector<uint8_t>& orig, size_t width, size_t height,
                       size_t channels);

/**
        Converts an HDR image from an array
        of unsigned chars (RGBE) to RGBdivA
        \return 0 if failed, otherwise returns 1
**/
bool RgbeToRgbDivA(std::vector<uint8_t>& image, size_t width, size_t height,
                   bool rescale_to_max);

/**
        Converts an HDR image from an array
        of unsigned chars (RGBE) to RGBdivA2
        \return 0 if failed, otherwise returns 1
**/
bool RgbeToRgbDivA2(std::vector<uint8_t>& image, size_t width, size_t height,
                    bool rescale_to_max);

}  // namespace soil::internal

#endif /* HEADER_IMAGE_HELPER	*/
