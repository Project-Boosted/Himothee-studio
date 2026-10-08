# Himothee Studio Development Changelog

Underlying OBS baseline: OBS Studio 32.2.2.

## 0.9.4 — Twitch live chat

- Added Twitch provider to Unified Chat.
- Reuses the Twitch account connected in OBS/Himothee Studio.
- Refreshes and validates the Twitch OAuth user token.
- Checks the user:read:chat permission.
- Added Auto - My Channel.
- Added optional Custom Channel lookup.
- Added EventSub WebSocket receive.
- Handles welcome, keepalive, reconnect, and revocation.
- Creates channel.chat.message subscriptions.
- Deduplicates EventSub envelopes.
- Maps Twitch chat into the common Himothee timeline.
- Added Connect Twitch / Disconnect Twitch controls.
- Added visible account/channel/error state.
- Message sending remains disabled until Stage 9.5.
- Stage 9.2 Windows CI passed.

## 0.9.3 — RTMP compatibility hardening

- Added platform-safe H.264 selection for Twitch/Kick independent outputs.
- Avoided cloning Enhanced Broadcasting HEVC into ordinary Twitch RTMP.
- Added Kick-safe output limits.
- Added duplicate-primary-stream-key detection without logging keys.
- Runtime testing confirmed the Kick independent secondary remained live.

## 0.9.2 — Multistream diagnostics

- Added visible destination error text.
- Improved Shared Encoder plus Multitrack/Enhanced Broadcasting diagnostics.
- Added clearer recovery instructions.

## 0.9.1 — Startup hotfix

- Fixed early startup crash caused by reading audio-track profile labels
  before the active profile config was ready.
- Audio track selectors now initialise safely and refresh later.

## 0.9.0 — Unified Chat foundation

- Added native Himothee Chat dock.
- Added All / Twitch / YouTube / Kick tabs.
- Added common message model and provider interface.
- Added thread-safe bounded history.
- Added provider-state summary and composer foundation.
- Hosted Chat Manager at application level.

## 0.8.0 — Audio routing

- Added per-destination OBS Track 1-6.
- Added shared-video/dedicated-audio path.
- Added independent routed audio.
- Added audio bitrate and status controls.

## 0.7.0 — Destination resilience

- Added reconnect policies.
- Added reconnect/error counters.
- Added uptime/state timing and Retry Selected.

## 0.6.0 — Independent encoders

- Added Shared/Independent mode.
- Added independent per-destination video/audio encoding.
- Added bitrate and resolution overrides.

## 0.5.0 — Shared encoder production mode

- Added shared-codec validation and detailed health telemetry.

## 0.4.0 — Multistream Manager

- Added native destination-management UI and lifecycle controls.

## 0.3.0 — Native multistream core

- Added multiple secondary RTMP/RTMPS outputs and destination telemetry.

## 0.2.0 — Himothee identity

- Added Himothee branding, version identity, and config separation.

## 0.1.0 — OBS baseline

- Established OBS Studio 32.2.2 baseline and Windows CI.
