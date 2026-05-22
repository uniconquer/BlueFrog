#pragma once

#include "BFTimer.h"
#include "Window.h"

#include <string>

// Engine-side application framework. Owns the bits that every game needs
// the same way — the OS window, the Win32 message pump, the dt timer, the
// main loop skeleton — and exposes the per-frame extension points as
// virtual hooks. The Unity / Unreal mental model: the engine drives the
// loop, the game subclasses and overrides Update/Render.
//
// Why this lives in the engine and not the game: every BlueFrog-powered
// title would otherwise re-author the same boilerplate (open window, pump
// messages, mark timer, BeginFrame/EndFrame, drive systems). Centralizing
// here keeps the game code focused on what is genuinely game-specific —
// renderers, scene composition, gameplay simulation, HUD.
//
// Lifecycle:
//   AppBase ctor:    creates Window + initializes timer.
//   AppBase::Run():  calls OnStartup once, then loops until Window posts
//                    a quit code: ProcessMessages → mark dt → OnUpdate(dt)
//                    → OnRender(). Calls OnShutdown right before return.
//
// Hooks (subclass overrides):
//   OnStartup()  — runs once, after Window + Graphics are live. Use for
//                  scene loading, audio init, profile load, anything that
//                  needs the engine alive but should happen exactly once.
//   OnUpdate(dt) — every frame, before rendering. Game state mutation,
//                  input collection, simulation tick, animation tick.
//   OnRender()   — every frame, after OnUpdate. Game owns the entire
//                  Graphics::BeginFrame → … → Graphics::EndFrame block
//                  here. The base intentionally does NOT wrap this — clear
//                  color, render order, EndFrame post-processing (e.g.
//                  clearing transient camera offsets) are all game choices.
//   OnShutdown() — runs once before Run() returns. Save profile, flush
//                  audio, anything teardown-shaped. Defaulted to no-op.
class AppBase
{
public:
	AppBase(int windowWidth, int windowHeight, std::wstring windowTitle);
	virtual ~AppBase() = default;

	AppBase(const AppBase&)            = delete;
	AppBase& operator=(const AppBase&) = delete;

	// Runs the main loop until Window::ProcessMessages returns an exit
	// code (WM_QUIT). Returns that exit code to the caller (typically
	// wWinMain). Calling Run() more than once is undefined — the loop is
	// not designed to be re-entered.
	int Run();

protected:
	virtual void OnStartup()                 {}
	virtual void OnUpdate(float dt)          = 0;
	virtual void OnRender()                  = 0;
	virtual void OnShutdown() noexcept       {}

	// Engine accessors the game uses to build its own renderers /
	// gather input. Returned by reference so the game treats them as
	// permanent collaborators rather than nullable services.
	Window&    GetWindow()   noexcept { return wnd; }
	Graphics&  GetGfx()      noexcept { return wnd.Gfx(); }
	Keyboard&  GetKeyboard() noexcept { return wnd.kbd; }
	Mouse&     GetMouse()    noexcept { return wnd.mouse; }

private:
	Window  wnd;
	BFTimer timer;
};
