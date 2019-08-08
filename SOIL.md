# Simple OpenGL Image Library

## Introduction

SOIL is a tiny C library used primarily for uploading textures into OpenGL. It is based on stb_image version 1.16, the public domain code from Sean Barrett (found [here](https://github.com/nothings/stb)). I have extended it to load TGA and DDS files, and to perform common functions needed in loading OpenGL textures. SOIL can also be used to save and load images in a variety of formats (useful for loading height maps, non-OpenGL applications, etc.)

## Download

You can grab the latest version of SOIL [here](www.lonesock.net/files/soil.zip). (July 7, 2008: see the change log at the bottom of this page.)
You can also checkout the latest code from the new SVN repository, login as guest/guest:
svn://www.twisted-works.com/jdummer/public/SOIL
(thanks for the SVN hosting, Sherief!)

## License

Public Domain

## Features

- Readable Image Formats:
    BMP - non-1bpp, non-RLE (from stb_image documentation)
    PNG - non-interlaced (from stb_image documentation)
    JPG - JPEG baseline (from stb_image documentation)
    TGA - greyscale or RGB or RGBA or indexed, uncompressed or RLE
    DDS - DXT1/2/3/4/5, uncompressed, cubemaps (can't read 3D DDS files yet)
    PSD - (from stb_image documentation)
    HDR - converted to LDR, unless loaded with *HDR* functions (RGBE or RGBdivA or RGBdivA2)
- Writeable Image Formats:
    TGA - Greyscale or RGB or RGBA, uncompressed
    BMP - RGB, uncompressed
    DDS - RGB as DXT1, or RGBA as DXT5
- Can load an image file directly into a 2D OpenGL texture, optionally performing the following functions:
    Can generate a new texture handle, or reuse one specified
    Can automatically rescale the image to the next largest power-of-two size
    Can automatically create MIPmaps
    Can scale (not simply clamp) the RGB values into the "safe range" for NTSC displays (16 to 235, as recommended [here](http://msdn2.microsoft.com/en-us/library/bb174608.aspx#NTSC_Suggestions))
    Can multiply alpha on load (for more correct blending / compositing)
    Can flip the image vertically
    Can compress and upload any image as DXT1 or DXT5 (if EXT_texture_compression_s3tc is available), using an internal (very fast!) compressor
    Can convert the RGB to YCoCg color space (useful with DXT5 compression: see [this link](http://developer.nvidia.com/object/real-time-ycocg-dxt-compression.html) from NVIDIA)
    Will automatically downsize a texture if it is larger than GL_MAX_TEXTURE_SIZE
    Can directly upload DDS files (DXT1/3/5/uncompressed/cubemap, with or without MIPmaps). Note: directly uploading the compressed DDS image will disable the other options (no flipping, no pre-multiplying alpha, no rescaling, no creation of MIPmaps, no auto-downsizing)
    Can load rectangluar textures for GUI elements or splash screens (requires GL_ARB/EXT/NV_texture_rectangle)
- Can decompress images from RAM (e.g. via [PhysicsFS](http://icculus.org/physfs/) or similar) into an OpenGL texture (same features as regular 2D textures, above)
- Can load cube maps directly into an OpenGL texture (same features as regular 2D textures, above)
    Can take six image files directly into an OpenGL cube map texture
    Can take a single image file where width = 6*height (or vice versa), split it into an OpenGL cube map texture
- No external dependencies
- Tiny
- Cross platform (Windows, *nix, Mac OS X)
- Public Domain

## ToDo

- More testing
- Add HDR functions to load from memory and load to RGBE unsigned char*

## Usage

SOIL is meant to be used as a static library (as it's tiny and in the public domain). You can use the static library file included in the zip (libSOIL.a works for MinGW and Microsoft compilers...feel free to rename it to SOIL.lib if that makes you happy), or compile the library yourself. The code is cross-platform and has been tested on Windows, Linux, and Mac. (The heaviest testing has been on the Windows platform, so feel free to email me if you find any issues with other platforms.)

Simply include SOIL.h in your C or C++ file, link in the static library, and then use any of SOIL's functions. The file SOIL.h contains simple doxygen style documentation. (If you use the static library, no other header files are needed besides SOIL.h) Below are some simple usage examples:

```c
/* load an image file directly as a new OpenGL texture */
GLuint tex_2d = LoadOglTexture
    (
        "img.png",
        ImageChannels::kAuto,
        kSoilCreateNewId,
        Flags::kMipMaps | Flags::kInvertY | Flags::kNtscSafeRgb | Flags::kCompressToDxt
    );

/* check for an error during the load process */
if( 0 == tex_2d )
{
    printf( "SOIL loading error: '%s'\n", GetLastResult() );
}

/* load another image, but into the same texture ID, overwriting the last one */
tex_2d = LoadOglTexture
    (
        "some_other_img.dds",
        ImageChannels::kAuto,
        tex_2d,
        Flags::kDdsLoadDirect
    );

/* load 6 images into a new OpenGL cube map, forcing RGB */
GLuint tex_cube = LoadOglCubemap
    (
        "xp.jpg",
        "xn.jpg",
        "yp.jpg",
        "yn.jpg",
        "zp.jpg",
        "zn.jpg",
        ImageChannels::kRgb,
        kSoilCreateNewId,
        Flags::kMipMaps
    );

/* load and split a single image into a new OpenGL cube map, default format */
/* face order = East South West North Up Down => "ESWNUD", case sensitive! */
GLuint single_tex_cube = LoadOglSingleCubemap
    (
        "split_cubemap.png",
        "EWUDNS",
        ImageChannels::kAuto,
        kSoilCreateNewId,
        Flags::kMipMaps
    );

/* actually, load a DDS cubemap over the last OpenGL cube map, default format */
/* try to load it directly, but give the order of the faces in case that fails */
/* the DDS cubemap face order is pre-defined as SOIL_DDS_CUBEMAP_FACE_ORDER */
single_tex_cube = LoadOglSingleCubemap
    (
        "overwrite_cubemap.dds",
        SOIL_DDS_CUBEMAP_FACE_ORDER,
        ImageChannels::kAuto,
        single_tex_cube,
        Flags::kMipMaps | Flags::kDdsLoadDirect
    );

/* load an image as a heightmap, forcing greyscale (so channels should be 1) */
int width, height, channels;
unsigned char *ht_map = LoadImage
    (
        "terrain.tga",
        &width, &height, &channels,
        ImageChannels::kLuminous
    );

/* save that image as another type */
int save_result = SaveImage
    (
        "new_terrain.dds",
        SaveTypes::kDds,
        width, height, channels,
        ht_map
    );

/* save a screenshot of your awesome OpenGL game engine, running at 1024x768 */
save_result = SaveScreenshot
    (
        "awesomenessity.bmp",
        SaveTypes::kBmp,
        0, 0, 1024, 768
    );

/* loaded a file via PhysicsFS, need to decompress the image from RAM, */
/* where it's in a buffer: unsigned char *image_in_RAM */
GLuint tex_2d_from_RAM = LoadOglTextureFromMemory
    (
        image_in_RAM,
        image_in_RAM_bytes,
        ImageChannels::kAuto,
        kSoilCreateNewId,
        Flags::kMipMaps | Flags::kInvertY | Flags::kCompressToDxt
    );

/* done with the heightmap, free up the RAM */
FreeImageData( ht_map );
```

## Change Log

- July 7, 2008
    upgraded to stb_image 1.16 (threadsafe! loads PSD and HDR formats)
    removed __inline__ keyword from native SOIL functions (thanks Sherief, Boder, Amnesiac5!)
    added LoadOglHdrTexture (loads a Radience HDR file into RGBE, RGB/a, RGB/A^2)
    fixed a potential bug loading DDS files with a filename
    added a VC9 project file (thanks Sherief!)
- November 10, 2007: added Flags::kTextureRectangle (pixel addressed non POT, useful for GUI, splash screens, etc.). Not useful with cubemaps, and disables repeating and MIPmaps.
- November 8, 2007
    upgraded to stb_image 1.07
    fixed some includes and defines for compiling on OS X (thanks Mogui and swiftcoder!)
- October 30, 2007
    upgraded to stb_image 1.04, some tiny bug fixes
    there is now a makefile (under projects) for ease of building under Linux (thanks D J Peters!)
    Visual Studio 6/2003/2005 projects are working again
    patched SOIL for better pointer handling of the glCompressedTexImage2D extension (thanks Peter Sperl!)
    fixed DDS loading when force_channels=4 but there was no alpha; it was returning 3 channels. (Thanks LaurentGom!)
    fixed a bunch of channel issues in general. (Thanks Sean Barrett!)
- October 27, 2007
    correctly reports when there is no OpenGL context (thanks Merick Zero!)
    upgraded to stb_image 1.03 with support for loading the HDR image format
    fixed loading JPEG images while forcing the number of channels (e.g. to RGBA)
    changed SOIL_DDS_CUBEMAP_FACE_ORDER to a #define (thanks Dancho!)
    reorganized my additions to stb_image (you can define STBI_NO_DDS to compile SOIL without DDS support)
    added Flags::kCoCgY, will convert RGB or RGBA to YCoCg color space ([link](http://developer.nvidia.com/object/real-time-ycocg-dxt-compression.html))
- October 5, 2007
    added Flags::kNtscSafeRgb
    bugfixed & optimized up_scale_image (used with Flags::kPowerOfTwo and Flags::kMipMaps)
- September 20, 2007
    upgraded to stb_image 1.0
    added the DXT source files to the MSVS projects
    removed sqrtf() calls (VS2k3 could not handle them)
    distributing only 1 library file (libSOIL.a, compiled with MinGW 4.2.1 tech preview!) for all windows compilers
    added an example of the *_from_memory() functions to the Usage section
- September 6, 2007
    added a slew of SOIL_load_*_from_memory() functions for people using PhysicsFS or similar
    more robust loading of non-compliant DDS files (thanks Dan!)
- September 1, 2007 - fixed bugs from the last update [8^)
- August 31, 2007
    can load uncompressed and cubemap DDS files
    can create a cubemap texture from a single (stitched) image file of any type
    sped up the image resizing code
- August 24, 2007 - updated the documentation examples (at the bottom of this page)
- August 22, 2007
    can load cube maps (needs serious testing)
    can compress 1- or 2-channel images to DXT1/5
    fixed some malloc() casts
    fixed C++ style comments
    fixed includes to compile under *nix or Mac (hopefully, needs testing...any volunteers?)
- August 16, 2007
    Will now downsize the image if necessary to fit GL_MAX_TEXTURE_SIZE
    added CreateOglTexture() to upload raw image data that isn't from an image file
- August 14, 2007 (PM) - Can now load indexed TGA
- August 14, 2007 (AM)
    Updated to stb_image 0.97
    added result messages
    can now decompress DDS files (DXT1/2/3/4/5)
- August 11, 2007 - MIPmaps can now handle non-square textures
- August 7, 2007
    Can directly upload DXT1/3/5 DDS files (with or w/o MIPmaps)
    can compress any image to DXT1/5 (using a new & fast & simple compression scheme) and upload
    can save as DDS
- July 31, 2007 - added compressing to DXT and flipping about Y
- July 30, 2007 - initial release
