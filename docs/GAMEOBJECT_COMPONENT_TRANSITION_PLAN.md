# BlueFrog GameObject-Component Transition Plan

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

### Phase A: Scene Foundations

Add a small scene layer that introduces:

- `Transform`
- `SceneObject`
- parent/child transform support later if needed

At this stage, a `SceneObject` can stay very small:

- stable name or id
- `Transform`
- enabled/disabled flag

## Phase B: First Components

Once the scene exists, add only the components needed for the top-down RPG slice:

- `RenderComponent`
- `CollisionComponent`
- `PlayerComponent`
- `NpcComponent`
- `CameraFollowComponent` or equivalent game-side logic

Avoid generic components that do not power the current game milestone.

## Phase C: Systems Around Components

After a few components exist, move recurring logic into systems:

- render system
- collision system
- player controller system
- npc behavior system

This gives many ECS benefits without forcing a full data-oriented rewrite.

## Phase D: Serialization

When object and component shapes settle, add data loading and saving for:

- scene placement
- test maps
- item and npc spawn data
- quest and interaction trigger data

At this point, scene files become the bridge between runtime objects and future editor tooling.

## Phase E: Editor-Oriented Features

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

## Immediate Next Step

The next practical implementation step is:

- add `Transform`
- add `TopDownCamera`
- render a simple top-down test map

That gives BlueFrog its first real scene-space foundation and sets up the future `SceneObject + Component` transition cleanly.
