# Himothee Studio Development Changelog

Underlying OBS baseline: OBS Studio 32.2.2.

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
