# Engine Extension Plan

> ⚠ **이 문서는 초기 엔진 확장 설계안의 히스토리 기록이다.** 아래 "권장 마일스톤" Milestone 1–5는 이미 모두 완료된 상태다.
> - Phase 0–3 완료 스냅샷과 Phase 4 예고는 [PHASE_0_TO_3_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_0_TO_3_EXECUTION_PLAN.md)를 본다.
> - Phase 4 실행 계획(진행 중): [PHASE_4_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_4_EXECUTION_PLAN.md).
> - 장기 컴포넌트/시스템 전환은 [GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md)를 본다.

For the short-term runtime structure shift toward `GameObject + Component`, also see `GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md`.

이 문서 본문은 초기 설계 의도를 그대로 남겨 두고, 아래 내용은 참고용으로만 읽는다.

## 목표

BlueFrog를 "창만 띄우는 샘플"에서 "기초 Direct3D 엔진"으로 확장합니다. 목표는 대형 엔진을 흉내 내는 것이 아니라, 렌더링과 게임 루프의 핵심 흐름을 직접 통제할 수 있는 작고 명확한 구조를 만드는 것입니다.

## 설계 원칙

- 현재의 단순한 진입 구조는 유지하되 책임을 더 잘 분리한다.
- Win32, 렌더링, 장면, 앱 로직을 계층별로 분리한다.
- 예외와 로그를 초기에 정리해 디버깅 비용을 낮춘다.
- "그리기 한 번 성공"보다 "확장 가능한 최소 구조"를 우선한다.

## 제안 아키텍처

### 1. Platform 계층

역할:
- 창 생성/해제
- 메시지 루프
- 입력 수집
- 타이밍

기존 파일:
- [Window.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Window.cpp)
- [Keyboard.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Keyboard.h)
- [Mouse.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Mouse.h)
- [BFTimer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/BFTimer.cpp)

정리 방향:
- `Window`는 Win32 래퍼 역할만 맡긴다.
- 입력 큐 소비 로직은 별도 `InputSystem` 또는 `App` 계층으로 이동한다.

### 2. Graphics Core 계층

역할:
- 디바이스/스왑체인/렌더 타깃/뷰포트 관리
- 프레임 시작/종료
- 공통 그래픽 리소스 생성

추천 클래스:
- `Graphics`
- `Renderer`
- `RenderTarget`
- `DepthStencil`
- `Shader`
- `VertexBuffer`
- `IndexBuffer`
- `ConstantBuffer`

첫 단계에서는 [Graphics.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Graphics.cpp)를 "디바이스 소유자"로 유지하고, 실제 draw 호출은 `Renderer` 쪽으로 분리하는 편이 자연스럽습니다.

### 3. Scene 계층

역할:
- 카메라
- 변환 행렬
- 렌더 가능한 오브젝트 목록
- 업데이트와 렌더 분리

추천 클래스:
- `Transform`
- `Camera`
- `Mesh`
- `SceneNode`
- `Scene`

최소 MVP는 "삼각형 1개 + 카메라 1개 + 월드/뷰/프로젝션 행렬"입니다.

### 4. App 계층

역할:
- 엔진 초기화 순서 조합
- 프레임 업데이트
- 디버그 UI 또는 테스트 씬 선택

[App.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/App.cpp)는 지금처럼 메인 루프를 갖되, 장기적으로는 아래 형태가 좋습니다.

```cpp
while (running)
{
    PumpMessages();
    const float dt = timer.Mark();
    input.BeginFrame();
    scene.Update(dt);
    renderer.BeginFrame(clearColor);
    scene.Render(renderer);
    renderer.EndFrame();
}
```

## 권장 마일스톤

### Milestone 1: 렌더 루프 정리 (완료)

- `Graphics::BeginFrame()` 추가
- `App::DoFrame()`에서 렌더 단계와 업데이트 단계를 분리
- 예외/로그 체계 정리

완료 기준:
- 현재 배경색 애니메이션 유지
- `BeginFrame -> EndFrame` 구조 확립

### Milestone 2: 첫 번째 도형 렌더링 (완료)

- 정점 셰이더/픽셀 셰이더 추가
- 입력 레이아웃, 버텍스 버퍼, 토폴로지 설정
- 삼각형 또는 컬러 쿼드 출력

완료 기준:
- Clear만 하던 화면 대신 실제 지오메트리 1개가 그려짐

### Milestone 3: 카메라와 변환 (완료)

- `Transform`과 행렬 유틸 추가
- 월드/뷰/프로젝션 상수 버퍼 도입
- 객체 이동, 회전 테스트

완료 기준:
- 키보드/마우스로 카메라 또는 오브젝트를 움직일 수 있음

### Milestone 4: 리소스 추상화 (완료)

- 셰이더/버퍼 생성 코드 래핑
- 중복 Direct3D 상태 설정 제거
- 디버그 이름 또는 로그 정보 추가

완료 기준:
- D3D API 호출이 앱 전역에 흩어지지 않고 렌더 계층으로 모임

### Milestone 5: 씬 구조와 자산 진입점 (완료)

- `Scene`과 `Entity` 또는 `SceneNode` 추가
- 간단한 메시 로더 또는 하드코딩 메시 지원
- 향후 텍스처/모델 로딩을 위한 경로 정리

완료 기준:
- 한 프레임에 여러 오브젝트를 관리할 수 있는 구조 확보

## 파일 재구성 제안

초기에는 기존 `Core` 폴더를 크게 흔들지 말고 하위 폴더를 추가하는 편이 안전합니다.

```text
BlueFrog/Core/
  App.*
  Window.*
  Graphics.*
  Input/
    Keyboard.*
    Mouse.*
  Render/
    Renderer.*
    Shader.*
    VertexBuffer.*
    IndexBuffer.*
    ConstantBuffer.*
  Scene/
    Transform.*
    Camera.*
    Mesh.*
    Scene.*
```

## 기술 부채 우선순위

- `Graphics`의 raw COM 포인터를 장기적으로는 스마트 래퍼로 교체
- 렌더 타깃 리사이즈 처리 추가
- `App`의 시간 계산을 `Peek()` 중심에서 `Mark()` 기반 `dt` 갱신으로 전환
- 사용되지 않는 `WindowsMessageMap`의 역할 명확화

## 테스트 전략

- 창 생성 실패, D3D 초기화 실패, `Present()` 실패 시 예외 메시지 확인
- 입력 이벤트 큐가 클릭/휠 좌표를 올바르게 유지하는지 확인
- 렌더 초기화 이후 삼각형 테스트 씬을 자동 회전시켜 기본 렌더 경로 검증

## 다음 방향

초기 마일스톤 1–5는 모두 완료됐다. 이 문서에서 제시했던 `BeginFrame()` 도입, 첫 도형 렌더링, `Renderer` 분리 작업은 이미 커밋되어 있으며, 이후에는 `Engine/Render/` 리소스 래퍼와 `GameplaySimulation` 시스템 추출까지 이어졌다.

이후 작업 방향은 아래 문서를 우선한다.

- Phase 0–3 완료 스냅샷과 Phase 4 예고: [PHASE_0_TO_3_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_0_TO_3_EXECUTION_PLAN.md)
- Phase 4 실행 계획(진행 중): [PHASE_4_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_4_EXECUTION_PLAN.md)
- 장기 컴포넌트/시스템 전환: [GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md)
