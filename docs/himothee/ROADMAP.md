# Himothee Studio Roadmap

## Current status

Current development version: **0.10.8**

OBS baseline: **32.2.2**, commit ba2f32bdf791005443988a4955e963663e16b1ed.

The native multistream foundation through Stage 8 is implemented. The
experimental native chat path has been removed; multiplatform chat now uses
Botrix Multi Chat through OBS/Himothee Custom Browser Docks.

## Foundation

### v0.1 — OBS 32.2.2 baseline — Implemented

- Windows baseline CI.
- Himothee branch/upstream strategy.
- Clean OBS 32.2.2 starting point.

### v0.2 — Himothee identity — Implemented

- Product/version identity.
- Separate Himothee config/data directory.
- Side-by-side behaviour with stock OBS.
- About/attribution work.

## Native multistream

### v0.3 — Multi-output core — Implemented

- Multiple simultaneous secondary outputs.
- Destination lifecycle/state/error handling.
- Shared primary encoders.

### v0.4 — Multistream Manager — Implemented

- Add/edit/remove/enable destinations.
- Twitch, YouTube, Kick, and Custom RTMP.
- Start/stop and status controls.

### v0.5 — Shared encoder mode — Implemented

- Shared video/audio encoders.
- Compatibility checks.
- Live output health telemetry.

### v0.6 — Independent encoder mode — Implemented

- Shared or Independent mode per destination.
- Per-destination encoders.
- Per-destination bitrate and resolution.

### v0.7 — Reliability and telemetry — Implemented

- Reconnect policy.
- Reconnect/error counters.
- Uptime/state timing.
- Manual retry and partial-failure isolation.

### v0.8 — Audio routing — Implemented

- OBS Track 1-6 per destination.
- Shared-video plus dedicated-audio path.
- Per-destination audio bitrate.

### v0.9.x — Runtime compatibility and control hardening — Current

- Startup config-order hotfix.
- Visible destination diagnostics.
- Enhanced Broadcasting/shared-encoder conflict reporting.
- Platform-safe H.264 selection for Twitch/Kick secondary RTMP.
- Kick-safe limits and duplicate-primary-key detection.
- Separate Destination Enabled from Auto-start.
- Prepare enabled destinations without forcing them live.
- Go Live Selected / End Selected controls.
- Go Live Enabled / End Secondaries controls.
- Explicit shared OBS Input resolution and per-destination Output resolution status.
- Independent destination scaling for mixed-resolution streaming.
- Removed the experimental native chat implementation.
- Standardised chat on Botrix Multi Chat through Custom Browser Docks.

## Chat strategy

Native Twitch, YouTube, and Kick chat providers are not on the forward roadmap.

Recommended chat workflow:

- Botrix Multi Chat.
- Docks > Custom Browser Docks.
- One browser dock for combined Twitch/YouTube/Kick chat.

This avoids maintaining a duplicate OAuth/EventSub/live-chat API stack and keeps
Himothee focused on streaming/output and production features.

## Overlays and production tools

### Stage 10.1 — Overlay Engine & Dock — Implemented / runtime validation next

- Native Himothee Overlays dock.
- Per-profile `overlays.json`.
- Localhost overlay server on 127.0.0.1:3293.
- Stable per-overlay browser-source URLs.
- JSON runtime state endpoint.
- New/Delete/Save overlay controls.
- Show/Hide without reloading the Browser Source.
- Browser preview and Copy URL.
- Create OBS Browser Source in the current scene.
- First basic Text renderer.
- Width/height stored per overlay.

### Stage 10.2 — Timers & Counters — Implemented / runtime validation next

- Countdown timer.
- Count-up / stopwatch.
- Generic counter.
- Kill counter.
- Streak counter.
- Challenge/progress counter with progress bar.
- Stream uptime.
- +1 / -1 / Reset counter controls.
- Start / Pause / Reset timer controls.
- Backward-compatible Stage 10.1 Text widgets.

### Stage 10.3 — Overlay Designer — Implemented / runtime validation next

- Himothee Dark, Minimal, Neon, Transparent, and Custom themes.
- Font family and font size.
- Text/background colours.
- Background opacity and corner radius.
- Nine-position placement presets.
- None/Fade/Pop/Slide Up/Slide Left entry animations.
- Optional image/GIF/video/HTTPS media layer.
- Local media served through the localhost overlay engine.
- Designer values persisted per overlay.

### Stage 10.4 — Darts Widgets — Implemented / runtime validation next

- 180 counter.
- 140+ counter.
- 100+ counter.
- Legs Won counter.
- Match Wins counter.
- Two-decimal darts average display.
- Timed checkout notification with score/route.
- Manual Trigger/Clear and +1/-1/Reset controls.
- Persistent darts settings/values.
- HimotheeLink/Autodarts automatic event integration remains a later pass.

### Stage 10.5 — Gaming Widgets — Implemented / runtime validation next

- Combined K/D/A panel.
- Combined Wins/Losses panel.
- Round counter.
- Attempts counter.
- Deaths counter.
- Free-text Personal Best display.
- Combined Session Stats panel with K/D/A and W/L.
- Manual quick controls for kills, deaths, assists, wins and losses.
- Persistent gaming values in `overlays.json`.
- Full compatibility with Designer themes, positions, animations and media.

### Stage 10.6 — Automation and external control — In progress

#### Stage 10.6.1 — Himothee Action Registry — Implemented / build validation next

- Central stable action IDs.
- Structured parameters and result/error objects.
- Action discovery metadata.
- State snapshot foundation.
- Streaming, recording, Replay Buffer, scene, and audio actions.
- Multistream destination actions.
- Overlay/counter/timer actions.
- Darts and gaming actions.
- Startup/profile readiness guard.
- UI-thread execution contract.

#### Stage 10.6.2 — Stream Deck local bridge — Implemented / build validation next

- Loopback HTTP JSON bridge on 127.0.0.1:3294.
- Automatic Himothee detection via health probe.
- Action discovery and execution endpoints.
- State snapshot polling foundation.
- No manual IP configuration.
- Native Stream Deck plugin and live push remain future work.

#### Later Stage 10.6 work

- Native Stream Deck action plugin.
- Stream Deck+ dials/touch controls.
- Live button states.
- OBS hotkeys.
- Media/sound triggers.
- Rules/macros.

### Additional production/release work

- Multistream presets and safer platform defaults.
- Better primary/secondary service presentation.
- Release/signing pipeline.

## Release principle

A stage is complete only when previous OBS functionality still works and the new
feature has a reproducible build and runtime test path.
