# BlueFrog GameObject-Component Transition Plan

## Status (2026-04-10 기준)

- Phase A (Scene Foundations): 완료 — `Transform`, `SceneObject`, `Scene`이 `Engine/Scene/`에 존재하며 `GameplayArenaBuilder`가 이를 사용한다.
- Phase B (First Components): 완료 — `RenderComponent`, `CollisionComponent`, `CombatComponent`가 optional 컴포넌트로 붙는 구조.
- Phase C (Systems Around Components): 진행 중 — `GameplayCameraSystem`, `PlayerGameplaySystem`, `EnemyGameplaySystem`, `CollisionSystem`, `CombatSystem`이 도입됐으며 `GameplaySimulation`이 시스템 업데이트 순서를 담당한다.
- Phase D (Serialization): 미착수 — 아레나는 아직 `GameplayArenaBuilder`에서 하드코딩된다.
- Phase E (Editor-Oriented Features): 미착수.

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

- Phase C 마무리: `GameplaySimulation`의 하드코딩된 시스템 순서(카메라 → 플레이어 → 적)를 데이터 주도 등록으로 전환할지 결정한다.
- Phase D 진입 전 조건: 컴포넌트 shape 안정화, 씬 오브젝트 이름 기반 조회(`GameplaySceneIds`)를 ID 핸들 기반으로 전환할지 검토한다.
- Phase D 첫 단계: `GameplayArenaBuilder`의 하드코딩된 아레나를 데이터 파일에서 불러오는 최소 로더로 교체한다.
