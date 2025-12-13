#include "TextureManager.hpp"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace slippygl::render
{

TextureManager::~TextureManager()
{
    destroyAll();
}

TexHandle TextureManager::createRGBA8(int w, int h, const std::uint8_t* pixels)
{
    if (w <= 0 || h <= 0 || !pixels) {
        spdlog::error("TextureManager: invalid parameters (w={}, h={}, pixels={})", 
                      w, h, pixels ? "valid" : "null");
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0) {
        spdlog::error("TextureManager: glGenTextures failed");
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, tex);

    // Filtering: NEAREST for sharp tile map rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Wrapping: CLAMP_TO_EDGE to prevent edge blur
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        spdlog::error("TextureManager: OpenGL error {}", err);
        glDeleteTextures(1, &tex);
        return 0;
    }

    textures_.insert(tex);
    spdlog::debug("TextureManager: created texture {} ({}x{})", tex, w, h);

    return tex;
}

void TextureManager::destroy(TexHandle tex)
{
    if (tex == 0) return;

    auto it = textures_.find(tex);
    if (it != textures_.end()) {
        glDeleteTextures(1, &tex);
        textures_.erase(it);
        spdlog::debug("TextureManager: destroyed texture {}", tex);
    }
}

void TextureManager::destroyAll()
{
    for (TexHandle tex : textures_) {
        glDeleteTextures(1, &tex);
    }
    spdlog::debug("TextureManager: destroyed {} textures", textures_.size());
    textures_.clear();
}

} // namespace slippygl::render
