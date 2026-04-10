# Build And Run

## 전제 조건

- Windows 10 이상
- Visual Studio 2022 또는 `v143` 툴셋이 포함된 Build Tools
- Windows 10 SDK

프로젝트 기본 설정은 [BlueFrog.vcxproj](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.vcxproj)에 정의된 `v143`입니다.

## Visual Studio에서 빌드

1. [BlueFrog.sln](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.sln)을 엽니다.
2. `Debug | x64` 또는 원하는 구성을 선택합니다.
3. `BlueFrog`를 시작 프로젝트로 실행합니다.

## 명령줄에서 빌드

기본 설정 그대로 아래 명령으로 빌드할 수 있습니다.

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  .\BlueFrog\BlueFrog.sln `
  /p:Configuration=Debug `
  /p:Platform=x64
```

이 워크스페이스에서 확인한 `MSBuild.exe` 경로는 다음과 같습니다.

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
```

## 실행 결과

정상 실행되면 다음 동작을 확인할 수 있습니다.

- `Blue Frog`라는 제목의 창이 열리고, 제목 표시줄에는 플레이어 체력과 타깃/목표 텍스트가 표시된다.
- 탑다운 카메라로 바라본 작은 아레나가 렌더링된다. 바닥 평면(`Ground`), 중앙 `ShrineCore`, 사방 벽(`NorthWall`/`SouthWall`/`EastWall`/`WestWall`), 기둥 2개(`PillarA`/`PillarB`)가 배치돼 있다.
- 연두색 큐브의 `Player`와 붉은색 큐브의 `EnemyScout` 1명이 아레나에 배치된다.
- `WASD`로 카메라 기준 이동, 마우스로 조준, 좌클릭으로 근접 공격이 가능하다.
- `Q`/`E`로 카메라 공전, 마우스 휠로 줌 인·아웃이 가능하다.
- 체력 바, 공격 쿨다운, 크로스헤어 등의 HUD가 직교 투영으로 화면에 오버레이된다.

## 트러블슈팅

### `MSB8020: v143 빌드 도구를 찾을 수 없습니다`

- Visual Studio Installer에서 `MSVC v143` 툴셋을 추가 설치합니다.
- 가능하면 Visual Studio 2022 환경에서 여는 쪽이 가장 단순합니다.

### Direct3D 초기화 예외가 발생합니다

- GPU 드라이버와 Direct3D 11 지원 여부를 확인합니다.
- 원격 세션이나 가상 환경에서는 하드웨어 장치 생성이 실패할 수 있습니다.
- 최근 코드 정리로 이제 실패 시 조용히 크래시하지 않고 예외 메시지가 표시됩니다.

### 입력 좌표가 이상합니다

- 현재 코드는 마우스 클릭과 휠 이벤트 시점의 좌표를 직접 갱신하도록 수정돼 있습니다.
- `WM_MOUSEWHEEL`은 스크린 좌표를 클라이언트 좌표로 변환한 뒤 처리합니다.
