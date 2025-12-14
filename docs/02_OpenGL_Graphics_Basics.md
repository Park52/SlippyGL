# 🎮 OpenGL 그래픽스 기초

> **학습 목표**: SlippyGL에서 사용하는 OpenGL 개념 완전 정복
> 
> **예상 학습 시간**: 총 2시간
> 
> **사전 지식**: 01_TileSystem_DeepDive.md 완료

---

## 📚 목차

1. [OpenGL 파이프라인 개요](#1-opengl-파이프라인-개요)
2. [버텍스와 프래그먼트 셰이더](#2-버텍스와-프래그먼트-셰이더)
3. [VAO, VBO, EBO 이해하기](#3-vao-vbo-ebo-이해하기)
4. [텍스처와 샘플링](#4-텍스처와-샘플링)
5. [QuadRenderer 완전 해부](#5-quadrenderer-완전-해부)
6. [연습 문제](#6-연습-문제)

---

# 1. OpenGL 파이프라인 개요

## 📖 읽기 시간: 10분

### 1.1 그래픽스 파이프라인이란?

**정의**: 3D/2D 데이터를 화면 픽셀로 변환하는 단계별 처리 과정

```
정점 데이터 → 정점 셰이더 → 래스터화 → 프래그먼트 셰이더 → 프레임버퍼
   (CPU)        (GPU)          (GPU)         (GPU)           (화면)
```

### 1.2 각 단계 설명

| 단계 | 입력 | 출력 | 역할 |
|-----|-----|-----|------|
| 정점 셰이더 | 정점 좌표 | 클립 좌표 | 좌표 변환 (MVP) |
| 래스터화 | 삼각형 | 프래그먼트 | 삼각형 → 픽셀 후보 생성 |
| 프래그먼트 셰이더 | 프래그먼트 | 색상 | 각 픽셀 색상 결정 |
| 프레임버퍼 | 색상 | 화면 | 최종 이미지 저장 |

### 1.3 SlippyGL에서의 흐름

```
타일 쿼드 정점 (4개)
    ↓
버텍스 셰이더: 월드 → 스크린 좌표 변환 (MVP 행렬)
    ↓
래스터화: 쿼드 → 65,536 프래그먼트 (256×256 픽셀)
    ↓
프래그먼트 셰이더: 텍스처 샘플링 → 픽셀 색상
    ↓
프레임버퍼 → 화면 표시
```

### 1.4 GPU vs CPU

```
CPU: 순차 처리, 복잡한 로직
     1 task × 빠른 속도

GPU: 병렬 처리, 단순한 로직
     1000+ tasks × 느린 속도 (개별)
     
총 처리량: GPU >> CPU (그래픽스에서)
```

**예시: 256×256 픽셀 색칠**
```
CPU: 65,536회 루프 = 65,536 사이클
GPU: 65,536개 코어가 동시 처리 = ~1 사이클
```

### 💡 핵심 인사이트

> **GPU는 "같은 작업을 수천 번"에 최적화되어 있습니다.**
> 
> 그래서 셰이더(동일 코드)가 각 정점/픽셀에 병렬 적용됩니다.

---

# 2. 버텍스와 프래그먼트 셰이더

## 📖 읽기 시간: 15분

### 2.1 GLSL 소개

**GLSL** (OpenGL Shading Language): GPU에서 실행되는 C 스타일 언어

```glsl
#version 330 core  // OpenGL 3.3

// 버텍스 셰이더
in vec2 aPos;      // 입력: 정점 좌표
out vec2 vTexCoord; // 출력: 다음 단계로 전달
uniform mat4 uMVP; // 유니폼: 모든 정점에 공통

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
}
```

### 2.2 변수 타입

| 키워드 | 의미 | 예시 |
|-------|-----|------|
| `in` | 입력 (이전 단계에서) | `in vec2 aPos;` |
| `out` | 출력 (다음 단계로) | `out vec4 color;` |
| `uniform` | 상수 (CPU에서 설정) | `uniform mat4 uMVP;` |

### 2.3 데이터 타입

```glsl
float   // 32비트 부동소수점
vec2    // (x, y)
vec3    // (x, y, z) 또는 (r, g, b)
vec4    // (x, y, z, w) 또는 (r, g, b, a)
mat4    // 4×4 행렬

// 스위즐링 (Swizzling)
vec4 v = vec4(1, 2, 3, 4);
vec2 xy = v.xy;      // (1, 2)
vec3 rgb = v.rgb;    // (1, 2, 3)
vec4 bgra = v.bgra;  // (3, 2, 1, 4) - 순서 변경!
```

### 2.4 QuadRenderer 버텍스 셰이더

```glsl
#version 330 core

// 입력: CPU에서 전달받은 정점 데이터
layout(location = 0) in vec2 aPos;       // 정점 위치
layout(location = 1) in vec2 aTexCoord;  // 텍스처 좌표

// 출력: 프래그먼트 셰이더로 전달
out vec2 vTexCoord;

// 유니폼: 변환 행렬
uniform mat4 uMVP;

void main() {
    // 월드 좌표 → 클립 좌표
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    
    // 텍스처 좌표는 그대로 전달
    vTexCoord = aTexCoord;
}
```

**`gl_Position` 설명:**
- 내장 출력 변수 (반드시 설정해야 함)
- vec4: (x, y, z, w) 클립 좌표
- w=1.0은 점(point)을 의미

**`layout(location = 0)` 의미:**
- VAO에서 어떤 속성과 연결되는지 지정
- location 0 = 위치, location 1 = 텍스처 좌표

### 2.5 프래그먼트 셰이더

```glsl
#version 330 core

// 입력: 버텍스 셰이더에서 보간된 값
in vec2 vTexCoord;

// 출력: 픽셀 색상
out vec4 FragColor;

// 유니폼: 텍스처
uniform sampler2D uTexture;

void main() {
    // 텍스처에서 색상 샘플링
    FragColor = texture(uTexture, vTexCoord);
}
```

**`texture()` 함수:**
- 입력: sampler2D, UV 좌표
- 출력: vec4 색상 (RGBA)
- UV 좌표 (0,0)~(1,1)을 텍스처 픽셀로 변환

### 2.6 보간(Interpolation)

**질문**: 정점 4개에만 텍스처 좌표를 지정했는데, 65,536 픽셀의 좌표는 어떻게?

**답**: 래스터화 단계에서 자동 보간!

```
정점 A (0,0)         정점 B (1,0)
     ●────────────────●
     │                │
     │    * (0.5,0.5) │  ← 자동 계산됨
     │                │
     ●────────────────●
정점 C (0,1)         정점 D (1,1)
```

**보간 공식 (2D 선형 보간):**
```
P(u,v) = (1-u)(1-v)A + u(1-v)B + (1-u)v*C + uv*D

예: u=0.5, v=0.5
P = 0.25*A + 0.25*B + 0.25*C + 0.25*D
  = (0.25*0 + 0.25*1 + 0.25*0 + 0.25*1, ...)
  = (0.5, 0.5)
```

---

# 3. VAO, VBO, EBO 이해하기

## 📖 읽기 시간: 15분

### 3.1 용어 정의

| 약어 | 풀네임 | 역할 |
|-----|--------|------|
| VAO | Vertex Array Object | 정점 속성 설정 저장 |
| VBO | Vertex Buffer Object | 정점 데이터 저장 |
| EBO | Element Buffer Object | 인덱스 데이터 저장 |

### 3.2 왜 EBO가 필요한가?

**사각형을 그리려면 삼각형 2개 필요:**

```
방법 1: EBO 없이 (6개 정점)
삼각형 1: A, B, C    삼각형 2: B, D, C
    A ───── B            A ───── B
    │    ╱               │    ╲ │
    │  ╱                 │      ╲│
    C                    C ───── D

정점 배열: [A, B, C, B, D, C]  ← B, C 중복!
메모리: 6 × 정점크기 = 6 × 16 = 96 bytes
```

```
방법 2: EBO 사용 (4개 정점 + 6개 인덱스)
정점 배열: [A, B, C, D]
인덱스 배열: [0, 1, 2, 1, 3, 2]

    0(A) ─── 1(B)
     │ ╲   ╱ │
     │   ╳   │
     │ ╱   ╲ │
    2(C) ─── 3(D)

메모리: 4 × 16 + 6 × 4 = 64 + 24 = 88 bytes
```

**복잡한 모델에서 효과:**
- 100,000 삼각형 모델
- 평균 정점 공유율 6:1 → 메모리 80% 절약!

### 3.3 VAO의 역할

```cpp
// VAO 없이 매 프레임마다:
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
// ... 반복 ...

// VAO 사용 시:
// 초기화에서 한 번만 설정
glBindVertexArray(vao);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glVertexAttribPointer(...);

// 렌더링에서는 바인딩만:
glBindVertexArray(vao);  // 모든 설정이 복원됨!
glDrawElements(...);
```

**VAO는 "설정 스냅샷"**

### 3.4 정점 레이아웃

```cpp
struct Vertex {
    float x, y;     // 위치 (8 bytes)
    float u, v;     // 텍스처 좌표 (8 bytes)
};  // 총 16 bytes (stride)
```

**메모리 레이아웃:**
```
Offset:  0    4    8   12   16   20   24   28
       ┌────┬────┬────┬────┬────┬────┬────┬────┐
Vertex0│ x0 │ y0 │ u0 │ v0 │ x1 │ y1 │ u1 │ v1 │ ...
       └────┴────┴────┴────┴────┴────┴────┴────┘
         ↑ 위치 (loc 0)  ↑ 텍스처 좌표 (loc 1)
```

### 3.5 glVertexAttribPointer 파라미터

```cpp
glVertexAttribPointer(
    0,                  // location = 0 (셰이더의 layout)
    2,                  // 컴포넌트 수 (vec2 = 2)
    GL_FLOAT,           // 데이터 타입
    GL_FALSE,           // 정규화 여부
    16,                 // stride (정점 간 간격)
    (void*)0            // offset (속성 시작 위치)
);

glVertexAttribPointer(
    1,                  // location = 1
    2,                  // vec2
    GL_FLOAT,
    GL_FALSE,
    16,                 // 같은 stride
    (void*)8            // offset: 8 bytes (x,y 다음)
);
```

---

# 4. 텍스처와 샘플링

## 📖 읽기 시간: 15분

### 4.1 텍스처 생성

```cpp
GLuint tex;
glGenTextures(1, &tex);           // ID 생성
glBindTexture(GL_TEXTURE_2D, tex); // 바인딩

// 픽셀 데이터 업로드
glTexImage2D(
    GL_TEXTURE_2D,      // 타겟
    0,                  // 밉맵 레벨
    GL_RGBA8,           // 내부 포맷 (GPU 저장 방식)
    256, 256,           // 크기
    0,                  // 보더 (항상 0)
    GL_RGBA,            // 입력 포맷
    GL_UNSIGNED_BYTE,   // 입력 데이터 타입
    pixels              // 픽셀 데이터 포인터
);
```

### 4.2 샘플링 파라미터

#### 4.2.1 필터링 (Filtering)

```cpp
// 확대 필터 (Magnification)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
// GL_NEAREST: 가장 가까운 픽셀 (박스형, 픽셀 아트용)
// GL_LINEAR: 주변 4픽셀 보간 (부드러움)

// 축소 필터 (Minification)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
// GL_NEAREST_MIPMAP_LINEAR: 밉맵 사용
```

**시각적 비교:**
```
원본:          GL_NEAREST:     GL_LINEAR:
██              ████████        ░░██████░░
                ████████        ░░██████░░
                ████████        ██████████
                ████████        ██████████
                ████████        ██████████
                ████████        ░░██████░░
                                ░░██████░░

(픽셀 아트는 NEAREST, 사진은 LINEAR)
```

#### 4.2.2 랩 모드 (Wrapping)

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

**랩 모드 종류:**
```
GL_REPEAT:         GL_CLAMP_TO_EDGE:
┌───┬───┬───┐      ┌───┬───┬───┐
│ A │ A │ A │      │ A │ A │ A │
├───┼───┼───┤      ├───┼───┼───┤
│ A │ A │ A │      │ A │ A │ A │  ← 경계 색상 반복
└───┴───┴───┘      └───┴───┴───┘
  ↑ 타일링           ↑ 늘리기
```

**SlippyGL에서 CLAMP_TO_EDGE 사용 이유:**
- 타일 경계에서 다른 타일 색상이 번지는 것 방지
- 각 타일이 독립적으로 렌더링됨

### 4.3 텍스처 좌표 (UV)

```
(0,0) ─────────── (1,0)
  │                 │
  │    텍스처       │
  │                 │
(0,1) ─────────── (1,1)
```

**주의**: OpenGL의 텍스처 좌표는 Y-Up!
- (0,0) = 좌하단
- 이미지 로더가 Y-Down으로 주면 뒤집어야 함

### 4.4 텍스처 유닛 (Texture Unit)

```cpp
// 여러 텍스처를 동시에 사용할 때
glActiveTexture(GL_TEXTURE0);  // 유닛 0 활성화
glBindTexture(GL_TEXTURE_2D, tex1);

glActiveTexture(GL_TEXTURE1);  // 유닛 1 활성화
glBindTexture(GL_TEXTURE_2D, tex2);

// 셰이더에서
uniform sampler2D texture0;  // GL_TEXTURE0과 연결
uniform sampler2D texture1;  // GL_TEXTURE1과 연결
```

**SlippyGL**: 타일 1개씩 그리므로 유닛 0만 사용

---

# 5. QuadRenderer 완전 해부

## 📖 읽기 시간: 15분

### 5.1 클래스 구조

```cpp
class QuadRenderer {
    GLuint vao_, vbo_, ebo_;  // OpenGL 버퍼
    GLuint program_;          // 셰이더 프로그램
    
    // 유니폼 위치 (조회 비용 절감)
    GLint locMVP_;
    GLint locTexture_;
    GLint locTexSize_;
};
```

### 5.2 init() - 초기화

```cpp
bool QuadRenderer::init() {
    // 1) 셰이더 컴파일 및 링크
    program_ = createShaderProgram(vertexSrc, fragmentSrc);
    
    // 2) 유니폼 위치 조회
    locMVP_ = glGetUniformLocation(program_, "uMVP");
    locTexture_ = glGetUniformLocation(program_, "uTexture");
    locTexSize_ = glGetUniformLocation(program_, "uTexSize");
    
    // 3) VAO/VBO/EBO 생성
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    
    // 4) 정점 레이아웃 설정
    glBindVertexArray(vao_);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, nullptr, GL_DYNAMIC_DRAW);
    
    // 위치 속성 (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // 텍스처 좌표 속성 (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)8);
    
    // 인덱스 버퍼
    unsigned short indices[] = {0, 1, 2, 2, 3, 0};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    return true;
}
```

**`GL_DYNAMIC_DRAW` vs `GL_STATIC_DRAW`:**
- VBO: DYNAMIC (매 프레임 정점 데이터 변경)
- EBO: STATIC (인덱스는 항상 같음: 0,1,2,2,3,0)

### 5.3 draw() - 쿼드 렌더링

```cpp
void QuadRenderer::draw(GLuint tex, const Quad& q, int texW, int texH, const glm::mat4& mvp) {
    // 1) 정점 데이터 생성 (정수 스냅)
    const float x0 = std::floor(static_cast<float>(q.x));
    const float y0 = std::floor(static_cast<float>(q.y));
    const float x1 = x0 + q.w;
    const float y1 = y0 + q.h;
    
    // 텍스처 좌표 (0~1 정규화)
    const float u0 = static_cast<float>(q.sx) / texW;
    const float v0 = static_cast<float>(q.sy) / texH;
    const float u1 = static_cast<float>(q.sx + q.sw) / texW;
    const float v1 = static_cast<float>(q.sy + q.sh) / texH;
    
    Vertex vertices[4] = {
        {x0, y0, u0, v0},  // 좌상단
        {x1, y0, u1, v0},  // 우상단
        {x1, y1, u1, v1},  // 우하단
        {x0, y1, u0, v1},  // 좌하단
    };
    
    // 2) VBO 업데이트
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    // 3) 셰이더 활성화 및 유니폼 설정
    glUseProgram(program_);
    glUniformMatrix4fv(locMVP_, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform2f(locTexSize_, texW, texH);
    
    // 4) 텍스처 바인딩
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(locTexture_, 0);  // 텍스처 유닛 0
    
    // 5) 그리기
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}
```

### 5.4 Quad 구조체

```cpp
struct Quad {
    int x, y;    // 화면 좌표 (좌상단)
    int w, h;    // 크기
    int sx, sy;  // 텍스처 소스 좌표
    int sw, sh;  // 텍스처 소스 크기
};
```

**전체 타일 렌더링 예:**
```cpp
Quad q;
q.x = 768;      // 월드 X (타일 3 × 256)
q.y = 256;      // 월드 Y (타일 1 × 256)
q.w = 256;      // 타일 너비
q.h = 256;      // 타일 높이
q.sx = 0;       // 텍스처 전체 사용
q.sy = 0;
q.sw = 256;
q.sh = 256;
```

### 5.5 정점 데이터 시각화

```
(x0,y0)         (x1,y0)
   0 ─────────── 1
   │ ╲           │
   │   ╲         │
   │     ╲       │      인덱스: 0,1,2, 2,3,0
   │       ╲     │      → 삼각형 (0,1,2), (2,3,0)
   │         ╲   │
   3 ─────────── 2
(x0,y1)         (x1,y1)
```

### 5.6 shutdown() - 정리

```cpp
void QuadRenderer::shutdown() {
    if (program_) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (ebo_) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}
```

**순서 중요!**
1. 프로그램 삭제
2. 버퍼 삭제
3. VAO 삭제 (VAO가 버퍼 참조하므로 마지막)

---

# 6. 연습 문제

## 📖 풀이 시간: 20분

### 문제 1: 셰이더 출력 (기초)

**Q**: 버텍스 셰이더의 `out vec2 vTexCoord;`와 프래그먼트 셰이더의 `in vec2 vTexCoord;`가 연결되는 이유는?

<details>
<summary>정답 보기</summary>

이름이 같고 타입이 같으면 자동 연결됩니다.
GLSL 링커가 `out`과 `in` 변수를 이름으로 매칭합니다.

```glsl
// 버텍스 셰이더
out vec2 vTexCoord;  // 출력

// 프래그먼트 셰이더
in vec2 vTexCoord;   // 입력 (같은 이름!)
```

</details>

### 문제 2: 메모리 계산 (중급)

**Q**: VBO에 4개 정점 (각 16 bytes)을 저장할 때 총 몇 bytes?

<details>
<summary>정답 보기</summary>

```
4 정점 × 16 bytes/정점 = 64 bytes
```

</details>

### 문제 3: 인덱스 해석 (중급)

**Q**: 인덱스 배열 `{0, 1, 2, 2, 3, 0}`이 생성하는 두 삼각형의 정점 순서는?

<details>
<summary>정답 보기</summary>

```
삼각형 1: 정점 0, 1, 2
삼각형 2: 정점 2, 3, 0

시각화:
   0 ─── 1
   │ ╲   │
   │   ╲ │
   3 ─── 2
```

</details>

### 문제 4: 텍스처 좌표 (중급)

**Q**: 256×256 텍스처에서 픽셀 (128, 64)의 UV 좌표는?

<details>
<summary>정답 보기</summary>

```
u = 128 / 256 = 0.5
v = 64 / 256 = 0.25
UV = (0.5, 0.25)
```

</details>

### 문제 5: glVertexAttribPointer (고급)

**Q**: `glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8)`의 각 파라미터 의미는?

<details>
<summary>정답 보기</summary>

```
1       - attribute location (셰이더의 layout(location=1))
2       - 컴포넌트 수 (vec2 = 2개의 float)
GL_FLOAT - 데이터 타입
GL_FALSE - 정규화 안 함
16      - stride (정점 간 간격, bytes)
(void*)8 - offset (이 속성의 시작 위치, bytes)
```

</details>

### 문제 6: 보간 계산 (고급)

**Q**: 정점 A(UV=0,0), B(UV=1,0), C(UV=0,1), D(UV=1,1)일 때, 쿼드 중앙 (u=0.5, v=0.5)의 보간된 UV는?

<details>
<summary>정답 보기</summary>

```
P(u,v) = (1-u)(1-v)A + u(1-v)B + (1-u)v*C + uv*D
P(0.5, 0.5) = 0.25×(0,0) + 0.25×(1,0) + 0.25×(0,1) + 0.25×(1,1)
            = (0,0) + (0.25,0) + (0,0.25) + (0.25,0.25)
            = (0.5, 0.5)

또는 단순히: 중앙점은 (0.5, 0.5)
```

</details>

---

## 📌 학습 체크리스트

- [ ] GPU 파이프라인의 각 단계를 설명할 수 있다
- [ ] VAO가 왜 필요한지 설명할 수 있다
- [ ] EBO를 사용하면 메모리가 절약되는 이유를 알고 있다
- [ ] glVertexAttribPointer의 각 파라미터를 이해했다
- [ ] GL_NEAREST와 GL_LINEAR의 차이를 알고 있다
- [ ] 텍스처 좌표가 0~1 사이인 이유를 이해했다

---

> **다음 문서**: [03_CPP_Patterns_Used.md](#) - SlippyGL에서 사용된 C++ 패턴
