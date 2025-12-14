# 🔧 C++ 패턴과 모던 C++ 기법

> **학습 목표**: SlippyGL에서 사용된 C++ 패턴과 관용구 마스터
> 
> **예상 학습 시간**: 총 1.5시간
> 
> **사전 지식**: C++ 기본 문법

---

## 📚 목차

1. [RAII와 리소스 관리](#1-raii와-리소스-관리)
2. [스마트 포인터 대신 참조](#2-스마트-포인터-대신-참조)
3. [Fluent Interface 패턴](#3-fluent-interface-패턴)
4. [std::optional과 nullable 타입](#4-stdoptional과-nullable-타입)
5. [템플릿 특수화 (Hash)](#5-템플릿-특수화-hash)
6. [inline과 constexpr](#6-inline과-constexpr)
7. [연습 문제](#7-연습-문제)

---

# 1. RAII와 리소스 관리

## 📖 읽기 시간: 15분

### 1.1 RAII란?

**RAII** (Resource Acquisition Is Initialization)
- 리소스를 객체의 생명주기에 묶는 패턴
- 생성자에서 획득, 소멸자에서 해제

### 1.2 SlippyGL의 RAII 예시

```cpp
// TileCache.cpp
TileCache::TileCache(size_t budgetBytes) 
    : budgetBytes_(budgetBytes)  // 리소스 "획득" (초기화)
{
}

TileCache::~TileCache() {
    clear();  // 리소스 "해제"
}

void TileCache::clear() {
    for (auto& [key, node] : cache_) {
        glDeleteTextures(1, &node.entry.texture);  // GPU 리소스 해제
    }
    cache_.clear();
    lruList_.clear();
}
```

### 1.3 왜 RAII가 중요한가?

**RAII 없이:**
```cpp
void renderTile() {
    GLuint tex;
    glGenTextures(1, &tex);
    
    if (downloadFailed) {
        return;  // 🐛 메모리 누수! tex가 삭제 안 됨
    }
    
    // ... 사용 ...
    
    glDeleteTextures(1, &tex);
}
```

**RAII 적용:**
```cpp
class TextureHandle {
    GLuint tex_ = 0;
public:
    TextureHandle() { glGenTextures(1, &tex_); }
    ~TextureHandle() { if (tex_) glDeleteTextures(1, &tex_); }
    
    // 복사 금지 (이동만 허용)
    TextureHandle(const TextureHandle&) = delete;
    TextureHandle& operator=(const TextureHandle&) = delete;
    
    GLuint get() const { return tex_; }
};

void renderTile() {
    TextureHandle tex;  // 생성자에서 획득
    
    if (downloadFailed) {
        return;  // ✅ 소멸자가 자동 호출 → 안전
    }
    
    // ... 사용 ...
}  // 소멸자에서 자동 해제
```

### 1.4 QuadRenderer의 RAII

```cpp
class QuadRenderer {
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLuint program_ = 0;
    
public:
    bool init() {
        glGenVertexArrays(1, &vao_);  // 획득
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);
        // ...
    }
    
    void shutdown() {
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
        if (program_) { glDeleteProgram(program_); program_ = 0; }
    }
};
```

**주의**: `shutdown()`을 명시적으로 호출해야 하는 이유
- OpenGL 컨텍스트가 유효할 때만 `glDelete*` 호출 가능
- 프로그램 종료 순서를 제어하기 위함

### 1.5 `= 0` 초기화의 중요성

```cpp
GLuint vao_ = 0;  // 0으로 초기화!
```

**왜 0으로 초기화?**
- OpenGL에서 0은 "유효하지 않은 핸들"
- `if (vao_)` 체크로 유효성 확인 가능
- 이중 삭제 방지: `glDeleteTextures(1, &tex); tex = 0;`

---

# 2. 스마트 포인터 대신 참조

## 📖 읽기 시간: 10분

### 2.1 SlippyGL의 의존성 주입

```cpp
class TileRenderer {
    TileCache& cache_;          // 참조로 저장
    TileDownloader& downloader_;
    TextureManager& texMgr_;
    
public:
    TileRenderer(TileCache& cache, TileDownloader& downloader, TextureManager& texMgr)
        : cache_(cache)
        , downloader_(downloader)
        , texMgr_(texMgr)
    {}
};
```

### 2.2 왜 참조를 사용하는가?

**장점:**
1. **명확한 소유권**: "나는 소유하지 않는다"를 명시
2. **null 불가**: 참조는 항상 유효한 객체 가리킴
3. **오버헤드 없음**: 포인터와 같은 크기, 추가 처리 없음

**비교:**
```cpp
// 참조 (권장)
TileCache& cache_;
// → 호출자가 생명주기 관리, null 불가

// raw 포인터
TileCache* cache_;
// → null 가능, 소유권 불명확

// shared_ptr
std::shared_ptr<TileCache> cache_;
// → 참조 카운팅 오버헤드, 순환 참조 위험
```

### 2.3 호출부 코드

```cpp
void RunTileRenderDemo() {
    // 호출자가 객체 생성 및 생명주기 관리
    TileCache texCache(128 * 1024 * 1024);
    TileDownloader downloader(...);
    TextureManager texMgr;
    
    // 참조로 전달
    TileRenderer renderer(texCache, downloader, texMgr);
    
    // ... 사용 ...
    
}  // 모든 객체가 역순으로 소멸 (안전)
```

### 2.4 언제 스마트 포인터를 사용하나?

| 상황 | 추천 |
|-----|------|
| 소유권 공유 필요 | `shared_ptr` |
| 배타적 소유 + 이동 | `unique_ptr` |
| 소유권 없음, null 불가 | 참조 `&` |
| 소유권 없음, null 가능 | 포인터 `*` 또는 `optional` |

---

# 3. Fluent Interface 패턴

## 📖 읽기 시간: 10분

### 3.1 예시: NetConfig

```cpp
class NetConfig {
    std::string userAgent_;
    bool verifyTLS_ = true;
    bool http2_ = false;
    int maxRetries_ = 3;
    
public:
    NetConfig& setUserAgent(const std::string& ua) {
        userAgent_ = ua;
        return *this;  // 자기 자신 반환!
    }
    
    NetConfig& setVerifyTLS(bool v) {
        verifyTLS_ = v;
        return *this;
    }
    
    NetConfig& setHttp2(bool v) {
        http2_ = v;
        return *this;
    }
};
```

### 3.2 사용법

```cpp
// 체이닝 (Fluent)
NetConfig cfg;
cfg.setUserAgent("SlippyGL/0.1")
   .setVerifyTLS(true)
   .setHttp2(true)
   .setMaxRetries(2);

// 일반 방식
NetConfig cfg;
cfg.setUserAgent("SlippyGL/0.1");
cfg.setVerifyTLS(true);
cfg.setHttp2(true);
cfg.setMaxRetries(2);
```

### 3.3 왜 Fluent Interface?

1. **가독성**: 설정이 연속적으로 보임
2. **간결함**: 변수명 반복 줄임
3. **빌더 패턴과 유사**: 복잡한 객체 구성에 적합

### 3.4 주의사항

```cpp
// 잘못된 예: 임시 객체에 체이닝
processConfig(NetConfig().setUserAgent("...").setHttp2(true));
// → 임시 객체의 참조가 함수에 전달됨 (위험할 수 있음)

// 권장: 변수에 저장 후 사용
NetConfig cfg;
cfg.setUserAgent("...").setHttp2(true);
processConfig(cfg);
```

---

# 4. std::optional과 nullable 타입

## 📖 읽기 시간: 10분

### 4.1 문제: 값이 없을 수 있는 경우

```cpp
// 나쁜 예: 매직 값 사용
int findIndex() {
    // ...
    return -1;  // 못 찾음을 -1로 표현 (위험!)
}

// 나쁜 예: 포인터 반환
std::string* getOptionalValue() {
    // ...
    return nullptr;  // null 체크 강제 불가
}
```

### 4.2 std::optional 사용

```cpp
// SlippyGL CacheMeta에서
class CacheMeta {
    std::optional<std::string> etag_;
    std::optional<std::string> contentEncoding_;
    
public:
    CacheMeta& setEtag(const std::string& v) {
        etag_ = v;
        return *this;
    }
    
    CacheMeta& setContentEncoding(std::optional<std::string> v) {
        contentEncoding_ = v;
        return *this;
    }
    
    std::optional<std::string> etag() const { return etag_; }
};
```

### 4.3 optional 사용법

```cpp
std::optional<std::string> maybeValue = getOptionalValue();

// 방법 1: has_value() 체크
if (maybeValue.has_value()) {
    std::cout << *maybeValue;  // 역참조
}

// 방법 2: value_or() 기본값
std::string value = maybeValue.value_or("default");

// 방법 3: if 초기화 구문 (C++17)
if (auto val = getOptionalValue(); val) {
    std::cout << *val;
}
```

### 4.4 TileCache에서의 활용

```cpp
bool TileCache::get(const TileKey& key, TexHandle& outTex) {
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return false;  // 값 없음
    }
    outTex = it->second.entry.texture;
    return true;  // 값 있음
}

// 더 나은 설계 (optional 반환)
std::optional<TexHandle> TileCache::get(const TileKey& key) {
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second.entry.texture;
}
```

---

# 5. 템플릿 특수화 (Hash)

## 📖 읽기 시간: 15분

### 5.1 std::hash 특수화가 필요한 이유

```cpp
std::unordered_map<TileKey, Texture> cache;  // 컴파일 에러!
// → TileKey의 해시 함수가 정의되지 않음
```

### 5.2 해시 함수 특수화

```cpp
// std 네임스페이스에 특수화 추가
namespace std {
    template<>  // 명시적 특수화
    struct hash<slippygl::tile::TileKey> {
        size_t operator()(const TileKey& key) const noexcept {
            size_t h = static_cast<size_t>(key.z);
            h = h * 31 + static_cast<size_t>(key.x);
            h = h * 31 + static_cast<size_t>(key.y);
            return h;
        }
    };
}
```

### 5.3 템플릿 특수화 문법

```cpp
// 기본 템플릿
template<typename T>
struct hash {
    size_t operator()(const T& v) const;  // 정의 없음 → 에러
};

// 특수화 (특정 타입에 대한 구현)
template<>  // 빈 꺾쇠: "모든 템플릿 파라미터 특수화"
struct hash<MyType> {
    size_t operator()(const MyType& v) const {
        return /* 구현 */;
    }
};
```

### 5.4 해시 함수 설계 원칙

**좋은 해시 함수의 특성:**
1. **결정적**: 같은 입력 → 항상 같은 출력
2. **균일 분포**: 출력이 전체 범위에 고르게 분포
3. **빠름**: O(1) 시간

**31을 곱하는 이유:**
```cpp
h = h * 31 + value;
```
- 31은 소수 → 충돌 감소
- `31 = 2^5 - 1` → 컴파일러가 `(h << 5) - h`로 최적화
- Java의 String.hashCode()에서 유래

### 5.5 대안: 함수 객체로 전달

```cpp
// 특수화 대신 사용자 정의 해시 함수
struct TileKeyHash {
    size_t operator()(const TileKey& key) const noexcept {
        // ...
    }
};

// 명시적으로 해시 함수 지정
std::unordered_map<TileKey, Texture, TileKeyHash> cache;
```

**장단점:**
- 특수화: 코드 어디서든 `unordered_map<TileKey, T>` 사용 가능
- 함수 객체: 더 명시적, std 네임스페이스 오염 없음

---

# 6. inline과 constexpr

## 📖 읽기 시간: 10분

### 6.1 inline 함수

```cpp
// TileKey.hpp
inline int worldPxToTileIndex(float worldPx) {
    return static_cast<int>(std::floor(worldPx / 256.0f));
}
```

**inline의 의미:**
1. **ODR 예외**: 헤더에 정의해도 다중 정의 에러 안 남
2. **인라인화 힌트**: 함수 호출 대신 코드 삽입 (힌트일 뿐!)

**현대 C++에서:**
```cpp
// 헤더에 정의하는 함수는 inline 필요
inline void foo() { }  // OK

// 클래스 내 정의는 암시적 inline
class A {
    void bar() { }  // 암시적 inline
};
```

### 6.2 constexpr

```cpp
// TileKey.hpp
constexpr int kTileSizePx = 256;

// TileCache.hpp
static constexpr std::size_t kDefaultBudgetBytes = 128 * 1024 * 1024;
```

**constexpr의 의미:**
- **컴파일 타임 상수**: 런타임이 아닌 컴파일 시 계산
- **const보다 강력**: const는 런타임 초기화 가능

**비교:**
```cpp
const int a = getRuntime();      // OK (런타임)
constexpr int b = getRuntime();  // 에러! (컴파일 타임이어야 함)

constexpr int c = 256;           // OK
constexpr int d = c * c;         // OK (컴파일 타임 계산)
```

### 6.3 static constexpr 멤버

```cpp
class TileCache {
    static constexpr size_t kDefaultBudget = 128 * 1024 * 1024;
    //     ↑ 클래스에 속함 (인스턴스당 아님)
    //              ↑ 컴파일 타임 상수
};
```

**static의 의미:**
- 클래스의 모든 인스턴스가 공유
- 객체 생성 없이 `TileCache::kDefaultBudget`로 접근

### 6.4 noexcept와 조합

```cpp
constexpr int kTileSizePx = 256;  // 상수

inline int worldPxToTileIndex(float worldPx) noexcept {  // 함수
    return static_cast<int>(std::floor(worldPx / static_cast<float>(kTileSizePx)));
}
```

**조합 사용:**
- `inline`: 헤더에 정의
- `noexcept`: 예외 없음 보장
- 내부에서 `constexpr` 상수 사용

---

# 7. 연습 문제

## 📖 풀이 시간: 15분

### 문제 1: RAII 이해 (기초)

**Q**: 다음 코드의 문제점은?

```cpp
void loadTexture() {
    GLuint tex;
    glGenTextures(1, &tex);
    
    if (!downloadImage()) {
        return;
    }
    
    // ... 텍스처 사용 ...
    
    glDeleteTextures(1, &tex);
}
```

<details>
<summary>정답 보기</summary>

`downloadImage()`가 실패하면 `glDeleteTextures`가 호출되지 않아 메모리 누수 발생.

RAII 해결책:
```cpp
class TextureGuard {
    GLuint tex_;
public:
    TextureGuard() { glGenTextures(1, &tex_); }
    ~TextureGuard() { glDeleteTextures(1, &tex_); }
    GLuint get() { return tex_; }
};

void loadTexture() {
    TextureGuard tex;  // 자동 관리
    if (!downloadImage()) return;  // 안전
    // ...
}
```

</details>

### 문제 2: 참조 vs 포인터 (중급)

**Q**: 아래 두 가지 방식의 차이점은?

```cpp
// 방식 1
TileRenderer(TileCache& cache);

// 방식 2
TileRenderer(TileCache* cache);
```

<details>
<summary>정답 보기</summary>

| 특성 | 참조 (`&`) | 포인터 (`*`) |
|-----|-----------|-------------|
| null 가능 | 불가 | 가능 |
| 재할당 | 불가 | 가능 |
| 구문 | `cache.method()` | `cache->method()` |
| 의도 | "반드시 필요" | "선택적" |

참조 사용 시 null 체크 불필요, 더 안전.

</details>

### 문제 3: Fluent Interface 구현 (중급)

**Q**: 다음 클래스에 Fluent Interface 패턴을 적용하세요.

```cpp
class WindowConfig {
    int width_, height_;
    std::string title_;
public:
    void setWidth(int w) { width_ = w; }
    void setHeight(int h) { height_ = h; }
    void setTitle(const std::string& t) { title_ = t; }
};
```

<details>
<summary>정답 보기</summary>

```cpp
class WindowConfig {
    int width_, height_;
    std::string title_;
public:
    WindowConfig& setWidth(int w) { 
        width_ = w; 
        return *this;  // 추가
    }
    WindowConfig& setHeight(int h) { 
        height_ = h; 
        return *this;  // 추가
    }
    WindowConfig& setTitle(const std::string& t) { 
        title_ = t; 
        return *this;  // 추가
    }
};

// 사용
WindowConfig cfg;
cfg.setWidth(800).setHeight(600).setTitle("My Window");
```

</details>

### 문제 4: optional 활용 (중급)

**Q**: 다음 함수를 `std::optional`을 사용하도록 리팩토링하세요.

```cpp
bool TileCache::get(const TileKey& key, TexHandle& outTex) {
    auto it = cache_.find(key);
    if (it == cache_.end()) return false;
    outTex = it->second.texture;
    return true;
}
```

<details>
<summary>정답 보기</summary>

```cpp
std::optional<TexHandle> TileCache::get(const TileKey& key) {
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second.texture;
}

// 사용
if (auto tex = cache.get(key)) {
    render(*tex);
}
```

</details>

### 문제 5: 해시 함수 (고급)

**Q**: 다음 Point 구조체의 해시 함수를 구현하세요.

```cpp
struct Point {
    int x, y;
};
```

<details>
<summary>정답 보기</summary>

```cpp
namespace std {
    template<>
    struct hash<Point> {
        size_t operator()(const Point& p) const noexcept {
            size_t h = static_cast<size_t>(p.x);
            h = h * 31 + static_cast<size_t>(p.y);
            return h;
        }
    };
}
```

또는 std::hash 조합:
```cpp
size_t operator()(const Point& p) const noexcept {
    size_t h1 = std::hash<int>{}(p.x);
    size_t h2 = std::hash<int>{}(p.y);
    return h1 ^ (h2 << 1);  // XOR 조합
}
```

</details>

---

## 📌 학습 체크리스트

- [ ] RAII가 왜 중요한지 설명할 수 있다
- [ ] 참조로 의존성을 주입하는 이유를 알고 있다
- [ ] Fluent Interface 패턴을 구현할 수 있다
- [ ] std::optional을 적절히 사용할 수 있다
- [ ] std::hash를 특수화할 수 있다
- [ ] inline, constexpr, static의 차이를 알고 있다

---

> **다음 문서**: [04_Debugging_and_Optimization.md](#) - 디버깅과 최적화 기법
