/*
    Jonathan Dummer

    image helper functions

    MIT license
*/

#include "image_helper.hpp"
#include <math.h>
#include <stdlib.h>
#include <algorithm>

namespace soil::internal {
namespace {  // internal file functions

// prototypes
float FindMaxRgbe(std::vector<uint8_t>& image, size_t width, size_t height);
uint8_t ClampByte(int x);
uint8_t ClampByte(uint x);

float FindMaxRgbe(std::vector<uint8_t>& image, size_t width, size_t height) {
    float max_val = 0.0f;
    auto img = image.begin();
    for (size_t i = width * height; i > 0; --i) {
        /* float scale = powf( 2.0f, img[3] - 128.0f ) / 255.0f; */
        float scale = ldexp(1.0f / 255.0f, static_cast<int>(img[3]) - 128);
        for (size_t j = 0; j < 3; ++j) {
            if (img[j] * scale > max_val) {
                max_val = static_cast<float>(img[j]) * scale;
            }
        }
        /* next pixel */
        img += 4;
    }
    return max_val;
}

uint8_t ClampByte(int x) {
    return static_cast<uint8_t>(std::clamp(x, 0, UINT8_MAX));
}

uint8_t ClampByte(uint x) { return ClampByte(static_cast<int>(x)); }

}  // namespace

/*	Upscaling the image uses simple bilinear interpolation	*/
bool UpscaleImage(const std::vector<uint8_t>& orig, size_t width, size_t height,
                  size_t channels, std::vector<uint8_t>& resampled,
                  size_t resampled_width, size_t resampled_height) {
    /* error(s) check	*/
    if ((width == 0) || (height == 0) || (resampled_width < 2) ||
        (resampled_height < 2) || (channels < 1)) {
        /*	signify badness	*/
        return false;
    }
    /*
                for each given pixel in the new map, find the exact location
                from the original map which would contribute to this guy
        */
    float dx = (width - 1.0f) / (resampled_width - 1.0f);
    float dy = (height - 1.0f) / (resampled_height - 1.0f);
    for (size_t y = 0; y < resampled_height; ++y) {
        /* find the base y index and fractional offset from that	*/
        float sample_y = y * dy;
        /*	if( int_y < 0 ) { int_y = 0; } else	*/
        size_t int_y = std::min(height - 2, static_cast<size_t>(sample_y));
        sample_y -= int_y;
        for (size_t x = 0; x < resampled_width; ++x) {
            float sample_x = x * dx;
            size_t int_x = std::min(width - 2, static_cast<size_t>(sample_x));
            sample_x -= int_x;
            /*	base index into the original image	*/
            size_t base_index = (int_y * width + int_x) * channels;
            for (size_t c = 0; c < channels; ++c) {
                /*	do the sampling	*/
                float value = 0.5f;
                value +=
                    orig[base_index] * (1.0f - sample_x) * (1.0f - sample_y);
                value += orig[base_index + channels] * (sample_x) *
                         (1.0f - sample_y);
                value += orig[base_index + width * channels] *
                         (1.0f - sample_x) * (sample_y);
                value += orig[base_index + width * channels + channels] *
                         (sample_x) * (sample_y);
                /*	move to the next channel	*/
                ++base_index;
                /*	save the new value	*/
                resampled[y * resampled_width * channels + x * channels + c] =
                    static_cast<uint8_t>(value);
            }
        }
    }
    /*	done	*/
    return true;
}

bool MipmapImage(const std::vector<uint8_t>& orig, size_t width, size_t height,
                 size_t channels, std::vector<uint8_t>& resampled,
                 size_t block_size_x, size_t block_size_y) {
    /*	error check	*/
    if ((width == 0) || (height == 0) || (channels < 1) ||
        (block_size_x == 0) || (block_size_y == 0)) {
        /*	nothing to do	*/
        return false;
    }
    size_t mip_width = width / block_size_x;
    size_t mip_height = height / block_size_y;
    mip_width = std::max(static_cast<size_t>(1), mip_width);
    mip_height = std::max(static_cast<size_t>(1), mip_height);
    for (size_t j = 0; j < mip_height; ++j) {
        for (size_t i = 0; i < mip_width; ++i) {
            for (size_t c = 0; c < channels; ++c) {
                const size_t index = (j * block_size_y) * width * channels +
                                     (i * block_size_x) * channels + c;
                size_t u_block = block_size_x;
                size_t v_block = block_size_y;
                /*	do a bit of checking so we don't over-run the boundaries
                        (necessary for non-square textures!)	*/
                if (block_size_x * (i + 1) > width) {
                    u_block = width - i * block_size_y;
                }
                if (block_size_y * (j + 1) > height) {
                    v_block = height - j * block_size_y;
                }
                size_t block_area = u_block * v_block;
                /*	for this pixel, see what the average
                        of all the values in the block are.
                        note: start the sum at the rounding value, not at 0
                 */
                size_t sum_value = block_area >> 1;
                for (size_t v = 0; v < v_block; ++v)
                    for (size_t u = 0; u < u_block; ++u) {
                        sum_value +=
                            orig[index + v * width * channels + u * channels];
                    }
                resampled[j * mip_width * channels + i * channels + c] =
                    static_cast<uint8_t>(sum_value / block_area);
            }
        }
    }
    return true;
}

bool ScaleImageRgbToNtscSafe(std::vector<uint8_t>& orig, size_t width,
                             size_t height, size_t channels) {
    constexpr float scale_lo = 16.0f - 0.499f;
    constexpr float scale_hi = 235.0f + 0.499f;
    size_t nc = channels;
    uint8_t scale_LUT[256];  // scaling Look Up Table
    /*	error check	*/
    if ((width == 0) || (height == 0) || (channels < 1)) {
        /*	nothing to do	*/
        return false;
    }
    /*	set up the scaling Look Up Table	*/
    for (size_t i = 0; i < 256; ++i) {
        scale_LUT[i] =
            static_cast<uint8_t>((scale_hi - scale_lo) * i / 255.0f + scale_lo);
    }
    /*	for channels = 2 or 4, ignore the alpha component	*/
    nc -= 1 - (channels & 1);
    /*	OK, go through the image and scale any non-alpha components	*/
    for (size_t i = 0; i < width * height * channels; i += channels) {
        for (size_t j = 0; j < nc; ++j) {
            orig[i + j] = scale_LUT[orig[i + j]];
        }
    }
    return true;
}

/*
        This function takes the RGB components of the image
        and converts them into YCoCg.  3 components will be
        re-ordered to CoYCg (for optimum DXT1 compression),
        while 4 components will be ordered CoCgAY (for DXT5
        compression).
*/
bool ConvertRgbToYcocg(std::vector<uint8_t>& orig, size_t width, size_t height,
                       size_t channels) {
    /*	error check	*/
    if ((width == 0) || (height == 0) || (channels < 3) || (channels > 4)) {
        /*	nothing to do	*/
        return false;
    }
    /*	do the conversion	*/
    if (channels == 3) {
        for (size_t i = 0; i < width * height * 3; i += 3) {
            uint32_t r = orig[i + 0];
            uint32_t g = (orig[i + 1] + 1) >> 1;
            uint32_t b = orig[i + 2];
            uint32_t tmp = (2 + r + b) >> 2;
            /*	Co	*/
            orig[i + 0] = ClampByte(128 + ((r - b + 1) >> 1));
            /*	Y	*/
            orig[i + 1] = ClampByte(g + tmp);
            /*	Cg	*/
            orig[i + 2] = ClampByte(128 + g - tmp);
        }
    } else {
        for (size_t i = 0; i < width * height * 4; i += 4) {
            uint r = orig[i + 0];
            uint g = (orig[i + 1] + 1) >> 1;
            uint b = orig[i + 2];
            uint8_t a = orig[i + 3];
            uint tmp = (2 + r + b) >> 2;
            /*	Co	*/
            orig[i + 0] = ClampByte(128 + ((r - b + 1) >> 1));
            /*	Cg	*/
            orig[i + 1] = ClampByte(128 + g - tmp);
            /*	Alpha	*/
            orig[i + 2] = a;
            /*	Y	*/
            orig[i + 3] = ClampByte(g + tmp);
        }
    }
    /*	done	*/
    return true;
}

/*
        This function takes the YCoCg components of the image
        and converts them into RGB.  See above.
*/
bool ConvertYcocgToRgb(std::vector<uint8_t>& orig, size_t width, size_t height,
                       size_t channels) {
    /*	error check	*/
    if ((width == 0) || (height == 0) || (channels < 3) || (channels > 4)) {
        /*	nothing to do	*/
        return false;
    }
    /*	do the conversion	*/
    if (channels == 3) {
        for (size_t i = 0; i < width * height * 3; i += 3) {
            int co = orig[i + 0] - 128;
            int y = orig[i + 1];
            int cg = orig[i + 2] - 128;
            /*	R	*/
            orig[i + 0] = ClampByte(y + co - cg);
            /*	G	*/
            orig[i + 1] = ClampByte(y + cg);
            /*	B	*/
            orig[i + 2] = ClampByte(y - co - cg);
        }
    } else {
        for (size_t i = 0; i < width * height * 4; i += 4) {
            int co = orig[i + 0] - 128;
            int cg = orig[i + 1] - 128;
            uint8_t a = orig[i + 2];
            int y = orig[i + 3];
            /*	R	*/
            orig[i + 0] = ClampByte(y + co - cg);
            /*	G	*/
            orig[i + 1] = ClampByte(y + cg);
            /*	B	*/
            orig[i + 2] = ClampByte(y - co - cg);
            /*	A	*/
            orig[i + 3] = a;
        }
    }
    /*	done	*/
    return true;
}

bool RgbeToRgbDivA(std::vector<uint8_t>& image, size_t width, size_t height,
                   bool rescale_to_max) {
    /* local variables */
    auto img = image.begin();
    float scale = 1.0f;
    /* error check */
    if ((width == 0) || (height == 0)) {
        return false;
    }
    /* convert (note: no negative numbers, but 0.0 is possible) */
    if (rescale_to_max) {
        scale = 255.0f / FindMaxRgbe(image, width, height);
    }
    for (size_t i = width * height; i > 0; --i) {
        /* decode this pixel, and find the max */
        /* e = scale * powf( 2.0f, img[3] - 128.0f ) / 255.0f; */
        float e = scale * ldexp(1.0f / 255.0f, static_cast<int>(img[3]) - 128);
        float r = e * img[0];
        float g = e * img[1];
        float b = e * img[2];
        float m = std::max({r, g, b});
        /* and encode it into RGBdivA */
        uint integer_value = (m != 0.0f) ? static_cast<uint>(255.0f / m) : 1.0f;
        integer_value = std::max(1U, integer_value);
        img[3] = std::min(255U, integer_value);
        integer_value = static_cast<uint>(img[3] * r + 0.5f);
        img[0] = std::min(255U, integer_value);
        integer_value = static_cast<uint>(img[3] * g + 0.5f);
        img[1] = std::min(255U, integer_value);
        integer_value = static_cast<uint>(img[3] * b + 0.5f);
        img[2] = std::min(255U, integer_value);
        /* and on to the next pixel */
        img += 4;
    }
    return true;
}

bool RgbeToRgbDivA2(std::vector<uint8_t>& image, size_t width, size_t height,
                    bool rescale_to_max) {
    auto img = image.begin();
    float scale = 1.0f;
    /* error check */
    if ((width == 0) || (height == 0)) {
        return false;
    }
    /* convert (note: no negative numbers, but 0.0 is possible) */
    if (rescale_to_max) {
        scale = 255.0f * 255.0f / FindMaxRgbe(image, width, height);
    }
    for (size_t i = width * height; i > 0; --i) {
        /* decode this pixel, and find the max */
        /* e = scale * powf( 2.0f, img[3] - 128.0f ) / 255.0f; */
        float e = scale * ldexp(1.0f / 255.0f, static_cast<int>(img[3]) - 128);
        float r = e * img[0];
        float g = e * img[1];
        float b = e * img[2];
        float m = std::max({r, g, b});
        /* and encode it into RGBdivA */
        uint integer_value =
            (m != 0.0f) ? static_cast<uint>(sqrtf(255.0f * 255.0f / m)) : 1U;
        integer_value = std::max(1U, integer_value);
        img[3] = std::min(255U, integer_value);
        integer_value = static_cast<uint>(img[3] * img[3] * r / 255.0f + 0.5f);
        img[0] = std::min(255U, integer_value);
        integer_value = static_cast<uint>(img[3] * img[3] * g / 255.0f + 0.5f);
        img[1] = std::min(255U, integer_value);
        integer_value = static_cast<uint>(img[3] * img[3] * b / 255.0f + 0.5f);
        img[2] = std::min(255U, integer_value);
        /* and on to the next pixel */
        img += 4;
    }
    return true;
}

}  // namespace soil::internal
