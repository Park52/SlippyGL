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