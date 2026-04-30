# BlueFrog GameObject-Component Transition Plan

## Status (2026-04-24 기준)

- Phase A (Scene Foundations): 완료 — `Transform`, `SceneObject`, `Scene`이 `Engine/Scene/`에 존재하며 `GameplayArenaBuilder`가 이를 사용한다.
- Phase B (First Components): 완료 — `RenderComponent`, `CollisionComponent`, `CombatComponent`가 optional 컴포넌트로 붙는 구조.
- Phase C (Systems Around Components): 완료 — `GameplayCameraSystem`, `PlayerGameplaySystem`, `EnemyGameplaySystem`, `TriggerGameplaySystem`, `CollisionSystem`, `CombatSystem`이 도입됐고 `GameplaySimulation`이 `SystemContext` 번들을 통해 통일된 시그니처로 시스템을 순차 호출한다. 데이터 주도 registry는 **명시적으로 거부** — 시스템 간 순서 제약(카메라 input → 플레이어 → 적 → 트리거 → 카메라 follow)이 semantic이고, 우선순위 숫자로 매핑되지 않으며, 씬별 토글 유스케이스가 없다. 근거는 `SystemContext.h` 주석.
- Phase D (Serialization): 완료 — Phase 4(씬 로더) → Phase 5(프리팹·다중 씬·로그 트리거·검증기) → Phase 6(이벤트 버스·`ObjectiveState`·씬 전환)까지 layering 완료. 세부는 [PHASE_6_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_6_EXECUTION_PLAN.md).
- Phase F (3D Rendering Capability): 진행 중 — Stage 1, Stage 2가 shipped:
  - **Stage 1** (정적 메시 import): `MeshImporter`가 cgltf 기반으로 .gltf를 파싱하고 `Renderer`가 path 키 기반 import 메시 캐시를 유지. `RenderComponent::meshPath` 추가, arena_trial의 `PillarA`는 hand-authored tetrahedron 메시. cgltf v1.15가 `vendor/cgltf/`에 도입돼 single-TU `cgltf_impl.cpp`로 컴파일 검증.
  - **Stage 2** (skinned mesh + GPU 스키닝, bind pose only): `MeshImporter`가 JOINTS_0 / WEIGHTS_0 / `skin.inverseBindMatrices`까지 추출. 신규 `SkinnedPipeline` (5-attr input layout: POSITION + NORMAL + TEXCOORD + JOINTS u16x4 + WEIGHTS f32x4) + 64-joint matrix palette cbuffer. `Renderer`가 lit / skinned 두 패스로 분리, skinned 자산은 자동 분기. 첫 사례: Khronos `RiggedSimple` 샘플 (2개 박스 + 1 조인트)이 arena_trial에 배치, identity joint matrix로 bind pose 렌더. 셰이더 스키닝 수식이 매 프레임 실행되는 것이 검증돼 Stage 3 애니메이션은 joint matrix 소스만 교체하면 됨.

  후속 Stage(3 애니메이션 클립 재생, 4 애니메이션 상태 머신)는 미착수.

- Phase E (Editor-Oriented Features): 진행 중 — 세 개의 dev-iteration sub-track이 shipped:
  - **디버그 기즈모** (`DebugRenderer`): F1로 토글되는 wireframe 오버레이. 모든 `CollisionComponent`(시안)와 `TriggerComponent`(마젠타)를 XZ-평면 사각형으로, 깊이 테스트 비활성으로 그린다. 다이내믹 라인 VB는 매 프레임 `WRITE_DISCARD`로 재업로드.
  - **Hot-reload** (`App::PollDebugToggles` + `currentScenePath`): F5로 현 씬 JSON을 재로드. `GameplaySimulation::ReloadScene` 기존 인프라 재사용 — `Scene::Clear` + `ObjectiveSystem::Reset` 보장된 풀 리셋. 트리거 구동 씬 전환 직후의 F5는 새 씬을 reload (currentScenePath가 transition 시 갱신).
  - **런타임 인스펙터** (`TextRenderer::RenderInspector`): F2로 토글되는 우측 도킹 패널. 씬 오브젝트 리스트(`[TRCBGE]` 컴포넌트 플래그 + 이름), Tab/Shift+Tab으로 선택 cycle, 선택 객체의 transform/render/collision/combat/behavior/trigger 컴포넌트 전체 덤프. Consolas 모노스페이스 + D2D `FillRectangle` 반투명 배경.
  - **인스펙터 라이브 편집** (`Engine/UI/InspectorFields.h`): PageUp/Down으로 편집 필드 cycle, Left/Right로 1 step 증감, Shift 동시 누름 = ×10. v1 편집 가능 필드는 `position.{x,y,z}` / `rotation.y` / `combat.health`. 편집 중인 필드는 패널에서 노란색 `*` 마커로 하이라이트. SceneObject가 직접 수정되므로 `Renderer`/`CollisionSystem`/`HudPresenter` 모두 즉시 반영. "tools can inspect and modify runtime objects" ✓.
  - **Scene save** (`Engine/Scene/SceneSerializer`): F12로 현 씬을 `currentScenePath`에 v2 스키마로 직렬화. 인스펙터 편집(F2) → save(F12) → reload(F5)로 에디터 round-trip 완성. 프리팹 참조는 v1에서 보존되지 않고 merged form으로 flatten — 후속에서 origin tracking 도입 시 개선. "scenes can be saved and loaded" ✓.

  후속 sub-track(scene inspect CLI → in-engine 객체 생성/배치 → tint·scale·trigger 같은 추가 필드 편집 → 프리팹 참조 보존 save)은 미착수.

## Goal

BlueFrog should move toward a Unity-like authoring and runtime structure gradually, without pausing game progress or rewriting the engine around a full ECS too early.

The target is not "become Unity." The target is:

- reusable runtime object structure
- clear separation between engine systems and game logic
- data that can later be serialized into scenes, prefabs, and save files
- a path toward editor tooling when the runtime is stable

## Why We Are Not Jumping Straight To Full ECS

BlueFrog is still early in renderer, scene, and gameplay foundations. A full ECS now would add complexity before the project has stable needs.

For the current stage, the safest path is:

1. `Transform`
2. `SceneObject`
3. a small set of gameplay-focused components
4. scene serialization
5. lightweight editor/debug tools
6. only then reconsider deeper ECS migration if the game actually needs it

## Migration Principles

- Keep the game playable while the architecture evolves.
- Never rebuild a large subsystem only for abstraction value.
- Introduce engine layers only when a visible phase result uses them.
- Prefer `GameObject + Component` over `type_index` ECS for the near term.
- Make every step compatible with later save/load and editor work.

## Target Runtime Shape

### Phase A: Scene Foundations (완료)

Add a small scene layer that introduces:

- `Transform`
- `SceneObject`
- parent/child transform support later if needed

At this stage, a `SceneObject` can stay very small:

- stable name or id
- `Transform`
- enabled/disabled flag

## Phase B: First Components (완료)

Once the scene exists, add only the components needed for the top-down RPG slice:

- `RenderComponent`
- `CollisionComponent`
- `PlayerComponent`
- `NpcComponent`
- `CameraFollowComponent` or equivalent game-side logic

Avoid generic components that do not power the current game milestone.

## Phase C: Systems Around Components (진행 중)

After a few components exist, move recurring logic into systems:

- render system
- collision system
- player controller system
- npc behavior system

This gives many ECS benefits without forcing a full data-oriented rewrite.

## Phase D: Serialization (미착수)

When object and component shapes settle, add data loading and saving for:

- scene placement
- test maps
- item and npc spawn data
- quest and interaction trigger data

At this point, scene files become the bridge between runtime objects and future editor tooling.

## Phase E: Editor-Oriented Features (미착수)

Only after runtime stability, add features that make the engine feel more Unity-like:

- scene file inspection
- component property editing
- prefab-like reusable object definitions
- debug gizmos
- in-engine placement helpers

## Recommended BlueFrog Order

1. `Transform`
2. `TopDownCamera`
3. test map scene composition
4. player object
5. `RenderComponent`
6. `CollisionComponent`
7. object spawning from simple data files
8. scene save/load
9. debug inspector or editor helpers

## What "Unity-like" Will Mean For BlueFrog

If BlueFrog follows this path, it can gradually become Unity-like in workflow:

- objects exist in a scene
- functionality is attached by components
- data drives object setup
- scenes can be saved and loaded
- tools can inspect and modify runtime objects

That is the correct long-term direction.

What we are not promising yet:

- full editor parity with Unity
- fully generic engine architecture from day one
- large-scale ECS migration before gameplay proves it is needed

## 다음 구현 단계

초기 "Transform / TopDownCamera / 탑다운 테스트 맵" 단계는 이미 완료됐다. 이어지는 실행 순서는 아래와 같다.

- Phase C 마무리(완료): `GameplaySimulation`의 시스템 호출 시그니처를 `SystemContext` 번들로 통일하고, **하드코딩된 순서는 유지**한다는 결정이 기록됐다. 데이터 주도 registry는 거부. 근거: 시스템 간 순서 제약이 semantic(camera input이 player 전, player가 enemy/trigger 전, camera follow가 player 후)이고, 우선순위 숫자로 매핑되지 않으며, 씬별 on/off 유스케이스가 없고, 테스트 이득도 없다. 새 시스템은 `SystemContext&` 한 인자를 받는 `Update`를 구현하고 `GameplaySimulation::Update` 시퀀스에 명시적으로 끼워 넣는다. 상세는 `BlueFrog/Game/Simulation/SystemContext.h` 주석.
- Phase D 진입 전 조건: 컴포넌트 shape 안정화, 씬 오브젝트 이름 기반 조회(`GameplaySceneIds`)를 ID 핸들 기반으로 전환할지 검토한다. 단기적으로는 이름 기반을 유지하고, JSON 씬 파일의 객체명이 `GameplaySceneIds` 상수와 일치하는 것을 계약으로 삼는다.
- Phase D 첫 단계(완료): `GameplayArenaBuilder`의 하드코딩된 아레나를 데이터 파일에서 불러오는 최소 로더로 교체한다. 이 작업은 [PHASE_4_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_4_EXECUTION_PLAN.md) 단계 B-1/B-2에서 완료됐다.
- Phase D (완료): 프리팹 / 다중 씬 / 트리거(로그 전용) / 로더 검증 강화가 [PHASE_5_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_5_EXECUTION_PLAN.md)에서 실현됐다. "item and npc spawn data, quest and interaction trigger data" 중 **spawn data(프리팹)** + **trigger 데이터의 최소 형태(감지+로그)**가 shipped.
- Phase D 확장(완료): 트리거→이벤트 계층(`EventBus`), JSON 구동 `ObjectiveState`, 트리거 액션 기반 씬 전환(`LoadSceneRequested`)이 [PHASE_6_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_6_EXECUTION_PLAN.md)에서 실현됐다. Phase 5의 "로그 전용" 트리거가 실제 소비자(목표 시스템, 씬 reload)를 얻어 "quest and interaction trigger data"의 동작 계층이 shipped. 다음 확장(리스너 모델로의 승격, OR/count-N 조건, 플레이어 상태 이관)은 Phase 7+ 로 연기.
