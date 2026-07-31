# Input & render status — vendored-deps build

Verified 2026-07-31 against `master` @ `28739e6c9` ("docs: the vendored build, and
deprecate the podman sysroot"), i.e. the first build that uses the vendored
GNUstep dependencies rather than the podman sysroot.

**Verdict: rendering is unchanged by the vendoring, and all tested input channels
work.**

## Test rig

- Xvfb `:9`, 1280x800x24, llvmpipe software GL, ~17-19 FPS.
- Game: 16.3 (`~/Game/16.3/LeagueofLegends.app`) replaying
  `16.3-EUN1-3908472567.rofl`, a 33 KB stub containing ~6.6 s of sim data.
  The sim pauses at the end of the stub but the renderer keeps running and the
  replay camera stays live, which is what makes this a clean input test.
- Screenshots via `import -window root`; differences via ImageMagick
  `compare -metric RMSE`. Unless stated otherwise, the quoted number is for the
  **centre region** `640x400+320+100` — the viewport away from HUD chrome, so
  HUD clocks and FPS counters don't pollute the metric.

## 1. Render check

`docs/img/vendored-16.3-replay.png` — captured ~35 s after `GAMESTATE_GAMELOOP`.

| Check | Result |
| --- | --- |
| File size (a black/garbage frame compresses small) | 1 260 107 B — 6x over the 200 KB floor |
| RMSE vs pre-vendoring `stub-split-16.3-replay-llvmpipe.png`, centre | **0.0123 (1.2 %)** |
| RMSE vs same, full frame | 0.0263 (2.6 %) |

Visually identical to the pre-vendoring baseline: Summoner's Rift blue-side
fountain, Lux on the fountain platter with health/level bar, scoreboard, minimap
with ward/turret icons, and the replay transport controls. The full-frame figure
is larger than the centre figure only because the HUD clock and FPS readout
differ between the two runs, exactly as expected.

Vendoring the dependencies did not regress rendering.

## 2. Input

There is **no window manager** on `:9`. `xdotool search` finds the SDL window
(1280x800 at 0,0, no `WM_NAME`/`WM_CLASS` since nothing sets them), and
`xdotool getwindowfocus` already reports a focused window — SDL sets input focus
itself. **No focus tricks were needed**: no click-to-focus, no `--window`
targeting, no `windowactivate`. Plain XTEST injection (`xdotool key`,
`mousemove`, `click`) reaches the game.

### Control: the idle frame is perfectly static

With no input at all, two screenshots 1.5 s apart differ by **RMSE 0 (exactly
zero)**. The sim is paused at the end of the replay stub and nothing animates.
That makes every number below unambiguous — any non-zero delta is caused by the
injected event and nothing else.

| Channel | How | Centre RMSE | Verdict |
| --- | --- | --- | --- |
| Arrow key pan | `keydown Right`, 2 s, `keyup Right` | **0.241 (24 %)** | works |
| Space (centre on champion) | `key space` | **0.241 (24 %)** | works |
| Mouse edge pan | `mousemove 1279 400`, held 2 s | **0.274 (27 %)** | works |
| Minimap click | `mousemove 1180 700` + `click 1` | **0.242 (24 %)** | works |

Corroborating detail beyond the raw deltas:

- **Arrows** — the view left the fountain and travelled right onto the blue-side
  terrain past the nexus, and the minimap camera rectangle tracked the move.
  Captured as `docs/img/vendored-16.3-input-pan.png`.
- **Space** — pressed while the camera was parked on the panned-away view, it
  snapped back onto the champion, landing within **RMSE 0.0007 (0.07 %)** of the
  original fountain framing. That is a semantically correct "centre on champion",
  not merely a frame that changed.
- **Edge pan** — pure pointer *motion*, no button, so mouse-motion events are
  being delivered, not just clicks.
- **Minimap click** — the camera jumped to the clicked map location and the
  minimap's camera rectangle repositioned itself directly under the click point,
  which means click coordinates arrive correctly scaled, not merely as an
  undifferentiated "a click happened".
- Every camera-affecting input also flipped the bottom-left widget from
  `Directed Camera` to `Directed Camera in 5...`, the game's own acknowledgement
  that a manual camera command was received. This is the game's internal state
  reacting, independent of anything measured from pixels.

### Log evidence

The run log is silent about input: no dropped-event warnings, no unhandled
selector or unrecognised-message noise, and the log does not grow while events
are being injected. Whatever the AppKit shims are doing with `NSEvent` /
`NSApplication`, they are not on the hot path for these events, and they are not
complaining.

## What was not tested

- **Gameplay input.** This is a replay of a 33 KB stub, so only replay camera
  controls exist. Ability casts, item shop, right-click move orders, and chat
  are untested — a live game or a full-length replay is needed for those.
- **Text entry.** No `NSTextInputContext` / IME path was exercised.
- **Modifier combinations and scroll wheel** (camera zoom) were not exercised.
- Only 16.3 was tested; 14.8 was not re-checked in this pass.

Nothing here suggests those are broken — they simply were not covered by this
test.
