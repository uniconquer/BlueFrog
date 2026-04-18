# BlueFrog Phase 6 Execution Plan — "Triggers → Events → Objectives"

> 선행 문서: [PHASE_5_EXECUTION_PLAN.md](PHASE_5_EXECUTION_PLAN.md)

## Context

Phase 5가 완료됐다 (`8fd23c0..0717f59`). 프리팹, 두 번째 씬, `--scene` CLI, TriggerComponent(로그 전용), 시작 시 asset validator가 shipped. 하지만 데이터 계층의 다음 비대칭이 드러났다:

- **트리거가 로그만 찍는다.** 소비자 없는 시스템이 하나 남았다. Phase 5에서 "로그 자체가 소비자"로 합의했지만, 이 대출증서를 갚아야 다음 시스템들이 트리거 위에 안전하게 쌓인다.
- **타이틀바 목표 텍스트 `"Defeat the scout"`가 완전히 정적**(`HudPresenter.h:31`에서 하드코딩). 적을 모두 처치해도 `hasTarget`이 false가 되면 `"Arena cleared"`로 바뀔 뿐, 씬별 다른 목표/여러 조건 / 달성 감각이 없다.
- **두 씬이 있지만 플레이 중 이동 수단이 없다.** CLI로 따로 띄워야 한다.
- **이벤트 인프라가 전무하다.** 저장소 전체에 EventBus / Observer / pub-sub이 0개. Greenfield.

Phase 6의 목표는 **"데이터 계층의 다음 단계 — 동작 간 이벤트 연결"**. 트리거가 이벤트를 발행하고, 전투가 킬 이벤트를 발행하고, JSON 구동 `ObjectiveState`가 이 이벤트를 소비해 타이틀바 목표 텍스트를 동적으로 갱신하고, 마지막으로 트리거 액션으로 씬 전환이 된다. 각 시스템은 **실제 소비자를 가진 최소 형태**로만 들어간다 — Phase 5 철학 유지.

## 스코프 결정

세 가지 안을 검토:

| 안 | 내용 | 기각/채택 사유 |
|---|---|---|
| α (채택) | **Events + Objective + Scene transition** — 트리거→이벤트→목표→씬 로드까지 최소 단위로 | 트리거 dead-code 해소, 목표 텍스트에 즉시 가치 체감, 다음 페이즈(다이얼로그/퀘스트 확장)가 올라탈 인프라 |
| β | In-viewport HUD (DirectWrite 오버레이) | 데이터 모델은 이미 완성 → 페이즈 한 덩어리 아님. α 중/후 독립 커밋으로 편입 가능 |
| γ | Behavior as data (AI/전투 수치 JSON화) | 적 종류 2개로는 레버리지 부족. Phase 7+ |

**Phase 7+ 명시 연기:** 이벤트 pub/sub 리스너 모델로 승격, 다이얼로그, 인벤토리, 퀘스트 상태 머신, 조건식 OR/count-N, 세이브/로드, 플레이어 상태 이관.

### 핵심 설계 결정

| 항목 | 선택 | 근거 |
|---|---|---|
| 이벤트 전달 방식 | **tick-drain vector** (`std::vector<GameEvent>` + Drain) | 4 시스템, 3 소비자, 단일 스레드. listener registry는 YAGNI. 소비자가 재발행하려는 욕구는 v1에서 `assert`로 금지 |
| 이벤트 표현 | **Tagged POD** (enum + 두 개 string) | 이벤트 타입 3종. `std::variant`는 `<variant>` 헤더와 `std::visit` 의식을 강제 — 가치 없음 |
| 씬 전환 요청 | **`LoadSceneRequested` 이벤트로 통합** (별도 채널 X) | 단일 큐 = 단일 source of truth |
| Objective 위치 | **씬 JSON 내부 `"objective"` top-level 키** | 씬↔목표 1:1. 파일 분리는 I/O 중복 + "씬 OK/objective FAIL" 분기만 생김 |
| Objective 조건 결합 | **AND만** (`conditions` 배열) | 현재 두 씬 모두 AND로 충분. 향후 `"mode": "any"` 추가는 5줄 변경 |
| Objective 텍스트 할당 지점 | **`GameplaySimulation::Update` 말미에 `hudState.objectiveText`에 주입** | HudPresenter는 "dumb"하게 유지. 시그니처 확장 피함 |
| 스키마 버전 | **v2 유지**. `objective`·`action`은 모두 optional | v2-without-them이 여전히 valid. 역호환은 설계로 확보 |
| 씬 전환 시 상태 이관 | **없음. 전체 리셋** | 세이브 인프라 부재 → 보존 계약 맺을 이유 없음. 문서화해서 향후 혼선 방지 |
| 킬 이벤트 발행 타이밍 | **`CombatSystem::ApplyDamage` 내부**에서 `wasAlive && !IsAlive()` 가드 | 적이 dead-but-in-scene 상태로 남아있으므로 post-scan 발행은 매 틱 중복 발행됨 |
| Scene reload 책임 | **`GameplaySimulation::ReloadScene(path, scene, camera)`로 단일화** | App이 BuildArena만 호출하고 ObjectiveSystem::Reset을 따로 호출하면 책임 분열. 하나의 함수가 양쪽 다 처리 |
| HUD 1-frame 스테일 방지 | **reload 직후 `BuildHudState` 재호출** | App 생성자가 최초 BuildArena 후에 BuildHudState를 호출하는 패턴과 동일 |

## 실행 순서 (5 commits)

각 단계는 빌드·실행 가능한 경계. `Debug|x64` / `v143` 전제.

---

### A-1 — EventBus 뼈대 (컴파일 OK, 동작 무변화)

**신규**
- `BlueFrog/Engine/Events/GameEvent.h`
  ```cpp
  enum class GameEventType { EnemyKilled, TriggerFired, LoadSceneRequested };
  struct GameEvent {
      GameEventType type;
      std::string a;  // name / tag / path
      std::string b;  // optional payload (trigger param, etc.)
  };
  ```
- `BlueFrog/Engine/Events/EventBus.h/.cpp`
  ```cpp
  class EventBus {
  public:
      void Publish(GameEvent);
      std::vector<GameEvent> Drain();  // move-out + reset
  private:
      std::vector<GameEvent> queue_;
      bool draining_ = false;  // assert guard against re-entrant publish
  };
  ```

**수정**
- `BlueFrog/Game/Simulation/GameplaySimulation.h/.cpp` — `EventBus eventBus;` 멤버 추가. 발행자/소비자는 아직 없음.
- `BlueFrog/BlueFrog.vcxproj` + `.filters` — 새 파일 등록, `Engine\Events` 필터 추가.

**완료 기준**: 빌드 clean, 게임이 Phase 5와 시각·기능적으로 **동일**. EventBus는 dead code.

**롤백**: 2개 파일 + `eventBus` 멤버만 제거.

---

### A-2 — Publisher 배선 + 임시 디버그 drain

**수정**
- `BlueFrog/Game/Combat/CombatSystem.h/.cpp` — `ApplyDamage` / `TryMeleeAttack`에 `EventBus*` optional param 추가. `ApplyDamage` 내부:
  ```cpp
  const bool wasAlive = target.combatComponent->IsAlive();
  target.combatComponent->health = std::max(0, ... - damage);
  if (bus && wasAlive && !target.combatComponent->IsAlive())
      bus->Publish({ GameEventType::EnemyKilled, target.name, "" });
  ```
  (플레이어/적 구분 없이 발행. 소비자가 이름으로 필터.)
- `BlueFrog/Game/Player/PlayerController.cpp` (호출 체인: `PlayerGameplaySystem::Update` → `PlayerController::Update` → `TryMeleeAttack`) — EventBus 스레딩.
- `BlueFrog/Game/Simulation/TriggerGameplaySystem.h/.cpp` — `Update(Scene&, EventBus&)`로 시그니처 확장. 진입 시 `bus.Publish({ TriggerFired, tc.tag, obj.name })`. 기존 로그는 유지(트리거 action이 없거나 `"log"`인 경우).
- `BlueFrog/Game/Simulation/GameplaySimulation.cpp::Update` — player/trigger 시스템에 `eventBus` 전달. **임시**: 말미에 drain → 각 이벤트를 `OutputDebugStringA`로 찍고 버림.

**완료 기준**:
- 적 처치 → `[Event] EnemyKilled EnemyScout` 출력 1회.
- 트리거 진입 → `[Event] TriggerFired center_zone TriggerZone_Center` 출력 1회.
- 재진입/이미 죽은 적 재타격 → 무음.

**롤백**: 단일 커밋 revert.

---

### A-3 — ObjectiveState + ObjectiveSystem + HUD 동적화

**신규**
- `BlueFrog/Game/Objectives/ObjectiveState.h`
  ```cpp
  struct ObjectiveCondition { std::string type; std::string name; bool met = false; };
  struct ObjectiveState {
      std::wstring text;
      std::wstring completionText;
      std::vector<ObjectiveCondition> conditions;
      bool IsComplete() const;
  };
  ```
- `BlueFrog/Game/Objectives/ObjectiveSystem.h/.cpp`
  ```cpp
  class ObjectiveSystem {
  public:
      void Reset(ObjectiveState);
      void Consume(const std::vector<GameEvent>&);
      std::wstring CurrentText() const;
  private:
      ObjectiveState state_;
  };
  ```
  `Consume`: `EnemyKilled { name }`이 오면 `condition.type == "enemy_killed" && condition.name == name`인 항목을 `met=true`로. v1은 이 조건 타입 하나만 지원. 향후 `trigger_fired` 확장.

**수정**
- `BlueFrog/Engine/Scene/SceneLoader.h/.cpp`
  - `Load(...)`에 `ObjectiveState* objectiveOut = nullptr` 파라미터 추가.
  - root의 top-level `"objective"` 키 파싱 → `{ text, completionText, conditions: [{type, name}, ...] }`.
  - `Validate`: `objective` 있으면 조건 타입 검사 (현재는 `"enemy_killed"`만 allow), 오탈자 reject with path-prefixed error.
- `BlueFrog/Game/Simulation/GameplayArenaBuilder.h` — `Build` 시그니처에 `ObjectiveState&` out-param 추가 + 내부에서 SceneLoader에 전달.
- `BlueFrog/Game/Simulation/GameplaySimulation.h/.cpp`
  - `ObjectiveSystem objectiveSystem;` 멤버.
  - `BuildArena` → `GameplayArenaBuilder::Build`에서 받은 `ObjectiveState`를 `objectiveSystem.Reset()`에 전달.
  - `Update` 말미: A-2의 임시 debug drain **제거**. 대신:
    ```cpp
    auto events = eventBus.Drain();
    objectiveSystem.Consume(events);
    auto hud = playerSystem.BuildHudState(scene);
    hud.objectiveText = objectiveSystem.CurrentText();
    return hud;
    ```
- `BlueFrog/Game/Hud/HudPresenter.h` — **삭제**: `hudState.objectiveText = hasTarget ? L"Defeat the scout" : L"Arena cleared";` (line 31). Simulation이 post-Build로 주입.
- `BlueFrog/Assets/Scenes/arena_trial.json` — root에 추가:
  ```json
  "objective": {
    "text": "Defeat the scout",
    "completionText": "Arena cleared",
    "conditions": [{ "type": "enemy_killed", "name": "EnemyScout" }]
  }
  ```
- `BlueFrog/Assets/Scenes/sparring_yard.json` — 동일 구조, `text: "Defeat both scouts"` + `completionText: "Yard cleared"` + 2개 condition (`EnemyScout`, `EnemyScoutDummy`).

**완료 기준**:
- arena_trial에서 스카우트 처치 → 타이틀 `"Defeat the scout"` → `"Arena cleared"`로 전환.
- sparring_yard에서 한 마리만 처치 → 텍스트 변화 없음. 두 번째도 처치 → `"Yard cleared"`.
- `hasTarget` 기반 하드코딩 완전 제거.

**롤백**: 단일 커밋 revert. HudPresenter 하드코딩 부활.

---

### A-4 — Trigger 액션 + 씬 전환

**수정**
- `BlueFrog/Engine/Scene/TriggerComponent.h` — `std::optional<TriggerAction> action;` 추가:
  ```cpp
  struct TriggerAction { std::string type; std::string param; };
  ```
- `BlueFrog/Engine/Scene/SceneLoader.cpp::ParseTrigger` — optional `"action": { "type": "...", "param": "..." }` 파싱. `Validate`: type이 `log`/`publish`/`load_scene`이 아니면 reject.
- `BlueFrog/Game/Simulation/TriggerGameplaySystem.cpp::Update` — 발화 시 분기:
  - `action` 없음 또는 `type == "log"`: 기존 로그 그대로.
  - `type == "publish"`: `bus.Publish({ TriggerFired, tc.tag, action.param })` + 로그.
  - `type == "load_scene"`: `bus.Publish({ LoadSceneRequested, action.param, "" })`. (로그는 선택 — 시끄럽지 않도록 info 수준.)
- `BlueFrog/Game/Simulation/GameplaySimulation.h/.cpp`
  - `Update`에서 `events = eventBus.Drain()` 이후, events를 **먼저 스캔**해 `LoadSceneRequested`를 `std::optional<std::string> pendingSceneLoad_`에 기록 (나머지는 ObjectiveSystem으로).
  - 공개 API: `std::optional<std::string> ConsumePendingSceneLoad();` (읽으면서 비움).
  - 새 API: `void ReloadScene(const std::string& path, Scene&, TopDownCamera&);` — 내부적으로 `BuildArena` + `ObjectiveSystem::Reset` 둘 다 수행. App이 이 하나만 호출하도록.
- `BlueFrog/Core/App.cpp::DoFrame` — `UpdateModel` 후:
  ```cpp
  if (auto path = gameplaySimulation.ConsumePendingSceneLoad()) {
      gameplaySimulation.ReloadScene(*path, scene, camera);
      hudState = gameplaySimulation.BuildHudState(scene);  // stale 1-frame 방지
  }
  ```
- `BlueFrog/Assets/Scenes/sparring_yard.json` — `TriggerZone_Center`에 `"action": { "type": "load_scene", "param": "Assets/Scenes/arena_trial.json" }` 추가.

**완료 기준**:
- sparring_yard 시작 → 중앙 진입 → arena_trial 로드 → 타이틀 `"Defeat the scout"`, 플레이어/적 모두 arena_trial 시작 상태, stale frame 없음.
- arena_trial에서 시작한 세션은 이전과 무변화 (트리거 없음).

**롤백**: 단일 커밋 revert. 스키마 bump 없으므로 기존 씬 파일 건드릴 것 없음.

**위험과 완화**:
- mid-frame reload 중 iterator live 문제 → **Update 종료 후**에 reload하므로 safe.
- 재진입 문제 (arena_trial에서 다시 sparring으로?) → 이번 페이즈에는 return trigger 없음. Scene::Clear가 trigger `fired` 상태도 날리므로 향후 양방향 이동에도 안전.
- 플레이어 상태 이관: v1은 **전체 리셋**. `docs/SCENE_SCHEMA.md`에 명시.

---

### A-5 — 문서화 + validator 강화 + 최종 검증

**수정**
- `D:\Work\Projects\BlueFrog\docs\SCENE_SCHEMA.md`
  - 새 섹션: `"objective"` 블록 (text/completionText/conditions[enemy_killed]).
  - 새 섹션: trigger `"action"` 블록 (log/publish/load_scene).
  - **명시**: "Scene load is a full reset — player state, trigger fired flags, objective progress all cleared."
- `BlueFrog/Engine/Scene/SceneLoader.cpp::Validate` — 이미 A-3/A-4에서 조건 타입 / action 타입 검사 추가됨. 이 커밋에서는 최종 점검 + 에러 메시지 일관성 스윕.
- `D:\Work\Projects\BlueFrog\docs\PHASE_6_EXECUTION_PLAN.md` — 이 파일을 복사.
- `docs/PHASE_5_EXECUTION_PLAN.md` 말미 — "Phase 6 실행 계획 수립됨" 링크.
- `docs/GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md` — Phase D 교차 링크 갱신.

**완료 기준**:
- 고의로 `"objective.conditions[0].type": "kill_zombie"` 같은 오탈자 → 창 뜨기 전 path-prefixed 에러.
- 고의로 `"trigger.action.type": "teleport"` → 같은 방식으로 reject.
- 정상 상태 → validator silent.

**롤백**: 문서만. 코드 변경 없음 (validator 강화는 소규모).

## Critical Files

### 읽기/참조 (기존 재사용)
- `BlueFrog/Game/Combat/CombatSystem.cpp:31-57` — `ApplyDamage` 킬 발행 지점
- `BlueFrog/Game/Simulation/GameplaySimulation.cpp:9-16` — 시스템 call order
- `BlueFrog/Game/Simulation/TriggerGameplaySystem.cpp:50-62` — 트리거 fire 로직
- `BlueFrog/Game/Hud/HudPresenter.h:31` — 하드코딩 제거 타겟
- `BlueFrog/Engine/UI/HudState.h:28` — `objectiveText` 정의
- `BlueFrog/Engine/Scene/TriggerComponent.h` — action optional 확장
- `BlueFrog/Engine/Scene/SceneLoader.cpp:157` — `scene.Clear()` 호출 (reload 경로가 자동 상속)
- `BlueFrog/Core/App.cpp:92-94` — `ComposeFrame`의 title set 루프
- `BlueFrog/Core/App.cpp:16` — 최초 BuildArena (reload 패턴의 선례)

### 신규
- `BlueFrog/Engine/Events/GameEvent.h`
- `BlueFrog/Engine/Events/EventBus.h/.cpp`
- `BlueFrog/Game/Objectives/ObjectiveState.h`
- `BlueFrog/Game/Objectives/ObjectiveSystem.h/.cpp`

### 수정 (중심축)
- `BlueFrog/Game/Combat/CombatSystem.h/.cpp` — A-2 (EventBus* param, 킬 publish)
- `BlueFrog/Game/Player/PlayerController.cpp` — A-2 (EventBus 스레딩)
- `BlueFrog/Game/Player/PlayerGameplaySystem.cpp` — A-2 (EventBus 전달)
- `BlueFrog/Game/Simulation/TriggerGameplaySystem.h/.cpp` — A-2 (publish), A-4 (action dispatch)
- `BlueFrog/Game/Simulation/GameplaySimulation.h/.cpp` — A-1 (eventBus), A-2 (debug drain), A-3 (ObjectiveSystem), A-4 (ReloadScene, ConsumePendingSceneLoad)
- `BlueFrog/Game/Simulation/GameplayArenaBuilder.h` — A-3 (ObjectiveState out-param)
- `BlueFrog/Game/Hud/HudPresenter.h` — A-3 (하드코딩 제거)
- `BlueFrog/Engine/Scene/SceneLoader.h/.cpp` — A-3 (objective 파싱/validate), A-4 (action 파싱/validate)
- `BlueFrog/Engine/Scene/TriggerComponent.h` — A-4 (action 필드)
- `BlueFrog/Core/App.cpp` — A-4 (reload 요청 처리)
- `BlueFrog/Assets/Scenes/arena_trial.json` — A-3 (objective 블록)
- `BlueFrog/Assets/Scenes/sparring_yard.json` — A-3 (objective), A-4 (load_scene action)
- `BlueFrog/BlueFrog.vcxproj` + `.filters` — 각 단계에서 신규 파일 등록

### 삭제
없음.

## Verification

Phase 6 완료 판정은 아래 **모두** 만족 시.

1. **빌드**: `Debug|x64`, `v143`, 경고 0개 증분.
2. **Phase 5 회귀**: `--scene` 인자 없이 실행 → arena_trial 정상 동작, 이동/공격/충돌/HUD 전부 regression 없음.
3. **이벤트 발행**: 적 처치 시 `EnemyKilled` 발행, 트리거 진입 시 `TriggerFired` 발행 (A-2에서 디버그 드레인으로 확인 후 제거됨).
4. **Objective 동적 텍스트**: arena_trial에서 스카우트 처치 → 타이틀 `"Defeat the scout"` → `"Arena cleared"`. sparring_yard에서 2마리 모두 처치 → `"Yard cleared"`.
5. **씬 전환**: sparring_yard 중앙 진입 → arena_trial 로드, HUD 1-frame stale 없음.
6. **Startup validator**: 오탈자 조건 타입 / action 타입 → path-prefixed 에러로 창 생성 전 종료. 정상 시 silent.
7. **`EventBus::Drain` 누수 없음**: tick 끝에 큐 비어있음 (assert 또는 smoke test).
8. **HudPresenter 하드코딩 부재**: `"Defeat the scout"` / `"Arena cleared"` 리터럴이 C++ 파일에 존재하지 않음 (grep 확인).
9. **v2 역호환**: `objective`/`action` 없는 v2 씬도 여전히 로드됨 (임시 테스트 파일 or arena_trial 이전 스냅샷).

## 위험과 롤백

- **Re-entrant publish**: v1에서 소비자가 drain 중에 재발행하면 `assert` fail. `draining_` 플래그로 보호. 실제 승격 필요 시 Phase 7에서 listener 모델로.
- **PlayerController 시그니처 churn**: EventBus 스레딩이 연쇄 수정을 일으킴. A-2 단일 커밋에 압축 — 반으로 나누면 중간 상태 빌드 불가.
- **Player 상태 리셋**: 씬 전환 시 체력/위치 전부 리셋. 세이브 인프라 생기면 재설계. `SCENE_SCHEMA.md`에 명시.
- **롤백 포인트**: 각 단계 독립 커밋. A-3이 가장 큰 개념 변경(목표 상태기) — 이 직후 `phase6-a3-objectives` 태그 권장.
- **중단 안전성**:
  - A-1 후 중단 → dead code 하나 추가, 기능 변화 0.
  - A-2 후 중단 → 디버그 드레인만 있는 상태, 기능적으로 Phase 5와 동일 + 콘솔에 이벤트 로그.
  - A-3 후 중단 → objective 시스템 완비, 씬 전환만 없음. 완결 가능.
  - A-4 후 중단 → Phase 6 기능 전체 완료. A-5는 문서/validator 정리뿐.

## 진행 상태

- [ ] A-1 EventBus 뼈대 (GameEvent, EventBus, GameplaySimulation 멤버)
- [ ] A-2 Publisher 배선 (CombatSystem, TriggerSystem) + 임시 debug drain
- [ ] A-3 ObjectiveState/System + HUD 동적화 + 씬 `"objective"` 블록
- [ ] A-4 Trigger 액션 + 씬 전환 (`LoadSceneRequested`, `ReloadScene`)
- [ ] A-5 SCENE_SCHEMA.md 확장 + validator 최종 점검
