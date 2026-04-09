# BlueFrog

BlueFrog는 Win32 창 위에 Direct3D 11 스왑체인을 올려 최소 렌더 루프를 구성한 C++ 학습용 엔진 스켈레톤입니다. 현재는 게임이라기보다 "기초 엔진 골격"에 가까우며, 창 생성, 메시지 루프, 입력 버퍼링, 타이머, 백버퍼 클리어까지 구현돼 있습니다.

## 현재 상태

- Win32 기반 창 생성 및 메시지 처리
- Direct3D 11 디바이스/스왑체인/렌더 타깃 생성
- 키보드/마우스 이벤트 버퍼링
- 프레임마다 창 제목에 경과 시간 표시
- 시간 값에 따라 배경색이 변하는 기본 렌더 루프
- Win32/Graphics 예외 메시지 정리

실행하면 `Blue Frog` 창이 뜨고, 제목 표시줄에는 경과 시간이 갱신되며 화면은 파란 계열 색으로 천천히 변화합니다.

## 디렉터리 구성

- [BlueFrog/BlueFrog.sln](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.sln): Visual Studio 솔루션
- [BlueFrog/BlueFrog.vcxproj](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.vcxproj): C++ 프로젝트 설정
- [BlueFrog/Core/WinMain.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/WinMain.cpp): 프로그램 진입점
- [BlueFrog/Core/App.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/App.cpp): 메인 루프와 프레임 처리
- [BlueFrog/Core/Window.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Window.cpp): Win32 창과 입력 메시지 처리
- [BlueFrog/Core/Graphics.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Graphics.cpp): Direct3D 11 초기화와 프레젠트
- [BlueFrog/Core/Keyboard.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Keyboard.h), [BlueFrog/Core/Mouse.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Mouse.h): 입력 상태/이벤트 큐
- [BlueFrog/Core/BFTimer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/BFTimer.cpp): 시간 측정
- [docs/BUILD_AND_RUN.md](/D:/Work/Projects/BlueFrog/docs/BUILD_AND_RUN.md): 빌드/실행 가이드
- [docs/ENGINE_EXTENSION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/ENGINE_EXTENSION_PLAN.md): Direct3D 기초 엔진 확장 설계안

## 요구 사항

- Windows 10 이상
- Visual Studio 2022 또는 `v143` 툴셋이 포함된 Build Tools
- Windows 10 SDK
- Direct3D 11 실행 가능한 GPU/드라이버 환경

현재 프로젝트 파일은 `PlatformToolset=v143`로 설정돼 있습니다. 이 환경에서 확인한 최신 설치 툴셋 기준으로 프로젝트를 맞췄기 때문에 Visual Studio 2022 환경에서 바로 빌드할 수 있습니다.

## 빠른 시작

자세한 절차는 [docs/BUILD_AND_RUN.md](/D:/Work/Projects/BlueFrog/docs/BUILD_AND_RUN.md)에 정리돼 있습니다. 핵심만 요약하면 아래와 같습니다.

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  .\BlueFrog\BlueFrog.sln `
  /p:Configuration=Debug `
  /p:Platform=x64
```

기본 프로젝트 설정만으로도 VS 2022 환경에서 바로 빌드됩니다.

## 최근 정리한 내용

- Direct3D 초기화 실패와 `Present()` 실패를 예외로 처리
- 마우스 클릭/휠 이벤트의 좌표 갱신 버그 수정
- `WM_MOUSEWHEEL`의 스크린 좌표를 클라이언트 좌표로 변환
- Win32 에러 문자열 처리 경로 정리
- 프로젝트 기본 툴셋을 `v143`로 업데이트
- 문서 구조 추가

## 다음 단계

- 렌더 상태와 리소스를 캡슐화한 `Renderer` 레이어 도입
- 셰이더, 버텍스 버퍼, 메쉬, 카메라, 변환 행렬 추가
- 앱 계층과 엔진 계층 분리
- 장면/엔티티/자산 로딩 구조 추가

## 참고

- 코딩 스타일 기준: [C++CodingStandards.txt](/D:/Work/Projects/BlueFrog/C++CodingStandards.txt)
