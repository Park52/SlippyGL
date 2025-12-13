#include "QuadRenderer.hpp"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <array>

namespace slippygl::render
{

// Vertex shader: converts pixel coordinates to clip coordinates using orthographic projection
static const char* kVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uProj;

void main()
{
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

// Fragment shader: texture sampling
static const char* kFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTex;

void main()
{
    FragColor = texture(uTex, vTexCoord);
}
)";

QuadRenderer::~QuadRenderer()
{
    shutdown();
}

bool QuadRenderer::init()
{
    // Create VAO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Dynamic buffer (6 vertices * 4 floats = 24 floats)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute (location 0): vec2
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // Texture coordinate attribute (location 1): vec2
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile and link shaders
    if (!compileShaders()) {
        shutdown();
        return false;
    }

    spdlog::info("QuadRenderer initialized");
    return true;
}

void QuadRenderer::shutdown()
{
    if (program_) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

bool QuadRenderer::compileShaders()
{
    // Compile vertex shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVertexShader, nullptr);
    glCompileShader(vs);

    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
        spdlog::error("Vertex shader compile error: {}", log);
        glDeleteShader(vs);
        return false;
    }

    // Compile fragment shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &kFragmentShader, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
        spdlog::error("Fragment shader compile error: {}", log);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }

    // Link program
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        spdlog::error("Shader program link error: {}", log);
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    // Shader objects can be deleted after linking
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Cache uniform locations
    uProjLoc_ = glGetUniformLocation(program_, "uProj");
    uTexLoc_ = glGetUniformLocation(program_, "uTex");

    return true;
}

void QuadRenderer::draw(TexHandle tex, const Quad& q, int texFullW, int texFullH, int fbW, int fbH)
{
    if (!program_ || !vao_ || tex == 0) return;
    if (texFullW <= 0 || texFullH <= 0 || fbW <= 0 || fbH <= 0) return;

    // Orthographic projection matrix (top-left origin, Y-axis down)
    // Clip coordinates: x: [0, fbW] -> [-1, 1], y: [0, fbH] -> [1, -1]
    float proj[16] = {0};
    proj[0] = 2.0f / static_cast<float>(fbW);       // scale x
    proj[5] = -2.0f / static_cast<float>(fbH);      // scale y (y-down)
    proj[10] = -1.0f;                                // scale z
    proj[12] = -1.0f;                                // translate x
    proj[13] = 1.0f;                                 // translate y
    proj[15] = 1.0f;

    // Generate quad vertices (pixel coordinates)
    const float x0 = static_cast<float>(q.x);
    const float y0 = static_cast<float>(q.y);
    const float x1 = static_cast<float>(q.x + q.w);
    const float y1 = static_cast<float>(q.y + q.h);

    // Texture coordinates (normalized to 0..1 range)
    const float tw = static_cast<float>(texFullW);
    const float th = static_cast<float>(texFullH);
    const float u0 = static_cast<float>(q.sx) / tw;
    const float v0 = static_cast<float>(q.sy) / th;
    const float u1 = static_cast<float>(q.sx + q.sw) / tw;
    const float v1 = static_cast<float>(q.sy + q.sh) / th;

    // 6 vertices (2 triangles)
    // Each vertex: x, y, u, v
    std::array<float, 24> vertices = {
        // Triangle 1
        x0, y0, u0, v0,  // top-left
        x1, y0, u1, v0,  // top-right
        x0, y1, u0, v1,  // bottom-left
        // Triangle 2
        x1, y0, u1, v0,  // top-right
        x1, y1, u1, v1,  // bottom-right
        x0, y1, u0, v1,  // bottom-left
    };

    // Update buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());

    // Render
    glUseProgram(program_);
    glUniformMatrix4fv(uProjLoc_, 1, GL_FALSE, proj);
    glUniform1i(uTexLoc_, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Enable alpha blending (PNG transparency support)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

} // namespace slippygl::render
