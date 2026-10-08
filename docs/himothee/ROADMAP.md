# Himothee Studio Roadmap

## Current status

Current development version: **0.9.6**

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

## Next production stages

- Alerts/event handling where useful.
- Media triggers.
- Macros.
- Counters and timers.
- Browser overlay server.
- Stream Deck / external control.
- Multistream presets and safer platform defaults.
- Better primary/secondary service presentation.
- Release/signing pipeline.

## Release principle

A stage is complete only when previous OBS functionality still works and the new
feature has a reproducible build and runtime test path.
