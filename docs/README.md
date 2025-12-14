# 📚 SlippyGL 학습 가이드

> **저자**: GitHub Copilot  
> **대상**: C++ 및 그래픽스 학습 중인 개발자  
> **마지막 업데이트**: 2025-12-15

---

## 🎯 학습 목표

이 문서들을 모두 학습하면 다음을 이해할 수 있습니다:

1. 웹 지도 타일 시스템의 동작 원리
2. OpenGL 그래픽스 파이프라인
3. LRU 캐시 알고리즘 구현
4. 2D 카메라와 좌표 변환
5. 모던 C++ 패턴 및 관용구

---

## 📖 학습 순서

### Day 1-2: 타일 시스템 기초
📄 **[00_QuickReference.md](./00_QuickReference.md)** (10분)
- 출퇴근 중 빠른 참조용
- 핵심 공식과 용어 정리
- 스마트폰에서 읽기 최적화

📄 **[01_TileSystem_DeepDive.md](./01_TileSystem_DeepDive.md)** (2-3시간)
- TileKey 구조체와 해시 함수
- TileGrid: 절두체 컬링
- TileCache: LRU 알고리즘
- TileRenderer: 파이프라인 오케스트레이션
- Camera2D: 변환 행렬
- 전체 시퀀스 다이어그램
- 연습 문제 6개

### Day 3-4: OpenGL 그래픽스
📄 **[02_OpenGL_Graphics_Basics.md](./02_OpenGL_Graphics_Basics.md)** (2시간)
- 그래픽스 파이프라인 개요
- 버텍스/프래그먼트 셰이더
- VAO, VBO, EBO 이해
- 텍스처와 샘플링
- QuadRenderer 완전 해부
- 연습 문제 6개

### Day 5: C++ 패턴
📄 **[03_CPP_Patterns_Used.md](./03_CPP_Patterns_Used.md)** (1.5시간)
- RAII와 리소스 관리
- 참조를 통한 의존성 주입
- Fluent Interface 패턴
- std::optional 활용
- std::hash 템플릿 특수화
- inline과 constexpr
- 연습 문제 5개

### 보너스: 카메라 시스템
📄 **[camera.md](./camera.md)**
- Camera2D 상세 설계 문서

---

## 📅 추천 학습 일정

### 출퇴근 학습 (하루 30분)

| 일 | 내용 | 문서 |
|---|------|------|
| 1일 | 빠른 참조 + 타일 기초 | 00, 01 §1-2 |
| 2일 | TileGrid, TileCache | 01 §3-4 |
| 3일 | TileRenderer, Camera | 01 §5-6 |
| 4일 | 시퀀스 + 연습문제 | 01 §7-8 |
| 5일 | OpenGL 파이프라인 | 02 §1-2 |
| 6일 | VAO/VBO/EBO, 텍스처 | 02 §3-4 |
| 7일 | QuadRenderer + 연습 | 02 §5-6 |
| 8일 | C++ 패턴 (전반) | 03 §1-3 |
| 9일 | C++ 패턴 (후반) | 03 §4-6 |
| 10일 | 전체 복습 | 00 |

---

## 💻 실습 환경

### 필수 도구
- Visual Studio 2022
- vcpkg (패키지 관리)
- Git (버전 관리)

### 빌드 방법
```powershell
cd d:\sources\SlippyGL\SlippyGL
msbuild SlippyGL.vcxproj /p:Configuration=Debug /p:Platform=x64
```

### 실행
```powershell
.\x64\Debug\SlippyGL.exe
```

### 조작법
- **드래그**: 지도 이동 (Pan)
- **스크롤**: 확대/축소 (Zoom)
- **R 키**: 초기 위치로 리셋
- **ESC**: 종료

---

## 📝 학습 체크리스트

### 타일 시스템 (01)
- [ ] 줌 레벨별 타일 수 공식 암기
- [ ] 월드 픽셀 ↔ 타일 인덱스 변환 가능
- [ ] LRU 캐시의 list+map 구조 이해
- [ ] 절두체 컬링 알고리즘 설명 가능
- [ ] MVP 행렬 순서 이해

### OpenGL (02)
- [ ] 파이프라인 5단계 나열 가능
- [ ] VAO, VBO, EBO 역할 구분
- [ ] glVertexAttribPointer 파라미터 이해
- [ ] GL_NEAREST vs GL_LINEAR 차이
- [ ] EBO로 메모리 절약 원리

### C++ 패턴 (03)
- [ ] RAII 패턴 구현 가능
- [ ] 참조 vs 포인터 선택 기준
- [ ] Fluent Interface 구현 가능
- [ ] std::optional 적절히 사용
- [ ] std::hash 특수화 가능

---

## 🔗 추가 학습 자료

### OpenGL
- [LearnOpenGL](https://learnopengl.com/) - 최고의 OpenGL 튜토리얼
- [OpenGL 레퍼런스](https://docs.gl/) - API 문서

### 타일 맵
- [OpenStreetMap Wiki: Slippy map tilenames](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames)
- [Google Maps API 문서](https://developers.google.com/maps/documentation)

### C++
- [C++ Reference](https://en.cppreference.com/) - 표준 라이브러리 문서
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) - 모던 C++ 가이드라인

---

## 📊 문서 통계

| 문서 | 읽기 시간 | 연습 문제 | 난이도 |
|-----|---------|---------|-------|
| 00_QuickReference | 10분 | - | ⭐ |
| 01_TileSystem | 2-3시간 | 6개 | ⭐⭐⭐ |
| 02_OpenGL | 2시간 | 6개 | ⭐⭐⭐ |
| 03_CPP_Patterns | 1.5시간 | 5개 | ⭐⭐ |
| **총계** | **~6시간** | **17개** | |

---

> **Tip**: 이해가 안 되는 부분은 코드와 함께 보세요!
> 
> 문서의 모든 코드는 `SlippyGL/src/` 디렉토리에 있습니다.

---

*Happy Learning! 🚀*
