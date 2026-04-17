# BlueFrog Phase 4 Execution Plan

> Phase 0–3 완료 이후 렌더/데이터 파이프라인을 한 단계 정리하는 단기 실행 계획서. 이 문서는 [PHASE_0_TO_3_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_0_TO_3_EXECUTION_PLAN.md)의 "Phase 4 예고" 섹션을 실제 실행 계획으로 구체화한 문서이며, [GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md)의 Phase D 첫 단계와 직접 맞물린다.

## Context

Phase 0–3은 완료됐다. Codex 후속 리팩터로 `GameplaySimulation`/`GameplayArenaBuilder`/시스템 분리까지 끝났고, 최근 커밋(`ee4de71`/`5c6fe36`/`25cfaf1`)에서 **텍스처가 입혀진 바닥 plane**이 메인에 들어왔다. 하지만 이 textured ground는 임시 해킹 상태다.

- `RenderComponent::visualKind` enum으로 `SolidColor`/`Textured` 분기
- `Renderer::Render()`가 `(visualKind == Textured && meshType == Plane)` 특수 조건으로만 텍스처 경로 진입
- 텍스처는 `Surface::MakeCheckerboard`로 **메모리에서 생성** — 파일 로딩 경로 자체가 없음
- 머티리얼/조명/Normal 개념이 전무
- `GameplayArenaBuilder::BuildArenaGeometry`가 모든 객체의 좌표·크기·색상·이름을 **리터럴로 하드코딩** (헤더에 인라인)
- `Shaders/`, `Assets/` 폴더는 존재하지 않고 HLSL은 `.h` 안에 문자열 상수

Phase 4의 목표는 이 두 축을 동시에 정리하는 것이다.

- **트랙 A — 텍스처/머티리얼 일반화 + 간단 조명**: textured ground 해킹을 걷어내고, 모든 오브젝트가 같은 Material/파이프라인을 거치도록 일반화한다. 파일 기반 텍스처 로딩과 디렉셔널 라이트 1개를 붙인다.
- **트랙 B — 씬 직렬화**: `GameplayArenaBuilder` 하드코딩을 JSON 파일 로더로 교체한다. 이는 `GAMEOBJECT_COMPONENT_TRANSITION_PLAN.md`의 Phase D 첫 단계와 정확히 일치한다.

두 트랙을 인터리빙 순서로 진행하는 이유는 **스키마 migration 비용**이다. A를 완전히 먼저 끝내면 B v1이 최신 Material shape을 반영할 수 있지만 A의 라이팅 필드가 늦게 들어와 추후 재작업 위험이 있고, B를 먼저 끝내면 `visualKind` 해킹이 v1 스키마에 새겨져 A 진행 시 반드시 마이그레이션이 필요해진다. 인터리빙(A-1 → A-2 → A-3 → B-1 → B-2 → A-4)이 되돌림 비용 최소 경로다.

## 핵심 설계 결정

| 항목 | 선택 | 근거 |
|---|---|---|
| 텍스처 파일 로더 | **WIC 직접 사용** | BlueFrog는 Windows-only + 기존에 `d3d11`/`dxgi`/`DirectXMath`/`wrl` 이미 링크 중. 의존성 0. stb_image의 크로스플랫폼 이점 무의미. DirectXTK는 과함. |
| Material 설계 | **`RenderComponent` inline value** (`std::optional<Material>`) | Phase 4에선 텍스처 공유 필요성이 미미 (Ground + ShrineCore 정도). `Renderer` 내부 `unordered_map<string, Texture2D>`로 캐시만 유지. 외부 레지스트리는 오버엔지니어링. |
| 정점 포맷 | **단일 포맷 통합**: POS+NORMAL+UV | 분기 제거가 핵심 이득. `SolidColor`는 1×1 white 기본 텍스처로 수렴시켜 같은 파이프라인을 탄다. |
| 조명 범위 | **디렉셔널 1개 + ambient** | "작은 한 걸음" 원칙. 포인트 라이트는 Phase 5에서 신전 발광/피격 이펙트와 묶는다. |
| 직렬화 포맷 | **nlohmann/json 단일 헤더** (`vendor/nlohmann/json.hpp`) | 손 편집 가능성 최대. 자체 파서는 스코프를 2배로 늘림. 유일 include site는 `SceneLoader.cpp`. |
| 트랙 순서 | **A-1 → A-2 → A-3 → B-1 → B-2 → A-4** | 위 Context 참고. 스키마 migration 비용 최소. |

## 실행 순서

각 단계는 **빌드·실행 가능한 경계**이며 중간 커밋이 가능해야 한다. `Debug|x64`/`v143` 전제.

### 단계 A-1 — WIC 기반 텍스처 파일 로더

**신규 파일**

- [BlueFrog/Engine/Render/ImageLoader.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/ImageLoader.h)
- [BlueFrog/Engine/Render/ImageLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/ImageLoader.cpp)
  - 시그니처: `Surface LoadSurfaceFromFile(const std::wstring& path);`
  - `CoInitializeEx(COINIT_MULTITHREADED)` 호출 (이미 초기화된 경우 `S_FALSE`/`RPC_E_CHANGED_MODE`는 무시)
  - `IWICImagingFactory::CreateDecoderFromFilename` → frame → `WICConvertBitmapSource(GUID_WICPixelFormat32bppRGBA)` → `CopyPixels` → `Surface`에 바이트 복사

**신규 자산**

- `BlueFrog/Assets/Textures/ground_checker.png` (64×64 이상, 기존 체커 느낌 유지)
- [BlueFrog/BlueFrog.vcxproj](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.vcxproj) 및 `.filters`에 `Assets/**`, `ImageLoader.*` 등록. 자산은 `<None>` + `Copy to Output Directory = Copy if newer`로 배포.

**수정**

- [BlueFrog/Core/Renderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.cpp):32 — `Surface::MakeCheckerboard` 호출을 `LoadSurfaceFromFile(L"Assets/Textures/ground_checker.png")`로 교체. 단, **`MakeCheckerboard`는 당분간 삭제하지 않음** (A-1 롤백 여지 확보). A-2 끝에서 삭제.

**완료 기준**: 실행 화면이 이전과 시각적으로 동일. 작업 디렉터리 상 `Assets/Textures/ground_checker.png`가 누락되면 로더 예외가 던져져 App 초기화에서 실패 — 이 동작을 확인.

### 단계 A-2 — Material 데이터 구조 + Renderer 내부 텍스처 캐시

**신규**

- [BlueFrog/Engine/Scene/Material.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/Material.h)

  ```cpp
  enum class SamplerPreset { WrapLinear, ClampLinear, WrapPoint };
  struct Material {
      std::string texturePath;       // 빈 문자열이면 default white
      DirectX::XMFLOAT3 tint{1,1,1};
      SamplerPreset sampler = SamplerPreset::WrapLinear;
  };
  ```

**수정**

- [BlueFrog/Engine/Scene/RenderComponent.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/RenderComponent.h)
  - `std::optional<Material> material` 필드 추가
  - `tint`, `visualKind`는 **일단 유지** (A-3에서 제거). 이 단계에서는 Material이 non-nullopt면 새 경로, nullopt면 기존 경로 — 이중 지원.
- [BlueFrog/Core/Renderer.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.h) / [Renderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.cpp)
  - `std::unordered_map<std::string, Texture2D> textureCache`
  - `Texture2D& ResolveTexture(const std::string& path)` 내부 헬퍼 — 캐시 미스 시 `LoadSurfaceFromFile` → `Texture2D` 생성 → 캐시 등록
  - `Texture2D defaultWhiteTexture` 멤버 (1×1 RGBA `{255,255,255,255}`, `Surface`를 직접 만들어 업로드)
  - `Sampler` 인스턴스를 `SamplerPreset`별로 미리 생성해 맵으로 보유
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h):28-35 — `createRenderable` 람다에 `Material` 선택 인자 추가. `Ground`는 `Material{ "Assets/Textures/ground_checker.png", {1,1,1}, WrapLinear }`로 지정, 나머지 객체는 Material을 넘기지 않거나 `Material{ "", tint, ClampLinear }`로 명시.

**완료 기준**: 모든 객체가 Material을 거쳐 색이 결정되지만 실행 화면은 이전과 동일. `Renderer::Render()` 내부 분기는 아직 남아 있어도 된다. `Surface::MakeCheckerboard` 호출 경로 삭제 가능 여부 확인.

### 단계 A-3 — 단일 파이프라인 통합 + Normal + visualKind 제거

이 단계가 Phase 4에서 가장 큰 변경. 독립 커밋 + 태그(`phase4-a3-pipeline-unified`)로 롤백 포인트 확보.

**신규**

- [BlueFrog/Engine/Render/LitPipeline.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/LitPipeline.h) — 현 [FlatColorPipeline.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/FlatColorPipeline.h)와 [TexturedPipeline.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/TexturedPipeline.h)를 대체
  - 입력 레이아웃: `POSITION(float3)`, `NORMAL(float3)`, `TEXCOORD(float2)`
  - HLSL cbuffer: `TransformBuffer : register(b0)`, `MaterialBuffer : register(b1)` (tint+pad), `LightBuffer : register(b2)` (A-3에선 비어 있고 A-4에서 채움)
  - PS: `albedo = surfaceTexture.Sample(...)`, 출력 = `albedo * float4(tint,1)` (라이팅 미적용)
  - `register(b0)` 중복 문제도 이 설계에서 자연 해소

**수정**

- [BlueFrog/Core/Renderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.cpp)
  - `GetCubeVertices`를 **face별 24정점**으로 확장 (각 면 고유 normal + UV 0..1)
  - `GetPlaneVertices`는 normal=+Y로 통일, UV 범위는 Ground tiling을 위해 `0..9` 유지 (plane 자체가 9타일을 암묵 가정, `uvScale` 같은 확장 필드는 Phase 5로 보류)
  - `GetTexturedPlaneVertices` 제거, 단일 `GetPlaneVertices`로 수렴
  - `TexturedVertex`/`Vertex` 두 구조 제거, 단일 `LitVertex` 구조
  - `BindFlatColorState` / `BindTexturedState` 제거, 단일 `BindLitState`
  - `Render()` 내부 `visualKind`/`meshType` 분기 **완전 제거**. 모든 객체는 `material ? *material : Material{}` 경로.
- [BlueFrog/Engine/Scene/RenderComponent.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/RenderComponent.h)
  - `visualKind` enum 삭제
  - `tint` 필드 삭제 (Material로 흡수)
  - `meshType`는 유지 (여전히 지오메트리 선택 수단)
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h)
  - `createRenderable` 서명에서 `visualKind` 제거, `tint` 인자 → `Material` 인자로 교체
  - 모든 객체에 Material을 명시적으로 지정
- 구 파이프라인 헤더 2개 삭제 (`FlatColorPipeline.h`, `TexturedPipeline.h`)

**검증**

- `Renderer::Render()`에서 `visualKind` grep → 0건
- `RenderVisualKind` 타입 grep → 0건
- Ground 텍스처는 그대로 보이고, 나머지 큐브들은 tint만 적용된 평면색 (라이팅 없음)
- 단계가 제일 크므로 스크린샷 1장 확보 후 커밋

### 단계 B-1 — nlohmann/json vendor 추가 + `SceneLoader` skeleton

**신규**

- `BlueFrog/vendor/nlohmann/json.hpp` — single-header 드롭
- [BlueFrog/BlueFrog.vcxproj](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.vcxproj)에 `AdditionalIncludeDirectories`로 `$(ProjectDir)vendor` 추가
- [BlueFrog/Engine/Scene/SceneLoader.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.h) / [SceneLoader.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneLoader.cpp)

  ```cpp
  class SceneLoader {
  public:
      static bool Load(const std::filesystem::path& path,
                       Scene& scene,
                       TopDownCamera& camera,
                       std::string* errorOut);
  };
  ```

  - 구현은 스켈레톤: `schemaVersion` 확인만. 파싱 실패/버전 불일치 시 `errorOut`에 사유, `false` 반환.
  - `json.hpp` include는 **이 .cpp에서만** (빌드 시간 격리).
- `BlueFrog/Assets/Scenes/arena_trial.json` — 현재 `BuildArenaGeometry`의 정확한 전사 (수동 작성)

**미수정**

- `GameplayArenaBuilder`는 아직 손대지 않음. `SceneLoader`는 존재하지만 아무도 호출 안 함.

**완료 기준**: 빌드 pass, 실행 동작 불변. `SceneLoader::Load`를 임시 경로에서 한 번 호출해 JSON 파싱이 되는지만 확인하고, 결과는 사용하지 않음 (로그만).

### 단계 B-2 — `SceneLoader` 완성 + `GameplayArenaBuilder` 교체

**수정**

- `SceneLoader.cpp`
  - 각 SceneObject에 대해 `transform` / `render`(optional) / `collision`(optional) / `combat`(optional) 파서 구현
  - `render.mesh` 문자열 → `RenderMeshType` (`"cube"` → `Cube`, `"plane"` → `Plane`)
  - `render.material.texture` 상대 경로 → exe 기준 resolve
  - `render.material.sampler` 문자열 → `SamplerPreset`
  - `collision.halfExtents`, `collision.blocking`
  - `combat.faction` (`"player"`/`"enemy"`/`"neutral"`), `maxHp`, `currentHp`
  - `scene.camera.target` → `TopDownCamera::SetTarget`
  - 미지 키는 **경고 로그 + skip** (후방 호환성)
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h)
  - `BuildArenaGeometry` 본체 완전 삭제
  - `Build`는 `SceneLoader::Load("Assets/Scenes/arena_trial.json", scene, camera, &err)` 호출만
  - 실패 시 에러 로그 + 빈 씬 fallback
- `BlueFrog/Assets/Scenes/arena_trial.json`에 `Ground`, `ShrineCore`, `NorthWall`/`SouthWall`/`EastWall`/`WestWall`, `PillarA`/`PillarB`, `Player`(rotation 포함), `EnemyScout` 모두 포함. 이름은 기존과 동일해야 `GameplaySceneIds::Player` / `EnemyScout` 조회가 깨지지 않음.

**스키마 (v1)**

```json
{
  "schemaVersion": 1,
  "scene": {
    "name": "ArenaTrial",
    "camera": { "target": [-4.0, 1.25, 0.0] },
    "objects": [
      {
        "name": "Ground",
        "transform": { "position": [0,0,0], "rotation": [0,0,0], "scale": [18,1,18] },
        "render": {
          "mesh": "plane",
          "material": { "texture": "Assets/Textures/ground_checker.png", "tint": [1,1,1], "sampler": "wrap_linear" }
        }
      },
      {
        "name": "Player",
        "transform": { "position": [-4,1.25,0], "rotation": [0,1.5708,0], "scale": [0.7,1.25,0.7] },
        "render":    { "mesh": "cube", "material": { "tint": [0.82,1.0,0.55] } },
        "collision": { "halfExtents": [0.45,0.45], "blocking": true },
        "combat":    { "faction": "player", "maxHp": 5, "currentHp": 5 }
      }
    ]
  }
}
```

스키마 원칙:

- `schemaVersion` 최상위 필수. 로더는 현재 버전만 읽고 미지 버전은 즉시 거부.
- 컴포넌트는 모두 **optional 오브젝트**. 키가 없으면 `std::optional = nullopt`. 기본값은 C++ struct의 기본값이 권위.
- `material.texture` optional: 없으면 solid color로 취급(내부에서 1×1 white 바인딩).
- 미지 키는 **경고 로그 + skip**. 이게 후방 호환의 핵심.
- 미래 확장(`lights`, `triggers`, `spawnPoints`)은 `scene` 하위에 새 배열로 추가. 기존 로더가 미지 키를 스킵하므로 구 파일이 그대로 돈다.
- `mesh`는 enum 문자열(`"cube"`, `"plane"`). 미래에 외부 mesh 파일이 생기면 `{"source":"assets/meshes/...","format":"obj"}` 오브젝트로 확장 가능하도록 string/object 둘 다 받을 수 있게 둔다.

**완료 기준**: `GameplayArenaBuilder.h`가 20줄 이하 thin wrapper로 축소. `arena_trial.json`의 tint를 손으로 편집해 재실행 → 즉시 반영. JSON 고의 파괴 → 크래시 없이 에러 로그 + 빈 씬.

### 단계 A-4 — 디렉셔널 라이트 활성화

**수정**

- [BlueFrog/Engine/Render/LitPipeline.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/LitPipeline.h)
  - `LightBuffer` 구조 채움: `float3 lightDirWS; float ambient; float3 lightColor; float pad;`
  - VS가 normal을 world space로 변환 → PS에 전달 (world matrix의 normal 행렬 생략, uniform scale 전제)
  - PS: `float n_dot_l = saturate(dot(normalize(normalWS), -lightDirWS)); return albedo * float4(tint,1) * (ambient + n_dot_l * lightColor);`
- [BlueFrog/Core/Renderer.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.h) / [Renderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.cpp)
  - `ConstantBuffer<LightData> lightBuffer`
  - `BeginFrame` 또는 `Render` 진입부에서 한 번 Update
  - 초기값: `lightDirWS = normalize({0.3, -1, 0.2})`, `ambient = 0.35`, `lightColor = {1, 0.96, 0.9}`
  - per-object 드로우에서 `lightBuffer`는 바인딩만 (업데이트 없음)

**완료 기준**: 큐브 면이 방향에 따라 명암 차이를 보임. `lightDirWS`를 런타임에 한 번 바꿔가며 스크린샷 2장(변경 전/후)으로 확인.

## Critical Files

### 읽기/참조 (기존, 재사용)

- [BlueFrog/Engine/Render/Texture2D.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/Texture2D.h) — Surface→GPU 업로드 + `Bind`. 그대로 사용.
- [BlueFrog/Engine/Render/Sampler.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/Sampler.h) — `Bind(slot)` 지원. `SamplerPreset`별 인스턴스 생성에 재사용.
- [BlueFrog/Engine/Render/Surface.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/Surface.h) — CPU RGBA 배열. `MakeCheckerboard`는 A-2 이후 삭제.
- [BlueFrog/Engine/Render/ConstantBuffer.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Render/ConstantBuffer.h) — 템플릿 `VertexConstantBuffer<T>` / `PixelConstantBuffer<T>`. LightBuffer에 그대로 사용.
- [BlueFrog/Engine/Scene/Scene.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/Scene.h) — `CreateObject`, `FindObject`, `Clear`. `SceneLoader`가 이 API만 사용.
- [BlueFrog/Engine/Scene/SceneObject.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/SceneObject.h) — `transform`, `renderComponent`, `collisionComponent`, `combatComponent` (모두 optional).
- [BlueFrog/Engine/Scene/Transform.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/Transform.h) — `position`, `rotation`, `scale`, `GetMatrix`.
- [BlueFrog/Game/Simulation/GameplaySceneIds.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplaySceneIds.h) — `Player`, `EnemyScout` 문자열. JSON 스키마의 객체명이 이와 반드시 일치.

### 신규

- `BlueFrog/Engine/Render/ImageLoader.h/.cpp`
- `BlueFrog/Engine/Render/LitPipeline.h`
- `BlueFrog/Engine/Scene/Material.h`
- `BlueFrog/Engine/Scene/SceneLoader.h/.cpp`
- `BlueFrog/vendor/nlohmann/json.hpp`
- `BlueFrog/Assets/Textures/ground_checker.png` (최소 1개)
- `BlueFrog/Assets/Scenes/arena_trial.json`

### 수정 (중심축)

- [BlueFrog/Core/Renderer.h](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.h) / [Renderer.cpp](/D:/Work/Projects/BlueFrog/BlueFrog/Core/Renderer.cpp) — 단계 A-2, A-3, A-4에서 연속 변경
- [BlueFrog/Engine/Scene/RenderComponent.h](/D:/Work/Projects/BlueFrog/BlueFrog/Engine/Scene/RenderComponent.h) — A-2(필드 추가)/A-3(필드 제거)
- [BlueFrog/Game/Simulation/GameplayArenaBuilder.h](/D:/Work/Projects/BlueFrog/BlueFrog/Game/Simulation/GameplayArenaBuilder.h) — A-2(Material 명시)/A-3(서명 변경)/B-2(로더 호출만)
- [BlueFrog/BlueFrog.vcxproj](/D:/Work/Projects/BlueFrog/BlueFrog/BlueFrog.vcxproj) + `.filters` — 신규 파일/자산/include 경로 추가

### 삭제

- `BlueFrog/Engine/Render/FlatColorPipeline.h` (A-3)
- `BlueFrog/Engine/Render/TexturedPipeline.h` (A-3)
- `RenderVisualKind` enum (A-3)

## Verification (end-to-end)

Phase 4 완료 판정은 아래 **모두** 만족 시.

1. **빌드**: `Debug|x64`, `v143`, 경고 0개 (기존 대비 증분 기준).
2. **기본 실행**: 탑다운 시점에서 아레나가 이전과 동일하게 보이고, 플레이어 이동/회전/공격/충돌/적 추적/HUD가 regression 없이 동작.
3. **텍스처**: Ground 체커가 **파일**(`Assets/Textures/ground_checker.png`)에서 로드된 것이 맞는지 — 파일 삭제/리네임 시 초기화 예외 발생으로 확인.
4. **파이프라인 일반화**: `grep -r "visualKind" BlueFrog/` → 0건, `grep -r "RenderVisualKind" BlueFrog/` → 0건. `Renderer::Render`에 meshType/visualKind 분기 없음.
5. **조명**: `lightDirWS`를 코드에서 90° 회전하면 큐브의 밝은 면이 따라 돈다 (전/후 스크린샷 2장).
6. **직렬화 왕복**: `Assets/Scenes/arena_trial.json`을 손으로 편집해 `ShrineCore`의 `tint`를 `[0.3, 0.5, 1.0]`로 바꾸고 **재빌드 없이** 재실행 → 신전이 푸른빛. 원상복구 편집 → 다시 기존 색.
7. **JSON 파괴 내성**: `arena_trial.json`에 키 오타 또는 타입 오류 주입 → 크래시 없이 에러 로그 + 빈 씬 fallback.
8. **객체 이름 안정성**: `GameplaySceneIds::Player` / `EnemyScout` 조회가 그대로 동작 — 실행 중 플레이어 입력 반응, 적 추적 동작 보장.
9. **`GameplayArenaBuilder.h` 축소**: 파일이 20줄 이하 thin wrapper.

## 위험과 롤백

- **WIC `CoInitialize` 경합**: 이미 초기화된 스레드에서 `COINIT_MULTITHREADED` 재호출 시 `RPC_E_CHANGED_MODE`. 로더에서 `S_FALSE`/`RPC_E_CHANGED_MODE`는 무시하고, 사용 후 `CoUninitialize`는 **하지 않는다**.
- **Cube normal 확장 실수**: A-3에서 즉시 눈에 띔. `GetCubeVertices`는 A-3 안에서도 별도 커밋으로 분리 권장.
- **`register(b0)` 전환**: `LitPipeline`은 처음부터 `b0/b1/b2`로 깨끗하게 설계되므로 기존 중복 문제는 자연 해소.
- **`nlohmann/json` 빌드 시간**: include site를 `SceneLoader.cpp` 한 곳으로만 제한. 헤더에는 전방 선언 없이 public API만.
- **인터리빙 중단 안전성**: A-3 완료 + B-2 미완료 상태는 "Material 기반으로 통합됐지만 아직 하드코딩 아레나" — **기능적으로 멀쩡**. 어느 시점에 중단해도 플레이 가능.
- **롤백 포인트**: 각 단계가 독립 커밋. A-3 완료 시점에 `phase4-a3-pipeline-unified` 태그 권장 — 가장 큰 변경점이 여기이므로 그 이전/이후로 나눠 `git revert` 가능.

## 진행 상태

- [x] A-1 WIC 로더 + 자산 배포 (`bffdaf0`)
- [x] A-2 Material + 텍스처 캐시 (`bf8146a`)
- [x] A-3 LitPipeline 통합 + visualKind 제거 (`a1ff1b8`)
- [x] B-1 nlohmann/json vendor + SceneLoader skeleton (`a3ad9d4`)
- [x] B-2 SceneLoader 완성 + 하드코딩 아레나 교체 (`06bf0b9`)
- [x] A-4 디렉셔널 라이트 (`838f1cd`)

Phase 4는 모두 완료됐다. 다음 단계인 프리팹/다중 씬/트리거/씬 검증은 [PHASE_5_EXECUTION_PLAN.md](/D:/Work/Projects/BlueFrog/docs/PHASE_5_EXECUTION_PLAN.md)에서 이어진다.
