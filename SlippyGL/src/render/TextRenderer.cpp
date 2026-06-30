#include "TextRenderer.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <filesystem>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace slippygl::render
{
namespace
{
    // 지원 글리프: ASCII 32..126 (95자) + © (U+00A9). 총 96자.
    constexpr int kAsciiFirst = 32;
    constexpr int kAsciiLast  = 126;
    constexpr int kAsciiCount = kAsciiLast - kAsciiFirst + 1; // 95
    constexpr std::uint32_t kCopyright = 0x00A9u;
    constexpr int kGlyphCount = kAsciiCount + 1;              // 96
    constexpr int kCopyrightIndex = kAsciiCount;              // 95
    constexpr int kAtlasSize = 512;

    const char* kVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
out vec2 vUV;
uniform mat4 uProj;
void main() {
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

    const char* kFragmentShader = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform vec4 uColor;
uniform int uSolid;   // 1이면 텍스처 무시(단색), 0이면 글리프 커버리지 사용
void main() {
    float a = (uSolid == 1) ? 1.0 : texture(uTex, vUV).r;
    FragColor = vec4(uColor.rgb, uColor.a * a);
}
)";

    GLuint compileShader(GLenum type, const char* src)
    {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
            spdlog::error("TextRenderer shader compile error: {}", log);
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    }
}

TextRenderer::~TextRenderer()
{
    shutdown();
}

std::string TextRenderer::findDefaultFontPath()
{
    namespace fs = std::filesystem;
    const char* candidates[] = {
        // Windows
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        // macOS
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Geneva.ttf",
        // Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
    std::error_code ec;
    for (const char* p : candidates) {
        if (fs::exists(p, ec)) {
            return p;
        }
    }
    return {};
}

bool TextRenderer::init(const std::string& fontPath, float pixelHeight)
{
    pixelHeight_ = pixelHeight;

    const std::string path = fontPath.empty() ? findDefaultFontPath() : fontPath;
    if (path.empty()) {
        spdlog::error("TextRenderer: no usable TTF font found (attribution overlay disabled)");
        return false;
    }

    // 폰트 파일 로드
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        spdlog::error("TextRenderer: failed to open font: {}", path);
        return false;
    }
    std::vector<unsigned char> fontBytes(
        (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (fontBytes.empty()) {
        spdlog::error("TextRenderer: empty font file: {}", path);
        return false;
    }

    // 코드포인트 목록: ASCII 32..126 + ©
    std::array<int, kGlyphCount> codepoints{};
    for (int i = 0; i < kAsciiCount; ++i) {
        codepoints[i] = kAsciiFirst + i;
    }
    codepoints[kCopyrightIndex] = static_cast<int>(kCopyright);

    // 글리프 아틀라스(R8) 굽기
    std::vector<unsigned char> bitmap(static_cast<size_t>(kAtlasSize) * kAtlasSize, 0);
    auto* packed = new stbtt_packedchar[kGlyphCount];

    stbtt_pack_context pc;
    if (!stbtt_PackBegin(&pc, bitmap.data(), kAtlasSize, kAtlasSize, 0, 1, nullptr)) {
        spdlog::error("TextRenderer: stbtt_PackBegin failed");
        delete[] packed;
        return false;
    }
    stbtt_PackSetOversampling(&pc, 1, 1);

    stbtt_pack_range range{};
    range.font_size = pixelHeight_;
    range.first_unicode_codepoint_in_range = 0;
    range.array_of_unicode_codepoints = codepoints.data();
    range.num_chars = kGlyphCount;
    range.chardata_for_range = packed;

    const int packOk = stbtt_PackFontRanges(&pc, fontBytes.data(), 0, &range, 1);
    stbtt_PackEnd(&pc);
    if (!packOk) {
        spdlog::warn("TextRenderer: some glyphs failed to pack (atlas may be too small)");
    }

    packed_ = packed;
    atlasW_ = kAtlasSize;
    atlasH_ = kAtlasSize;

    // GL 텍스처 업로드 (단일 채널)
    glGenTextures(1, &atlasTex_);
    glBindTexture(GL_TEXTURE_2D, atlasTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAtlasSize, kAtlasSize, 0,
                 GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO/VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    if (!compileShaders()) {
        shutdown();
        return false;
    }

    spdlog::info("TextRenderer initialized (font: {}, {}px)", path, pixelHeight_);
    return true;
}

bool TextRenderer::compileShaders()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        spdlog::error("TextRenderer program link error: {}", log);
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    uProjLoc_  = glGetUniformLocation(program_, "uProj");
    uTexLoc_   = glGetUniformLocation(program_, "uTex");
    uColorLoc_ = glGetUniformLocation(program_, "uColor");
    uSolidLoc_ = glGetUniformLocation(program_, "uSolid");
    return true;
}

void TextRenderer::shutdown()
{
    if (program_) { glDeleteProgram(program_); program_ = 0; }
    if (vbo_)     { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_)     { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (atlasTex_){ glDeleteTextures(1, &atlasTex_); atlasTex_ = 0; }
    if (packed_)  { delete[] static_cast<stbtt_packedchar*>(packed_); packed_ = nullptr; }
}

std::vector<std::uint32_t> TextRenderer::decodeUtf8(const std::string& s)
{
    std::vector<std::uint32_t> out;
    const auto* p = reinterpret_cast<const unsigned char*>(s.data());
    const auto* end = p + s.size();
    while (p < end) {
        std::uint32_t cp = 0;
        const unsigned char c = *p;
        int extra = 0;
        if (c < 0x80)        { cp = c;            extra = 0; }
        else if ((c >> 5) == 0x6)  { cp = c & 0x1F; extra = 1; }
        else if ((c >> 4) == 0xE)  { cp = c & 0x0F; extra = 2; }
        else if ((c >> 3) == 0x1E) { cp = c & 0x07; extra = 3; }
        else { ++p; continue; } // 잘못된 선행 바이트는 건너뜀
        ++p;
        for (int i = 0; i < extra && p < end && (*p & 0xC0) == 0x80; ++i, ++p) {
            cp = (cp << 6) | (*p & 0x3F);
        }
        out.push_back(cp);
    }
    return out;
}

int TextRenderer::glyphIndex(std::uint32_t cp) const
{
    if (cp >= static_cast<std::uint32_t>(kAsciiFirst) &&
        cp <= static_cast<std::uint32_t>(kAsciiLast)) {
        return static_cast<int>(cp) - kAsciiFirst;
    }
    if (cp == kCopyright) {
        return kCopyrightIndex;
    }
    return -1;
}

float TextRenderer::buildQuads(const std::vector<std::uint32_t>& cps,
                               float originX, float originBaselineY,
                               std::vector<Vert>& out,
                               glm::vec4& bbox) const
{
    auto* packed = static_cast<stbtt_packedchar*>(packed_);
    float xpos = originX;
    float ypos = originBaselineY;
    bool any = false;
    float minX = 0, minY = 0, maxX = 0, maxY = 0;

    for (std::uint32_t cp : cps) {
        const int gi = glyphIndex(cp);
        if (gi < 0) {
            continue; // 미지원 글리프는 건너뜀
        }
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(packed, atlasW_, atlasH_, gi, &xpos, &ypos, &q, 1);

        // 2 삼각형
        out.push_back({ q.x0, q.y0, q.s0, q.t0 });
        out.push_back({ q.x1, q.y0, q.s1, q.t0 });
        out.push_back({ q.x0, q.y1, q.s0, q.t1 });
        out.push_back({ q.x1, q.y0, q.s1, q.t0 });
        out.push_back({ q.x1, q.y1, q.s1, q.t1 });
        out.push_back({ q.x0, q.y1, q.s0, q.t1 });

        if (!any) { minX = q.x0; minY = q.y0; maxX = q.x1; maxY = q.y1; any = true; }
        else {
            minX = std::min(minX, q.x0); minY = std::min(minY, q.y0);
            maxX = std::max(maxX, q.x1); maxY = std::max(maxY, q.y1);
        }
    }

    bbox = glm::vec4(minX, minY, maxX, maxY);
    return xpos - originX;
}

void TextRenderer::uploadAndDraw(const std::vector<Vert>& verts,
                                 const glm::vec4& color, bool solid,
                                 int fbW, int fbH)
{
    if (verts.empty() || program_ == 0 || fbW <= 0 || fbH <= 0) {
        return;
    }

    // 화면 공간 ortho (top-left origin, y-down) — QuadRenderer와 동일 규약
    glm::mat4 proj(0.0f);
    proj[0][0] =  2.0f / static_cast<float>(fbW);
    proj[1][1] = -2.0f / static_cast<float>(fbH);
    proj[2][2] = -1.0f;
    proj[3][0] = -1.0f;
    proj[3][1] =  1.0f;
    proj[3][3] =  1.0f;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vert)),
                 verts.data(), GL_DYNAMIC_DRAW);

    glUseProgram(program_);
    glUniformMatrix4fv(uProjLoc_, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform4fv(uColorLoc_, 1, glm::value_ptr(color));
    glUniform1i(uSolidLoc_, solid ? 1 : 0);
    glUniform1i(uTexLoc_, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTex_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextRenderer::drawText(const std::string& utf8, float x, float y,
                            const glm::vec4& color, int fbW, int fbH)
{
    if (!ready()) return;
    const auto cps = decodeUtf8(utf8);
    std::vector<Vert> verts;
    glm::vec4 bbox(0.0f);
    buildQuads(cps, x, y, verts, bbox);
    uploadAndDraw(verts, color, /*solid*/false, fbW, fbH);
}

void TextRenderer::drawRect(float x, float y, float w, float h,
                            const glm::vec4& color, int fbW, int fbH)
{
    if (!ready()) return;
    const float x0 = x, y0 = y, x1 = x + w, y1 = y + h;
    std::vector<Vert> verts = {
        { x0, y0, 0, 0 }, { x1, y0, 0, 0 }, { x0, y1, 0, 0 },
        { x1, y0, 0, 0 }, { x1, y1, 0, 0 }, { x0, y1, 0, 0 },
    };
    uploadAndDraw(verts, color, /*solid*/true, fbW, fbH);
}

void TextRenderer::drawAttribution(int fbW, int fbH)
{
    if (!ready() || fbW <= 0 || fbH <= 0) return;

    static const std::string kText = "\xC2\xA9 OpenStreetMap contributors"; // "© ..."
    const auto cps = decodeUtf8(kText);

    // 1) 측정: 베이스라인 (0,0) 기준 bbox 계산
    std::vector<Vert> probe;
    glm::vec4 bbox(0.0f);
    buildQuads(cps, 0.0f, 0.0f, probe, bbox);
    const float textW = bbox.z - bbox.x;
    const float textH = bbox.w - bbox.y;

    // 2) 우하단 배치 좌표 계산
    constexpr float margin = 10.0f;
    constexpr float pad = 5.0f;
    const float boxX = static_cast<float>(fbW) - textW - margin - 2.0f * pad;
    const float boxY = static_cast<float>(fbH) - textH - margin - 2.0f * pad;

    // 텍스트 bbox의 top-left이 (boxX+pad, boxY+pad)에 오도록 베이스라인 원점 보정
    const float originX = (boxX + pad) - bbox.x;
    const float originY = (boxY + pad) - bbox.y;

    // 3) 반투명 배경 바
    drawRect(boxX, boxY, textW + 2.0f * pad, textH + 2.0f * pad,
             glm::vec4(0.0f, 0.0f, 0.0f, 0.5f), fbW, fbH);

    // 4) 흰색 텍스트
    std::vector<Vert> verts;
    glm::vec4 ignore(0.0f);
    buildQuads(cps, originX, originY, verts, ignore);
    uploadAndDraw(verts, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), /*solid*/false, fbW, fbH);
}

} // namespace slippygl::render
