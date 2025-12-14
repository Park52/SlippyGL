# 📝 SlippyGL 빠른 참조 카드

> **목적**: 출퇴근 중 빠르게 핵심 개념 복습
> 
> **사용법**: 스마트폰에서 스크롤하며 읽기

---

## 🗺️ 타일 시스템 공식

### 줌 레벨과 타일 수
```
타일 수 = 2^z × 2^z = 4^z

z=0:  1개    (전 세계)
z=10: 100만개 (도시)
z=12: 1600만개 (동네)
z=18: 68억개  (건물)
```

### 좌표 변환
```
월드 픽셀 → 타일 인덱스
  tileX = floor(worldPx / 256)

타일 인덱스 → 월드 픽셀 (좌상단)
  worldPx = tileX × 256
```

---

## 🎮 OpenGL 핵심 순서

### 렌더링 파이프라인
```
정점 데이터 (CPU)
    ↓
버텍스 셰이더 (위치 변환)
    ↓
래스터화 (삼각형 → 픽셀)
    ↓
프래그먼트 셰이더 (색상)
    ↓
프레임버퍼 (화면)
```

### 버퍼 관계
```
VAO ─┬─ VBO (정점 데이터)
     └─ EBO (인덱스 데이터)
```

### 사각형 인덱스
```
정점: 0,1,2,3 (4개)
인덱스: [0,1,2, 2,3,0] (6개)

0 ─── 1
│ ╲   │
│   ╲ │
3 ─── 2
```

---

## 🔢 변환 행렬 요약

### MVP 순서
```
최종좌표 = Projection × View × Model × 정점
           (투영)      (카메라) (객체)
```

### 뷰 행렬 (카메라)
```cpp
view = scale × translate(-origin)
// 카메라가 (100,50)에 있으면
// 세상을 (-100,-50) 이동
```

### screenToWorld
```cpp
world = screen/scale + origin
```

### 커서 중심 줌
```cpp
worldBefore = screenToWorld(cursor)
scale *= (1 + delta)
worldAfter = screenToWorld(cursor)
origin += worldBefore - worldAfter
```

---

## 💾 LRU 캐시 알고리즘

### 자료구조
```
list<Key>: [A] ↔ [B] ↔ [C] ↔ [D]
            ↑ 최근        오래됨 ↑

map<Key, {data, iter}>: O(1) 조회
```

### 연산
```
get(A): A를 리스트 맨 앞으로 이동
put(E): 예산 초과 시 맨 뒤 제거
evict(): lruList.back() 삭제
```

### 시간 복잡도
```
get:   O(1)
put:   O(1)
evict: O(1)
```

---

## 🔧 C++ 패턴 요약

### RAII
```cpp
생성자: 리소스 획득
소멸자: 리소스 해제
→ 예외에도 안전
```

### 참조 vs 포인터
```cpp
Foo& ref;  // null 불가, 재할당 불가
Foo* ptr;  // null 가능, 재할당 가능
```

### Fluent Interface
```cpp
config.setA(1)
      .setB(2)
      .setC(3);
```

### std::optional
```cpp
std::optional<int> val;
if (val) use(*val);
val.value_or(default);
```

---

## 📐 유용한 수학

### 비트 연산
```
1 << n = 2^n
예: 1 << 12 = 4096
```

### floor vs 캐스트
```cpp
(int)(-0.5) = 0   // 잘못됨
floor(-0.5) = -1  // 올바름
```

### 체커보드 패턴
```cpp
bool dark = ((x/16) + (y/16)) % 2 == 0;
```

### 보간 공식
```
P(t) = (1-t)A + tB
P(0.5) = 0.5A + 0.5B (중간점)
```

---

## 🎨 텍스처 설정

### 필터링
```cpp
GL_NEAREST  // 픽셀 아트 (박스형)
GL_LINEAR   // 부드러움 (사진)
```

### 랩 모드
```cpp
GL_REPEAT         // 타일링
GL_CLAMP_TO_EDGE  // 경계 고정 (타일 맵용!)
```

### 텍스처 좌표 (UV)
```
(0,0) = 좌하단 (OpenGL)
(1,1) = 우상단
```

---

## 🐛 디버깅 체크리스트

### 빈 화면
- [ ] 셰이더 컴파일 에러 확인
- [ ] MVP 행렬이 단위행렬인지
- [ ] 텍스처 바인딩 확인
- [ ] glGetError() 체크

### 타일 경계 틈새
- [ ] floor()로 정수 스냅
- [ ] GL_CLAMP_TO_EDGE 설정
- [ ] GL_NEAREST 필터

### 메모리 누수
- [ ] glDeleteTextures 호출
- [ ] 캐시 eviction 확인
- [ ] shutdown() 순서

---

## ⚡ 성능 팁

### 프레임당 제한
```cpp
constexpr int kMaxDownloadsPerFrame = 3;
// 나머지는 placeholder 표시
```

### 캐시 예산
```cpp
128 MB = ~512 타일 (256KB each)
```

### VAO 활용
```cpp
// 매 프레임 설정 대신
glBindVertexArray(vao);  // 한 번에 복원
```

---

## 📖 용어 정리

| 용어 | 의미 |
|-----|------|
| Slippy Map | 웹 타일 지도 |
| Frustum Culling | 시야 밖 제외 |
| LRU | 가장 오래된 것 제거 |
| MVP | Model-View-Projection |
| VAO | 정점 설정 저장소 |
| VBO | 정점 데이터 버퍼 |
| EBO | 인덱스 버퍼 |
| RAII | 생성자/소멸자로 자원관리 |
| Fluent | 체이닝 패턴 |

---

## ✅ 복습 질문

1. 줌 12에서 타일 수는? → `2^12 × 2^12 = 16,777,216`

2. LRU get의 시간 복잡도? → `O(1)`

3. 뷰 행렬에서 왜 -origin? → 카메라 이동 = 세상 반대로 이동

4. EBO 사용 이유? → 중복 정점 제거

5. inline의 역할? → ODR 예외 + 인라인화 힌트

---

> **Tip**: 매일 하나의 섹션만 읽어도 1주일이면 전체 복습!
