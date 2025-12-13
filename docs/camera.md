# Camera2D: 2D Pan/Zoom Camera System

## Overview

Camera2D는 SlippyGL의 2D 타일 맵 렌더링을 위한 카메라 시스템입니다.
Y-down 좌표계를 사용하며, 마우스 드래그 팬과 휠 줌을 지원합니다.

## Coordinate Systems

### Screen Coordinates
- 원점: 화면 왼쪽 상단 (0, 0)
- X: 오른쪽으로 증가
- Y: 아래로 증가
- 단위: 픽셀

### World Coordinates
- 원점: 월드 왼쪽 상단 (0, 0)
- X: 오른쪽으로 증가
- Y: 아래로 증가 (Y-down)
- 단위: 월드 픽셀 (타일 기준)

## Camera State

```cpp
class Camera2D {
    float worldOriginX_;  // 화면 (0,0)에 보이는 월드 X 좌표
    float worldOriginY_;  // 화면 (0,0)에 보이는 월드 Y 좌표
    float scale_;         // 줌 레벨 (1.0 = 1:1, 2.0 = 2배 확대)
};
```

## Coordinate Transformations

### Screen → World

```cpp
glm::vec2 screenToWorld(float sx, float sy) const {
    return glm::vec2(
        worldOriginX_ + sx / scale_,
        worldOriginY_ + sy / scale_
    );
}
```

**수식:**
```
W_x = O_x + S_x / scale
W_y = O_y + S_y / scale
```

### World → Screen

```cpp
glm::vec2 worldToScreen(float wx, float wy) const {
    return glm::vec2(
        (wx - worldOriginX_) * scale_,
        (wy - worldOriginY_) * scale_
    );
}
```

**수식:**
```
S_x = (W_x - O_x) × scale
S_y = (W_y - O_y) × scale
```

## Matrix Calculations

### Orthographic Projection (Y-down)

```cpp
glm::mat4 ortho(int fbW, int fbH) const {
    return glm::ortho(
        0.0f, (float)fbW,    // left, right
        (float)fbH, 0.0f,    // bottom, top (Y-down)
        -1.0f, 1.0f          // near, far
    );
}
```

**행렬:**
```
| 2/w    0     0   -1  |
|  0   -2/h    0    1  |
|  0     0    -1    0  |
|  0     0     0    1  |
```

### View Matrix

```cpp
glm::mat4 viewMatrix() const {
    glm::mat4 view(1.0f);
    view = glm::scale(view, glm::vec3(scale_, scale_, 1.0f));
    view = glm::translate(view, glm::vec3(-worldOriginX_, -worldOriginY_, 0.0f));
    return view;
}
```

**변환 순서:**
1. 월드 원점으로 이동 (translate)
2. 스케일 적용 (scale)

**행렬:**
```
| scale    0      0   -scale × O_x |
|   0    scale    0   -scale × O_y |
|   0      0      1        0       |
|   0      0      0        1       |
```

### Combined MVP

```cpp
glm::mat4 mvp(int fbW, int fbH) const {
    return ortho(fbW, fbH) * viewMatrix();
}
```

## Pan (드래그)

```cpp
void pan(float dx, float dy) {
    // 화면 드래그 → 월드 이동 (역방향)
    worldOriginX_ -= dx / scale_;
    worldOriginY_ -= dy / scale_;
}
```

**동작:**
- 마우스를 오른쪽으로 드래그 (dx > 0) → 월드가 왼쪽으로 이동
- 마우스를 아래로 드래그 (dy > 0) → 월드가 위로 이동

## Zoom (커서 기준)

```cpp
void zoomAt(float cx, float cy, float zoomDelta, int fbW, int fbH) {
    // 1. 줌 전 커서 아래 월드 좌표
    glm::vec2 worldBefore = screenToWorld(cx, cy);
    
    // 2. 스케일 적용
    scale_ *= (1.0f + kZoomSpeed * zoomDelta);
    scale_ = clamp(scale_, kMinScale, kMaxScale);
    
    // 3. 줌 후 커서 아래 월드 좌표
    glm::vec2 worldAfter = screenToWorld(cx, cy);
    
    // 4. 원점 보정 (동일 월드 포인트 유지)
    worldOriginX_ += (worldBefore.x - worldAfter.x);
    worldOriginY_ += (worldBefore.y - worldAfter.y);
}
```

**원리:**
줌 전후로 커서 아래의 월드 좌표가 변하면, 그 차이만큼 원점을 보정하여
사용자가 보는 포인트가 커서 아래에 고정되도록 합니다.

## Input Handling

### InputHandler 클래스

```
GLFW Callbacks
     ↓
InputHandler (브릿지)
     ↓
Camera2D (상태 업데이트)
```

### 키 바인딩

| 입력 | 동작 |
|------|------|
| 좌클릭 드래그 | 팬 (pan) |
| 스크롤 휠 ↑ | 줌 인 |
| 스크롤 휠 ↓ | 줌 아웃 |
| R 키 | 카메라 리셋 |
| ESC 키 | 종료 |

## Constants

```cpp
static constexpr float kMinScale = 0.25f;   // 최소 줌 (4배 축소)
static constexpr float kMaxScale = 8.0f;    // 최대 줌 (8배 확대)
static constexpr float kZoomSpeed = 0.1f;   // 줌 속도 배율
```

## Pixel Alignment

선명한 타일 렌더링을 위해 쿼드 좌표를 정수로 정렬합니다:

```cpp
// QuadRenderer::draw()
const float x0 = std::floor(static_cast<float>(q.x));
const float y0 = std::floor(static_cast<float>(q.y));
```

**조건:**
- 텍스처 필터링: `GL_NEAREST`
- 텍스처 래핑: `GL_CLAMP_TO_EDGE`
- 쿼드 좌표: `floor()` 정렬

## Future Extensions

### Multi-Tile Rendering
```cpp
// 가시 타일 범위 계산
glm::vec2 topLeft = camera.screenToWorld(0, 0);
glm::vec2 bottomRight = camera.screenToWorld(fbW, fbH);
int tileMinX = (int)floor(topLeft.x / TILE_SIZE);
int tileMaxX = (int)ceil(bottomRight.x / TILE_SIZE);
// ...
```

### Frustum Culling
화면에 보이는 영역만 렌더링하여 성능 최적화

### Smooth Zoom Animation
줌 델타를 프레임 간 보간하여 부드러운 애니메이션 구현
