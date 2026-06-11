# BlueFrog / FL (Fantasy Life)

C++17 + DirectX 11 게임 엔진(BlueFrogEngine.lib) + GTA2식 탑다운 판타지 ARPG(FL.exe).
유저와의 대화는 한국어로.

## 빌드 / 실행

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" BlueFrog.sln /p:Configuration=Debug /p:Platform=x64 /t:Build /v:minimal /m
```

- 실행: 리포 루트의 `RunVillage.bat` (village.json), `Run.bat` (arena_trial.json). 직접: `x64\Debug\FL.exe --scene Assets/Scenes/village.json`
- 스모크: 실행 후 6초 뒤 `tasklist`로 생존 확인, `taskkill /IM FL.exe /F`
- 시각 검증: FLApp.cpp의 `EndTextDraw()` 직후에 백버퍼→`_frame.raw` 덤프 스니펫을 임시 삽입(프레임 ~110, 기동 14초 대기 — 밉맵 생성으로 느림) 후 PNG 변환해 확인. **확인 끝나면 반드시 제거** (.gitignore에 `_frame.raw` 있음)
- 여러 체크가 통과하는데 증상이 지속되면 → 시각적 ground truth(프레임 덤프)부터 떠라

## 구조

- `Engine/` — 순수 엔진 (Game include 금지!). Render/Scene/Camera/UI/Audio
- `Game/` — FL 게임플레이 (Player/Simulation/Quest/Inventory/Skill/Profile)
- `Core/` — FLApp(게임 측 앱, AppBase 서브클래스) + Renderer
- `Assets/` — Scenes(json) / Prefabs(json) / Models / Textures / Quests / Items / Recipes / Skills

## 씬·프리팹 규칙

- 씬은 **프리팹 참조**(`"prefab": "Assets/Prefabs/X.prefab.json"`)가 원칙. F12 저장은 FLApp의 authored doc(`sceneDoc_`)를 그대로 쓰는 DRY 방식 — 런타임 오브젝트를 풀어서 쓰지 않음
- 배치 툴: F4 토글, LMB 배치, [ ] 프리팹 순환, T 회전, Backspace 언두, F12 저장
- 스캐터(`scatter` 디렉티브) 생성물은 배치 툴로 영구 삭제 불가(리스폰됨) — 알려진 제약
- 1유닛 = 1m. 캐릭터 키 ~1.8

## 캐릭터 / 애니메이션 (함정 많음)

- 캐릭터: Quaternius Universal (`Assets/Models/Universal/HeroMale|HeroFemale`). 병합 스크립트: `_tmp_fbximport/merge_universal.py` (Blender 4.4 헤드리스, `-- Female`처럼 변형 지정)
- **MeshImporter는 glTF `meshes[0]`만 읽는다** (그 안의 primitive들은 전부 병합). 메시 오브젝트가 여러 개면 Blender에서 **join 필수** — 안 하면 첫 메시(눈썹 따위)만 로드되어 "안 보이는 캐릭터"가 됨
- 게임이 쓰는 클립명: **Idle / Walk / Run / Die / Slash / SlashDown / Hit / Ride**(말 탑승) + 말 전용 Gallop. UAL 클립을 이 이름으로 리네임해서 export
- **PlayerController가 플레이어 tint를 런타임에 덮어씀** — 프리팹 material.tint로 플레이어 색 조정 불가 (적/NPC는 가능)
- 스킨드 경로도 importScale 적용됨 (Renderer::DrawSkinnedMesh). 과거 "안 먹는다"는 오진 — 실제 원인은 meshes[0] 문제였음
- MaxJoints 128

## 렌더링 함정

- **PixelConstantBuffer(b1 MaterialBuffer 등)는 PS 전용** — VS에서 읽으면 0이 나옴 (uvScale 평면 텍스처 버그의 원인이었음). 셰이더에서 cbuffer를 쓰는 스테이지를 반드시 확인
- 텍스처는 풀 밉체인 생성(Texture2D.h: MipLevels=0 + GenerateMips) — 없으면 원거리에서 평균색으로 뭉개짐
- baseColor=sRGB, normal/roughness/ORM=linear
- **카메라 가림 = lit PS 디더 컷아웃** (LightBuffer의 cutoutTarget/cutoutRadius, 카메라→타깃 원통 픽셀을 Bayer 디더로 discard) + 플레이어 X-ray 실루엣(스킨드 소스의 PSSilhouette 엔트리, GREATER/no-write DSS). 오브젝트 단위 알파 페이드는 모듈 조각 중첩 얼룩 때문에 폐기했음 — 되돌리지 말 것

## 시뮬레이션 함정

- **AppBase가 프레임 dt를 0.1s로 클램프** — 씬 로드 히치(밉맵 생성 수 초)가 첫 틱에 통째로 들어가 AI 순간이동 + 넉백 벽 관통을 일으켰음. 클램프를 지우지 말 것

## 관례

- 커밋은 유저가 요청할 때만. 빌드+스모크(+시각 검증) 후 커밋
- 모델 폴더엔 NOTICE.md(출처/라이선스)
- 프로필 저장(F8) → `Save/profile.json` (gitignored). 프로필의 scenePath가 `--scene` 인자를 덮어씀
