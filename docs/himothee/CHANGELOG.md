# Himothee Studio Development Changelog

Underlying OBS baseline: OBS Studio 32.2.2.

## 0.10.1 — Stage 10.2 Timers & Counters

- Added generic Counter widget.
- Added Kill Counter and Streak Counter presets.
- Added Challenge Progress with target/percentage/progress bar.
- Added Countdown with Start/Pause/Reset.
- Added Stopwatch with Start/Pause/Reset.
- Added Stream Uptime tied to the current primary streaming session.
- Added +1 / -1 / Reset quick controls for counter widgets.
- Added live-value status in the Himothee Overlays dock.
- Extended `overlays.json` with counter/timer runtime state.
- Preserved backward compatibility with Stage 10.1 Text overlays.
- Kept browser-source updates live without source reload.


## 0.10.0 — Stage 10.1 Overlay Engine & Dock

- Added native Himothee Overlays dock.
- Added per-profile overlay persistence in `overlays.json`.
- Added localhost-only overlay HTTP server on port 3293.
- Added stable `/overlay/<id>` browser-source pages.
- Added `/api/overlay/<id>` live state endpoints.
- Added the first transparent Text overlay renderer.
- Added New/Delete/Save and Show/Hide controls.
- Added browser Preview and Copy URL.
- Added Create OBS Source for the current scene.
- Added per-overlay browser width/height.
- Kept the overlay engine independent from Botrix chat and the Multistream Manager.


## 0.9.6 — Multistream control and resolution audit

- Separated destination Enabled from Auto-start.
- Enabled destinations are prepared with the primary stream even when Auto-start is off.
- Only Auto-start destinations go live automatically with the primary.
- Added Go Live Enabled and End Secondaries controls.
- Added Go Live Selected and End Selected live controls.
- Added explicit OBS encoder Input and actual destination Output resolution reporting.
- Kept Independent destination output width/height configurable per platform.
- Added Kick 1080p validation and clearer Twitch 1440p/Enhanced Broadcasting guidance.
- Documented the intended Twitch 2560x1440 primary + Kick 1920x1080 secondary setup.
- Kept the current architectural limitation that secondaries still require the OBS primary streaming session.


## 0.9.5 — Native chat removal / Botrix custom dock

- Removed the experimental Himothee Unified Chat manager.
- Removed the native Himothee Chat dock.
- Removed the Twitch EventSub provider.
- Removed the Qt WebSockets dependency that was added only for native chat.
- Removed chat-only Twitch OAuth accessors/helpers.
- Removed Stage 9.1/9.2 native-chat workflows and documentation.
- Retained Multistream Manager, audio routing, resilience, and RTMP compatibility work.
- Standardised multiplatform chat on Botrix Multi Chat through Custom Browser Docks.
- Added Botrix custom-dock documentation.

The earlier native-chat work remains only in experimental branch history and is
not part of the forward product path.

## 0.9.3 — RTMP compatibility hardening

- Added platform-safe H.264 selection for Twitch/Kick independent outputs.
- Avoided cloning Enhanced Broadcasting HEVC into ordinary Twitch RTMP.
- Added Kick-safe output limits.
- Added duplicate-primary-stream-key detection without logging keys.
- Runtime testing confirmed Kick independent secondary streaming remained live.

## 0.9.2 — Multistream diagnostics

- Added visible destination error text.
- Improved Shared Encoder + Multitrack/Enhanced Broadcasting diagnostics.

## 0.9.1 — Startup hotfix

- Fixed early startup crash caused by reading profile audio-track labels before
  the active profile config was ready.

## 0.8.0 — Audio routing

- Added per-destination OBS Track 1-6.
- Added shared-video/dedicated-audio and independent routed-audio paths.

## 0.7.0 — Destination resilience

- Reconnect policies, counters, uptime/state timing, and Retry Selected.

## 0.6.0 — Independent encoders

- Shared/Independent mode plus bitrate/resolution overrides.

## 0.5.0 — Shared encoder production mode

- Shared-codec validation and detailed health telemetry.

## 0.4.0 — Multistream Manager

- Native destination-management UI and lifecycle controls.

## 0.3.0 — Native multistream core

- Multiple secondary RTMP/RTMPS outputs and telemetry.

## 0.2.0 — Himothee identity

- Branding, version identity, and config separation.

## 0.1.0 — OBS baseline

- OBS Studio 32.2.2 baseline and Windows CI.
