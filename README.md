# SlippyGL

**SlippyGL**은 [OpenStreetMap](https://www.openstreetmap.org/)의 **래스터 타일(PNG)**을 다운로드하고,
OpenGL을 사용해 렌더링하는 C++ 프로젝트입니다.  
추후 **MVT(Vector Tile)**를 지원하여 클라이언트 측 스타일링 및 고품질 지도 렌더링으로 확장할 예정입니다.

---

## 📌 기능 (MVP)
- OSM XYZ 스킴 기반 타일 다운로드 (`z/x/y.png`)
- 디스크 캐시 및 메모리 캐시
- OpenGL 기반 타일 렌더링
- 카메라 이동(패닝) 및 확대/축소(줌)
- OSM 저작권 표기 표시

---

## 🚀 향후 계획
- **MVT(Vector Tile)** 지원 (`.mvt/.pbf`)
- MapLibre 스타일 JSON 파서 연동
- 벡터 기반 폴리곤/라인/라벨 렌더링
- 타일 다운로드 병렬화 (libcurl multi)

---

## 📂 프로젝트 구조

src/
app/ # 앱 실행 로직 (main.cpp)
core/ # 공통 타입, 좌표 변환, 설정
net/ # HTTP 클라이언트 (libcurl 기반)
cache/ # 디스크/메모리 캐시
decode/ # PNG/MVT 디코딩
render/ # OpenGL 렌더링
map/ # 카메라, 가시 타일 계산
tile/ # 타일 수명 관리
ui/ # 입력 처리 및 HUD


---

## 🛠 개발 환경
- **언어:** C++17 이상
- **빌드:** Visual Studio 2022 (MSBuild)
- **패키지 관리:** [vcpkg](https://github.com/microsoft/vcpkg) 매니페스트 모드
- **외부 라이브러리:**
  - curl[openssl], openssl
  - spdlog
  - nlohmann-json
  - glfw3
  - glad
  - glm
  - stb

---

## ⚙️ 빌드 방법
1. [vcpkg](https://github.com/microsoft/vcpkg) 설치 후 Visual Studio 2022에 통합  
2. 레포 루트에 `vcpkg.json` 확인  
3. 프로젝트 속성 → **vcpkg** → Use vcpkg Manifest = Yes, Triplet = x64-windows  
4. 빌드(F5) 실행

---

## 📜 라이선스
- 코드: MIT License
- 지도 데이터 및 타일: © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright) (ODbL)

---
