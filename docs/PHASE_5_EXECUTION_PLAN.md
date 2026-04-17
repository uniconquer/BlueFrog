# BlueFrog Phase 5 Execution Plan

> Phase 4 완료 이후 데이터/툴링 계층을 확장하는 단기 실행 계획서. 이 문서는 [GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md)의 Phase D — "item and npc spawn data, quest and interaction trigger data" 항목을 최소 단위로 실현한다. [PHASE_4_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_4_EXECUTION_PLAN.md)가 깐 JSON 씬 로더 위에 프리팹·다중 씬·트리거를 얹는다.

## Context

Phase 4가 완료됐다 (commits `9b5b3ee..838f1cd`). 아레나는 JSON 파일([Assets/Scenes/arena_trial.json](/D:/Work/Projects/BlueFrog/BlueFrog/Assets/Scenes/arena_trial.json))에서 로드되고, 모든 객체는 `Material` 기반 단일 파이프라인을 타며, 디렉셔널 라이트도 활성화됐다. [GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h)는 15줄 wrapper로 축소됐다.

하지만 데이터 계층은 여전히 **한 개의 하드코딩된 씬**에 국한돼 있다.

- 씬 파일 하나뿐 — 로더가 특정 파일명에 커플링돼 있어도 알 방법이 없음.
- Player, EnemyScout, 4개의 벽이 **모두 인라인으로 완전 기술**돼 있어 재사용 불가. 적 종류를 늘리거나 같은 벽 템플릿을 다른 씬에 쓰려면 복붙 외 방법이 없다.
- 트리거/상호작용 개념이 전무 — 플레이어가 특정 영역에 들어갔을 때 이벤트를 발생시킬 수단이 없다. 이후 퀘스트/다이얼로그 시스템의 전제 조건.
- [SceneLoader](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp)::Load 실패 시 `fprintf(stderr, ...)` 한 줄로 끝. 여러 씬/여러 프리팹을 동시에 관리하기 시작하면 이 에러 경로가 고통의 원인이 된다.

Phase 5의 목표는 "유니티 workflow"의 다음 핵심 요소인 **프리팹 + 다중 씬 + 트리거**를 도입하되, 각각은 현재 게임에 실제 소비자가 있는 최소 형태로만 들어가게 하는 것이다.

## 스코프 결정

세 가지 안을 검토했다.

| 안 | 내용 | 기각 사유 |
|---|---|---|
| A | 프리팹 + 스포너 + 트리거 + 두 번째 씬 + hot-reload | 과도. 스포너는 프리팹과 기능 중복, hot-reload는 파일 워처 디바운스만으로 반나절, 트리거 소비자 없으면 dead code |
| B | 프리팹만 깊게 (상속 체인, 배열 merge, override path) | 조숙함. 현재 프리팹 가치가 있는 객체는 Player/EnemyScout 2개뿐. 깊은 merge 규칙은 실제 프리팹 5개 이상이 쌓인 뒤에 정한다 |
| **C (선택)** | **최소 프리팹 + 두 번째 씬 + 로그-전용 트리거 + 씬 선택 + 로더 검증 강화** | 모든 기능에 실제 소비자가 존재. 시스템 하나도 "읽는 곳 없음" 상태로 들어가지 않음 |

**Phase 6 이후로 명시 연기**: 스포너 추상화, hot-reload, 프리팹 상속 체인, 트리거 퇴장 이벤트, 퀘스트 상태 머신, 다이얼로그, 인벤토리, 루트.

### 핵심 설계 결정

| 항목 | 선택 | 근거 |
|---|---|---|
| 프리팹 override 의미 | **컴포넌트 단위 shallow override** | 씬이 `transform`을 명시하면 프리팹의 `transform` 통째로 교체. 컴포넌트 내부 부분 override(`combat.maxHp`만 바꾸고 `combat.faction`은 유지)는 YAGNI. 필요해지면 그때 deep merge로 승격 |
| 프리팹 참조 방식 | **JSON에서 `"prefab": "Assets/Prefabs/X.prefab.json"` 키** | 이름 레지스트리 없이 경로 직접. 레지스트리는 프리팹 수가 늘면 추가 |
| 씬 선택 | **명령줄 인자 `--scene <path>`** + 기본값 `arena_trial.json` | 빌드 변경 없이 두 번째 씬 진입. ENV var보다 IDE 디버거에서 설정하기 쉬움 |
| 트리거 첫 버전 | **XZ axis-aligned box + `OutputDebugStringA` 로그 한 줄** | 소비자를 "로그 자체"로 둠. 퀘스트/이벤트 dispatcher는 Phase 6 |
| 스키마 버전 | **v1 → v2로 bump**. v2 로더는 v1도 수용 | v1 파일은 구조적으로 v2의 부분집합 (prefab/trigger는 optional). 역호환은 설계로 확보 |
| 에러 경로 | **startup validator**가 모든 씬/프리팹을 dry-run 로드 | 런타임 중 씬 전환 시 깨진 JSON이 튀어나오면 추적이 어려움. 시작 시점에 모두 확인 |

## 실행 순서 (5 commits)

각 단계는 빌드·실행 가능한 경계. `Debug|x64`/`v143` 전제.

### 단계 A-1 — 최소 프리팹 로더 (shallow override)

**신규 파일**

- `BlueFrog/Engine/Scene/PrefabLoader.h/.cpp`
  - `static bool LoadAndMerge(const std::filesystem::path& prefabPath, nlohmann::json& targetObj, std::string* errorOut);`
  - 프리팹 JSON을 읽어 `Load` 호출 스코프 한정 캐시 맵에 보존, `targetObj`의 누락된 최상위 키(`transform`/`render`/`collision`/`combat`)를 프리팹 값으로 채움. 이미 존재하는 키는 **씬 쪽이 승**.
  - 프리팹 JSON은 `schemaVersion` 없이 객체 정의 본체만 (`name` 키는 무시 — 씬의 `name`이 정본).
- `BlueFrog/Assets/Prefabs/Player.prefab.json`
- `BlueFrog/Assets/Prefabs/EnemyScout.prefab.json`

**수정**

- [BlueFrog/Engine/Scene/SceneLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp)
  - `schemaVersion` 체크를 `v == 1 || v == 2` 허용으로 확장.
  - 객체 파싱 loop 진입 직후, `objJson.contains("prefab")`이면 `PrefabLoader::LoadAndMerge`를 거쳐 병합된 `json` 객체를 기존 파서에 넘김.
  - 프리팹 캐시는 `SceneLoader::Load` 호출당 local (씬 간 오염 방지).
- [BlueFrog/Assets/Scenes/arena_trial.json](/D:/Work/Projects/BlueFrog/BlueFrog/Assets/Scenes/arena_trial.json)
  - `schemaVersion` → 2.
  - `Player`, `EnemyScout` 객체를 `{ "name": "Player", "prefab": "Assets/Prefabs/Player.prefab.json", "transform": { ... } }` 형태로 교체.
  - 나머지 8개 객체(Ground, ShrineCore, 4 walls, 2 pillars)는 이 단계에서 그대로.

**vcxproj 갱신**: `PrefabLoader.h/.cpp`, `Assets/Prefabs/*.prefab.json` 등록.

**완료 기준**: 게임이 Phase 4 종료 시점과 **시각적·기능적으로 동일**하게 작동. Player.prefab.json의 tint를 임시로 `[1,0,0]`로 편집해 재실행 → 플레이어가 빨강. `arena_trial.json`에서 Player에 `"render": { "material": { "tint": [0,1,0] } }`를 추가하면 씬 override가 이김(초록). 원상복구.

### 단계 A-2 — 두 번째 씬 + `--scene` 인자

**신규 파일**

- `BlueFrog/Assets/Scenes/sparring_yard.json`
  - 더 작은 아레나 (Ground 10×10, 벽 4개, EnemyScout 2마리를 프리팹에서 인스턴스화해 서로 다른 위치에).
  - Player 포함 (이름 `GameplaySceneIds::Player`와 반드시 일치).
  - `schemaVersion: 2`.

**수정**

- [BlueFrog/Core/App.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/App.cpp) (또는 메인 엔트리) — `int argc, char** argv`에서 `--scene <path>` 파싱해 `App` 멤버로 보관. 없으면 `"Assets/Scenes/arena_trial.json"` 기본값.
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h) — `Build(Scene&, TopDownCamera&, const std::string& scenePath)` 시그니처로 확장. 호출부에서 선택된 경로 주입.
- BlueFrog.vcxproj — `sparring_yard.json` 추가.

**완료 기준**: 인자 없이 실행 → arena_trial 유지. Visual Studio 디버거의 Command Arguments에 `--scene Assets/Scenes/sparring_yard.json` 설정 후 F5 → sparring_yard 로드, 적 2마리 추적 동작, 플레이어 조작 정상.

### 단계 A-3 — TriggerComponent + TriggerGameplaySystem (로그 전용)

**신규 파일**

- `BlueFrog/Engine/Scene/TriggerComponent.h`

  ```cpp
  struct TriggerComponent {
      DirectX::XMFLOAT2 halfExtents{1.0f, 1.0f};  // XZ axis-aligned
      std::string tag;
      bool fireOnce = true;
      bool fired = false;   // runtime state
  };
  ```

- `BlueFrog/Game/Simulation/TriggerGameplaySystem.h/.cpp`
  - 매 틱 `Scene`을 순회하며 `trigger` 컴포넌트가 있는 객체와 `GameplaySceneIds::Player`의 위치를 비교.
  - 최초 진입 시 `OutputDebugStringA` + `std::cout`으로 `"[Trigger] Player entered '<tag>' (object='<name>')\n"` 출력.
  - `fireOnce == true`이면 재진입 시 무시. 퇴장 감지는 Phase 6.

**수정**

- [BlueFrog/Engine/Scene/SceneObject.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneObject.h) — `std::optional<TriggerComponent> trigger;` 추가.
- [BlueFrog/Engine/Scene/SceneLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp) — `"trigger": { "halfExtents":[x,z], "tag":"...", "fireOnce":true }` 파서 추가.
- [BlueFrog/Game/Simulation/GameplaySimulation.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplaySimulation.cpp) — 플레이어 이동 이후 단계에 `TriggerGameplaySystem::Update` 호출 추가.
- `BlueFrog/Assets/Scenes/sparring_yard.json` — `TriggerZone_Center` 객체 추가 (render/collision 없이 `transform` + `trigger`만, Ground 중앙 근처).

**완료 기준**: sparring_yard에서 플레이어가 해당 영역에 들어가면 로그 1회 출력. 그 자리에서 벗어났다가 재진입해도 추가 출력 없음. arena_trial(트리거 없음)은 아무 로그 없이 이전과 동일.

### 단계 A-4 — ArenaWall 프리팹 + 스키마 문서화

**신규 파일**

- `BlueFrog/Assets/Prefabs/ArenaWall.prefab.json` — 4개 벽이 공유하던 `render.material` + `collision` 템플릿.
- `docs/SCENE_SCHEMA.md` — v2 스키마 필드 목록, prefab 참조 의미(shallow override), trigger 필드 설명. 한 페이지 분량.

**수정**

- [BlueFrog/Assets/Scenes/arena_trial.json](/D:/Work/Projects/BlueFrog/BlueFrog/Assets/Scenes/arena_trial.json) — 4개 벽을 `{ "name":"NorthWall", "prefab":"Assets/Prefabs/ArenaWall.prefab.json", "transform":{...} }`로 축소. 2개 Pillar도 동일한 변환이 유익하면 `Pillar.prefab.json` 추가 여부 판단 (tint가 다르면 별도 프리팹 가치가 없을 수 있음 — 4 walls만으로 가치 증명되면 충분).
- `BlueFrog/Assets/Scenes/sparring_yard.json` — 동일 프리팹으로 벽 정리.

**완료 기준**: 두 씬 모두 시각적으로 이전과 동일하게 렌더링. `arena_trial.json` 라인 수가 30% 이상 감소.

### 단계 B-1 — Startup validator + 에러 메시지 포맷 강화

**수정**

- [BlueFrog/Engine/Scene/SceneLoader.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.h) / [SceneLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp)
  - 에러 메시지에 파일 경로 + JSON pointer 형태 위치 포함: `"sparring_yard.json: objects[3].trigger.halfExtents: expected array of 2 numbers"`.
  - `SceneLoader::Validate(const std::filesystem::path& path, std::string* errorOut)` 엔트리 포인트 추가 — 파일을 파싱·검증하되 `Scene`/`TopDownCamera`에 쓰지 않음. 프리팹 참조 유효성도 여기서 확인.
- [BlueFrog/Core/App.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/App.cpp) 또는 초기화 지점 — 창 생성 전에 `Assets/Scenes/*.json` 및 `Assets/Prefabs/*.prefab.json`을 순회하며 `Validate`. 실패 시 콘솔 에러 + `MessageBoxA` 후 종료.

**완료 기준**: 고의로 프리팹 JSON을 깨뜨려 실행 → 창 뜨기 전에 명확한 메시지 + 종료. 정상 상태로 원복 → 정상 기동.

## Critical Files

### 읽기/참조 (기존, 재사용)

- [BlueFrog/Engine/Scene/SceneLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp) — v1 파서. v2 확장의 기반.
- [BlueFrog/Engine/Scene/SceneObject.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneObject.h) — trigger optional 추가 지점.
- [BlueFrog/Engine/Scene/Scene.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/Scene.h) — `CreateObject`, `FindObject`, `Clear`.
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h) — A-2에서 시그니처 확장.
- [BlueFrog/Game/Simulation/GameplaySimulation.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplaySimulation.cpp) — 시스템 업데이트 순서 등록 지점.
- [BlueFrog/Game/Simulation/GameplaySceneIds.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplaySceneIds.h) — Player/EnemyScout 이름 계약.
- `BlueFrog/vendor/nlohmann/json.hpp` — 이미 존재, PrefabLoader에서 재사용 (include site는 가급적 `.cpp`만).

### 신규

- `BlueFrog/Engine/Scene/PrefabLoader.h/.cpp`
- `BlueFrog/Engine/Scene/TriggerComponent.h`
- `BlueFrog/Game/Simulation/TriggerGameplaySystem.h/.cpp`
- `BlueFrog/Assets/Prefabs/Player.prefab.json`
- `BlueFrog/Assets/Prefabs/EnemyScout.prefab.json`
- `BlueFrog/Assets/Prefabs/ArenaWall.prefab.json`
- `BlueFrog/Assets/Scenes/sparring_yard.json`
- `docs/SCENE_SCHEMA.md`

### 수정 (중심축)

- [BlueFrog/Engine/Scene/SceneLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp) — A-1 (prefab), A-3 (trigger), B-1 (validator/에러 포맷).
- [BlueFrog/Engine/Scene/SceneObject.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneObject.h) — A-3 (trigger 필드).
- [BlueFrog/Core/App.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/App.cpp) — A-2 (--scene 인자), B-1 (startup validator).
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h) — A-2 (scenePath 주입).
- [BlueFrog/Game/Simulation/GameplaySimulation.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplaySimulation.cpp) — A-3 (시스템 등록).
- [BlueFrog/Assets/Scenes/arena_trial.json](/D:/Work/Projects/BlueFrog/BlueFrog/Assets/Scenes/arena_trial.json) — A-1 (Player/Enemy prefab 참조), A-4 (벽 프리팹).
- BlueFrog.vcxproj + `.filters` — 각 단계에서 신규 파일/자산 등록.

### 삭제

없음. 모든 변경은 추가·대체 형태.

## Verification (end-to-end)

Phase 5 완료 판정은 아래 **모두** 만족 시.

1. **빌드**: `Debug|x64`, `v143`, 경고 0개 증분.
2. **Phase 4 회귀**: `--scene` 인자 없이 실행 → arena_trial이 Phase 4 종료 시점과 시각적으로 동일. 모든 기존 기능(이동/회전/공격/충돌/적 추적/HUD) regression 없음.
3. **프리팹 override**: `Player.prefab.json`의 tint 편집 → 재빌드 없이 적용. 씬에서 override 추가 → 씬 값이 승.
4. **두 번째 씬**: `--scene Assets/Scenes/sparring_yard.json` → 다른 레이아웃, 적 2마리, 플레이어 조작 정상.
5. **트리거 로그**: sparring_yard에서 지정 영역 최초 진입 시 Visual Studio Output 창에 정확히 1회 로그. 재진입 시 무음.
6. **스키마 v2 역호환**: 기존 v1 `arena_trial.json`(A-1 이전 스냅샷)도 수용된다는 것을 로더 유닛 수준에서 확인.
7. **프리팹 축소 효과**: A-4 후 `arena_trial.json` 라인 수가 30% 이상 감소.
8. **Startup validator**: 프리팹 JSON을 고의로 깨뜨리면 창 생성 전에 명확한 에러(`파일명: path: 사유`)로 종료. 정상 상태에서는 무음.
9. **객체 이름 안정성**: `GameplaySceneIds::Player`/`EnemyScout` 조회가 두 씬 모두에서 동작.

## 위험과 롤백

- **프리팹 경로 상대 해석**: 씬 JSON이 `Assets/Prefabs/X.prefab.json`을 지정할 때, 로더가 **CWD 기준**으로 연다 (기존 `arena_trial.json`도 그렇게 동작 중). 이 규칙을 문서화.
- **프리팹 순환 참조**: 프리팹이 다른 프리팹을 참조하는 기능은 Phase 5에 포함하지 않는다 (A-1의 merge는 1단계만). 순환 감지 코드도 불필요.
- **트리거 AABB vs Player 충돌 축**: `CollisionSystem`과 별개의 검사. 트리거는 "통과 가능한 영역"이므로 `blocking=false` 의미. 충돌 시스템을 건드리지 않는다.
- **명령줄 인자 파싱**: 가장 얇은 파싱 (단일 키-값, 순서 의존). 본격적인 CLI 파서는 도입하지 않는다.
- **롤백 포인트**: 각 단계 독립 커밋. A-1(프리팹)이 가장 큰 개념 변경이므로 이 직후에 `phase5-a1-prefabs` 태그 권장.
- **중단 안전성**: A-3 완료 + A-4 미완료 상태 = "두 씬 + 프리팹 2종 + 로그 트리거, 벽은 아직 인라인". 기능적으로 완결이며 언제든 재개 가능.

## 진행 상태

- [ ] A-1 프리팹 로더 + Player/EnemyScout 프리팹
- [ ] A-2 두 번째 씬 + `--scene` 인자
- [ ] A-3 TriggerComponent + TriggerGameplaySystem (로그 전용)
- [ ] A-4 ArenaWall 프리팹 + 스키마 문서화
- [ ] B-1 Startup validator + 에러 메시지 포맷 강화
