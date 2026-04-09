# BlueFrog

BlueFrog는 Win32 창 위에 Direct3D 11 스왑체인을 올려, 탑다운 판타지 액션 RPG를 만들기 위한 작은 C++ 전용 엔진으로 확장 중인 프로젝트입니다. 지금은 더 이상 단순 샘플이 아니라, 탑다운 테스트 맵과 플레이어 수직 슬라이스가 들어간 초기 게임 런타임 단계에 와 있습니다.

## 현재 상태

- Win32 기반 창 생성 및 메시지 처리
- Direct3D 11 디바이스/스왑체인/렌더 타깃 생성
- 탑다운 카메라와 작은 테스트 맵 렌더링
- 플레이어 이동, 마우스 조준, 좌클릭 근접 공격
- 적 1종 추적 및 접촉 공격
- 벽/기둥/적 충돌 처리
- 화면 오버레이 기반 HUD 바 렌더링
- Win32/Graphics 예외 메시지 정리

실행하면 `Blue Frog` 창이 뜨고, 화면에는 탑다운 테스트 아레나와 플레이어/적, 그리고 체력/쿨다운 HUD가 표시됩니다. 기본 조작은 `WASD` 이동, 마우스 조준, 좌클릭 공격, `Q/E` 카메라 공전, 마우스 휠 줌입니다.

## 디렉터리 구성

- [BlueFrog/BlueFrog.sln](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.sln): Visual Studio 솔루션
- [BlueFrog/BlueFrog.vcxproj](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.vcxproj): C++ 프로젝트 설정
- [BlueFrog/Core/WinMain.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/WinMain.cpp): 프로그램 진입점
- [BlueFrog/Core/App.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/App.cpp): 메인 루프와 프레임 처리
- [BlueFrog/Core/Window.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Window.cpp): Win32 창과 입력 메시지 처리
- [BlueFrog/Core/Graphics.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Graphics.cpp): Direct3D 11 초기화와 프레젠트
- [BlueFrog/Core/Renderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.cpp): 월드 지오메트리와 렌더 파이프라인
- [BlueFrog/Core/Keyboard.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Keyboard.h), [BlueFrog/Core/Mouse.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Mouse.h): 입력 상태/이벤트 큐
- [BlueFrog/Core/BFTimer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/BFTimer.cpp): 시간 측정
- [BlueFrog/Engine/UI/UIRenderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/UI/UIRenderer.cpp): HUD 전용 UI 드로우 계층
- [BlueFrog/Game/Player/PlayerController.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Player/PlayerController.cpp): 플레이어 이동/조준/공격 처리
- [BlueFrog/Game/NPC/SimpleEnemyController.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Game/NPC/SimpleEnemyController.cpp): 적 1종 추적 및 공격 처리
- [BlueFrog/Game/Combat/CombatSystem.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Combat/CombatSystem.cpp): 근접 공격 판정과 데미지 처리
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

- `SceneObject + Render/Collision/Combat` 기반 최소 컴포넌트 구조 추가
- 플레이어/적/충돌/전투가 들어간 Phase 3 수직 슬라이스 구현
- HUD를 `Renderer` 밖의 `UIRenderer` 계층으로 분리 시작
- 미사용 `WindowsMessageMap` 디버그 유틸 제거
- 프로젝트 기본 툴셋을 `v143`로 업데이트
- 단기 실행 계획과 전환 계획 문서 추가

## 다음 단계

- `UIRenderer`에 텍스트/아이콘 또는 간단한 위젯 계층 추가
- 플레이어/적 상태를 `System` 중심으로 한 단계 더 정리
- 텍스처/머티리얼과 조명으로 넘어가기 전 렌더/씬 책임 경계 다듬기
- 장면 데이터 직렬화의 첫 진입점 만들기

## 참고

- 코딩 스타일 기준: [C++CodingStandards.txt](/D:/Work/Projects/BlueFrog/C++CodingStandards.txt)
