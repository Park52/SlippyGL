#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace slippygl::render
{
    /**
     * TTF 기반 텍스트 오버레이 렌더러.
     *
     * stb_truetype로 폰트 글리프 아틀라스를 한 번 구워(R8 텍스처) 화면 공간(top-left
     * origin, y-down)에 텍스트와 단색 사각형을 그린다. ASCII(32~126) + © (U+00A9)를
     * 지원하므로 OSM 저작자 표시(`© OpenStreetMap contributors`)를 진짜 © 글자로 렌더한다.
     *
     * 추후 Step 8 디버그 오버레이(타일 z/x/y 텍스트)에서도 재사용한다.
     */
    class TextRenderer
    {
    public:
        TextRenderer() = default;
        ~TextRenderer();

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;

        /**
         * 폰트를 로드해 글리프 아틀라스를 굽고 GL 리소스를 준비한다.
         * @param fontPath  TTF 경로. 빈 문자열이면 시스템 폰트를 자동 탐색한다.
         * @param pixelHeight  글리프 픽셀 높이(ascender~descender)
         * @return 성공 시 true
         */
        bool init(const std::string& fontPath = "", float pixelHeight = 16.0f);

        void shutdown();

        bool ready() const { return program_ != 0 && atlasTex_ != 0; }

        /**
         * 텍스트를 그린다. (x, y)는 첫 글자의 베이스라인 펜 위치(화면 픽셀, y-down).
         */
        void drawText(const std::string& utf8, float x, float y,
                      const glm::vec4& color, int fbW, int fbH);

        /** 단색 사각형(반투명 배경 바 등)을 그린다. */
        void drawRect(float x, float y, float w, float h,
                      const glm::vec4& color, int fbW, int fbH);

        /** 사각형 외곽선(두께 thickness px)을 그린다. 타일 경계선 등. */
        void drawRectOutline(float x, float y, float w, float h, float thickness,
                             const glm::vec4& color, int fbW, int fbH);

        /**
         * (tlx, tly)를 텍스트 박스의 top-left로 삼아, 반투명 배경(bgColor.a>0일 때)과
         * 텍스트를 함께 그린다. 베이스라인 보정은 내부에서 처리한다.
         */
        void drawTextBoxed(const std::string& utf8, float tlx, float tly,
                           const glm::vec4& textColor, const glm::vec4& bgColor,
                           float pad, int fbW, int fbH);

        /** 텍스트 픽셀 크기(폭/높이)를 측정한다. 미초기화면 false. */
        bool measure(const std::string& utf8, float& outW, float& outH) const;

        /**
         * `© OpenStreetMap contributors`를 화면 우하단에 반투명 배경과 함께 상시 표시.
         * (OSM 타일 정책 필수: 저작자 표시)
         */
        void drawAttribution(int fbW, int fbH);

    private:
        // 한 정점: 화면 픽셀 좌표(x,y) + 아틀라스 UV(u,v)
        struct Vert { float x, y, u, v; };

        // UTF-8 → 코드포인트 디코드 (1~4바이트)
        static std::vector<std::uint32_t> decodeUtf8(const std::string& s);
        // 코드포인트 → 아틀라스 글리프 인덱스. 미지원이면 -1
        int glyphIndex(std::uint32_t cp) const;

        // 코드포인트열을 (originX, originBaselineY)에서 시작해 삼각형 정점으로 펼친다.
        // bbox(minX,minY,maxX,maxY)를 갱신하고, 펜 진행 후 너비를 반환한다.
        float buildQuads(const std::vector<std::uint32_t>& cps,
                         float originX, float originBaselineY,
                         std::vector<Vert>& out,
                         glm::vec4& bbox) const;

        void uploadAndDraw(const std::vector<Vert>& verts,
                           const glm::vec4& color, bool solid,
                           int fbW, int fbH);

        bool compileShaders();
        static std::string findDefaultFontPath();

        unsigned int vao_ = 0;
        unsigned int vbo_ = 0;
        unsigned int program_ = 0;
        unsigned int atlasTex_ = 0;

        int uProjLoc_ = -1;
        int uTexLoc_ = -1;
        int uColorLoc_ = -1;
        int uSolidLoc_ = -1;

        int atlasW_ = 0;
        int atlasH_ = 0;
        float pixelHeight_ = 16.0f;

        // 베이크된 글리프 메타데이터(stbtt_packedchar). 불투명 포인터로 보관해
        // 헤더에 stb_truetype 의존을 노출하지 않는다.
        void* packed_ = nullptr;  // stbtt_packedchar[kGlyphCount]
    };
}
