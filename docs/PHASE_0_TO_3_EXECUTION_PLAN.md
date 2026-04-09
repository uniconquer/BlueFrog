# BlueFrog Phase 0-3 Detailed Execution Plan

## 목적

이 문서는 BlueFrog를 "DX11 학습용 샘플"에서 "탑다운 판타지 액션 RPG를 만들기 위한 전용 엔진"으로 발전시키기 위한 단기 실행 계획서다. 목표는 Unity 같은 범용 엔진을 흉내 내는 것이 아니라, 현재 목표 게임을 안정적으로 만들 수 있는 런타임과 제작 기반을 먼저 확보하는 것이다.

## 현재 상태

- Win32 창 생성, 메시지 루프, 입력 큐, 타이머 구현 완료
- DX11 디바이스/스왑체인/렌더 타깃 초기화 완료
- `Graphics`와 `Renderer`가 분리된 최소 렌더 구조 확보
- 런타임 셰이더 컴파일 기반 테스트 삼각형 렌더링 가능
- `v143` 기본 빌드와 실행 검증 완료

즉, 지금부터의 핵심은 "렌더러 데모를 엔진 기초로 승격"하는 것이다.

## 제품 방향

- 장르: GTA2 감성의 탑다운 판타지 오픈월드 액션 RPG
- 핵심 경험: 탐험, 전투, 퀘스트, NPC 상호작용, 인벤토리, 마운트
- 시점: 탑다운 카메라가 기본, 자유 카메라는 디버그 전용
- 엔진 목표: 이 게임을 만들 수 있는 전용 엔진
- 비목표: 초기부터 범용 ECS 엔진, 에디터, 절차적 오픈월드, Unity급 툴체인 구축

## Phase 0-3 종료 시 기대 결과

Phase 3이 끝나면 아래가 되어 있어야 한다.

- 탑다운 카메라로 플레이어를 따라다니는 3D 월드가 보인다.
- 플레이어가 이동하고, 마우스 방향을 바라보며, 기본 공격이 가능하다.
- 바닥/벽/충돌체가 있는 작은 테스트 맵이 있다.
- 적 1종이 플레이어를 추적하거나 반응한다.
- 체력 등 최소 HUD가 보인다.

이 수준이 되어야 "엔진이 게임 제작에 들어갔다"고 판단할 수 있다.

## 지금 하지 않을 것

- 범용 ECS 전면 도입
- 절차적 월드 생성
- 대규모 청크 스트리밍
- 복잡한 퀘스트/대화 편집기
- 완전한 모델 임포트 파이프라인
- 범용 UI 툴킷
- 저장/불러오기

이 항목들은 필요 없어서가 아니라, 지금 만들면 게임 코어보다 엔진 자체가 더 커지기 때문이다.

## 공통 개발 원칙

- 게임 우선: 모든 엔진 확장은 Phase 결과물과 직접 연결돼야 한다.
- 탑다운 우선: 자유 카메라는 디버그 도구이며 핵심 플레이 기준이 아니다.
- 데이터 흐름 우선: 클래스 수를 늘리기보다 프레임 흐름과 책임 경계를 먼저 정리한다.
- 검증 우선: 각 단계는 빌드 성공, 실행 성공, 화면 또는 조작 결과로 확인할 수 있어야 한다.
- 보류 명시: 나중으로 미루는 기능은 "삭제"가 아니라 "후순위"로 문서화한다.

## 권장 디렉터리 구조

Phase 0-3 동안은 아래 구조까지만 확장한다.

```text
BlueFrog/
  Core/
    App.*
    Window.*
    Graphics.*
    Renderer.*
    Keyboard.*
    Mouse.*
    BFTimer.*
  Engine/
    Render/
    Scene/
    Camera/
    Math/
    Input/
    Physics/
    UI/
    Debug/
  Game/
    Player/
    Combat/
    NPC/
  Shaders/
  Assets/
  docs/
```

초기에는 `Core`에 있는 기존 코드를 억지로 크게 옮기지 않고, 새 엔진 계층을 `Engine/`부터 채워 나간다.

## Phase 0: 방향 고정과 엔진 기초 재정렬

### 목표

BlueFrog의 범위를 "이 게임을 위한 전용 엔진"으로 고정하고, 앞으로 Phase 1-3에서 흔들리지 않을 구조적 기준을 세운다.

### 결과물

- 현재 문서와 기존 [ENGINE_EXTENSION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/ENGINE_EXTENSION_PLAN.md)가 정리돼 있다.
- `Graphics`는 DX11 기반 계층, `Renderer`는 실제 드로우 계층이라는 책임 분리가 확정된다.
- `Engine/`와 `Game/`의 역할 경계가 정리된다.
- 빌드, 실행, 디렉터리 정책이 문서화된다.

### 세부 작업

1. 디렉터리 정책 확정
- `Core`는 OS/창/디바이스 초기화 중심
- `Engine`은 재사용 가능한 런타임 시스템
- `Game`은 목표 게임 규칙과 콘텐츠 로직

2. 렌더 경계 확정
- `Graphics`는 device, context, swap chain, frame begin/end, viewport, render target 책임만 가진다.
- `Renderer`는 shader, mesh, draw path 같은 실제 렌더링 책임을 가진다.

3. 기술 표준 확정
- 기본 툴셋: `v143`
- 기본 빌드: `Debug|x64`
- 셰이더: 초기에는 런타임 컴파일 허용, 이후 `.cso` 빌드 단계 고려
- 수학: DirectXMath 사용
- 포인터: COM 자원은 장기적으로 `ComPtr` 전환 목표

4. MVP 범위 고정
- 플레이어 1명
- 적 1종
- 무기 1종 또는 기본 근접 공격
- 작은 테스트 맵 1개
- 최소 HUD

### 새로 만들거나 정리할 파일

- [docs/PHASE_0_TO_3_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_0_TO_3_EXECUTION_PLAN.md)
- 필요 시 향후 [docs/ARCHITECTURE.md](/D:/Work/Projects/BlueFrog/docs/ARCHITECTURE.md)

### 완료 기준

- 앞으로 구현할 우선순위와 비우선순위가 문서로 명확하다.
- 이후 작업이 "이 게임용 엔진" 방향을 벗어나지 않는다.

### 검증

- 문서 리뷰로 충분
- 저장소가 `v143` 기본 빌드 상태를 유지

## Phase 1: 렌더링 파이프라인 기초 완성

### 목표

현재 테스트 삼각형 수준의 렌더러를 "3D 오브젝트를 여러 개 그릴 수 있는 기본 파이프라인"으로 승격한다.

### 결과물

- depth stencil이 적용된 3D 렌더링
- indexed draw 기반 큐브 또는 plane 렌더링
- shader, vertex buffer, index buffer, constant buffer가 리소스 단위로 분리
- `Renderer`가 하나 이상의 draw path를 관리

### 세부 작업

1. `Graphics` 강화
- `DepthStencil` 추가
- `OMSetRenderTargets`에 depth 대상 포함
- 리사이즈 대비 viewport 설정 정리
- `GetDevice`, `GetContext` 유지

2. 렌더 리소스 분리
- `Engine/Render/VertexBuffer`
- `Engine/Render/IndexBuffer`
- `Engine/Render/VertexShader`
- `Engine/Render/PixelShader`
- `Engine/Render/InputLayout`
- `Engine/Render/ConstantBuffer`
- `Engine/Render/Topology`

3. Drawable 또는 Mesh 진입 구조 도입
- 지금 단계에서는 범용 `Bindable` 계층을 과도하게 키우지 않는다.
- "정말 필요한 공통 리소스 래퍼"만 만든다.

4. 첫 3D 오브젝트
- 컬러 큐브 1개
- plane 1개
- 월드 행렬 기반 회전/이동

### 추천 파일 구조

```text
Engine/
  Render/
    VertexBuffer.*
    IndexBuffer.*
    VertexShader.*
    PixelShader.*
    InputLayout.*
    ConstantBuffer.*
    Topology.*
    DepthStencil.*
  Scene/
    Transform.*
```

### 보류 항목

- 텍스처
- 조명
- 모델 로딩
- 머티리얼 시스템

### 완료 기준

- 삼각형이 아니라 indexed 큐브가 렌더링된다.
- depth test가 정상 동작한다.
- 렌더 관련 코드가 `Graphics`에 다시 뭉치지 않는다.

### 검증

- `Debug|x64` 빌드 성공
- 큐브와 바닥이 겹칠 때 depth가 올바르게 보인다.
- 프레임 드로우가 `BeginFrame -> Draw -> EndFrame` 흐름으로 유지된다.

## Phase 2: Scene + TopDown Camera + 테스트 맵

### 목표

탑다운 RPG의 기본 화면 구성을 만든다. 즉, "오브젝트를 그린다"에서 "월드를 바라본다"로 넘어간다.

### 결과물

- 탑다운 카메라
- transform 기반 씬 오브젝트 배치
- 바닥/벽/장애물로 구성된 작은 테스트 맵
- 디버그 카메라 또는 와이어프레임 토글은 선택적으로 제공

### 세부 작업

1. 수학/변환 계층
- `Transform`
- `BFMath` 또는 `MathUtils`
- world/view/projection 결합 경로 정리

2. 카메라 시스템
- `TopDownCamera`를 먼저 구현
- 높이, 각도, 줌, 타겟 추적 지원
- 자유 카메라는 디버그 기능으로만 추가 가능

3. 씬 구조
- `SceneObject` 또는 `SceneNode`
- static mesh를 여러 개 월드에 배치
- 큐브, plane, 벽 오브젝트로 미니 맵 구성

4. 렌더 경로 확장
- object별 transform constant buffer 반영
- 카메라 view/projection 행렬 적용

### 추천 파일 구조

```text
Engine/
  Math/
    BFMath.*
  Camera/
    TopDownCamera.*
    DebugCamera.*
  Scene/
    Transform.*
    SceneObject.*
    Scene.*
  Debug/
    RenderDebugFlags.*
```

### 보류 항목

- 텍스처 매핑
- 광원
- 애니메이션
- 절차적 월드

### 완료 기준

- 탑다운 시점으로 큐브/plane 기반 테스트 맵이 보인다.
- 카메라 줌 또는 위치 조정이 가능하다.
- 월드 오브젝트가 카메라 기준으로 올바르게 렌더링된다.

### 검증

- `F1` 또는 별도 디버그 키로 카메라 모드 변경 가능 여부는 선택
- 마우스 휠 줌 또는 고정 각도 추적 확인
- 바닥, 벽, 플레이어 더미 오브젝트가 탑다운 구도에서 읽기 쉬운지 확인

## Phase 3: 플레이어 수직 슬라이스

### 목표

엔진이 아니라 "게임이 시작됐다"는 감각을 만드는 첫 수직 슬라이스를 구현한다.

### 결과물

- 플레이어 이동
- 마우스 방향 바라보기
- 기본 공격
- 충돌
- 적 1종의 간단한 추적 반응
- 최소 HUD

### 세부 작업

1. 입력 매핑
- `WASD` 이동
- 마우스 월드 포인팅
- 공격 입력

2. 플레이어 로직
- `PlayerController`
- 이동 속도, 회전, 입력 반영
- 상태값: HP, 이동 속도, 공격 쿨다운

3. 충돌 기초
- 2.5D 기준 충돌
- AABB 또는 capsule-lite 단순 해법
- 벽 슬라이딩

4. 적 반응
- 적 1종
- 플레이어 추적 또는 일정 거리에서 반응
- 데미지 처리까지는 단순화 가능

5. 최소 UI
- 체력 바
- 간단한 크로스헤어 또는 선택 표시
- 디버그 텍스트 가능

### 추천 파일 구조

```text
Engine/
  Input/
    InputMapper.*
  Physics/
    AABB.*
    CollisionSystem.*
  UI/
    UIRenderer.*
    HealthBar.*
Game/
  Player/
    PlayerController.*
  Combat/
    CombatSystem.*
  NPC/
    SimpleEnemyController.*
```

### 보류 항목

- 퀘스트
- 인벤토리
- 대화 시스템
- 마운트
- 정식 애니메이션

### 완료 기준

- 플레이어가 탑다운 맵 위에서 조작된다.
- 마우스 방향으로 캐릭터가 바라본다.
- 공격 입력이 적 또는 더미 대상에 반영된다.
- 벽/장애물 충돌이 작동한다.
- 체력 UI가 화면에 표시된다.

### 검증

- `Debug|x64` 빌드 성공
- 실행 후 플레이어 이동/회전/공격 확인
- 벽 끼임 없이 충돌 동작 확인
- 적 추적 또는 피격 반응 확인

## Phase별 체크리스트

### Phase 0 체크리스트

- [ ] 디렉터리 역할 문서화
- [ ] MVP 범위 고정
- [ ] 후순위 기능 명시

### Phase 1 체크리스트

- [ ] depth stencil
- [ ] indexed cube/plane
- [ ] render resource 래퍼 분리

### Phase 2 체크리스트

- [ ] top-down camera
- [ ] scene object 배치
- [ ] 테스트 맵 구성

### Phase 3 체크리스트

- [ ] player controller
- [ ] basic combat
- [ ] collision
- [ ] simple enemy
- [ ] HUD

## 다음 단계 예고

Phase 3이 끝난 뒤에야 아래를 본격적으로 시작한다.

- 텍스처와 머티리얼
- 조명
- 모델 로딩
- 애니메이션
- 퀘스트/대화/인벤토리
- 마운트
- 저장/불러오기

즉, 지금의 정답은 "기술 데모를 늘리는 것"이 아니라 "플레이 가능한 작은 탑다운 액션 RPG 조각"을 가능한 빨리 만드는 것이다.
