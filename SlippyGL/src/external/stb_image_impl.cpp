// Single translation unit that compiles the stb_image implementation.
// Keep these defines here (not in PngCodec.cpp) so there is exactly one
// implementation in the program.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO   // memory-only (no file I/O)
#define STBI_ONLY_PNG   // PNG only
#include <stb_image.h>
