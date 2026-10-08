# Himothee Studio Roadmap

## Current status

Current development version: **0.9.4**

OBS baseline: **32.2.2**, commit ba2f32bdf791005443988a4955e963663e16b1ed.

The multistream foundation through Stage 8 is implemented. Stage 9 is
building the native Unified Chat system.

## Foundation

### v0.1 — OBS 32.2.2 baseline — Implemented

- Pinned development to OBS Studio 32.2.2.
- Established the Himothee branch strategy.
- Added Windows baseline CI.
- Documented upstream tracking.

### v0.2 — Himothee identity — Implemented

- Product identity and version metadata.
- Separate Himothee configuration/data directory.
- Side-by-side behaviour with stock OBS.
- About dialog attribution.
- Stock OBS updater disabled until Himothee has its own update channel.

## Native multistream

### v0.3 — Multi-output core — Implemented

- Multiple simultaneous secondary outputs.
- Destination state/error lifecycle.
- Shared primary encoder support.
- Simple and Advanced output integration.

### v0.4 — Multistream Manager — Implemented

- Destination add/edit/remove/enable.
- Twitch, YouTube, Kick, and Custom RTMP presets.
- Start/stop controls.
- Individual destination state.

### v0.5 — Shared encoder mode — Implemented

- Shared video/audio encoders.
- Codec compatibility checks.
- Bitrate, health, connection, dropped-frame, and codec telemetry.

### v0.6 — Independent encoder mode — Implemented

- Shared or Independent mode per destination.
- Per-destination video/audio encoder instances.
- Per-destination bitrate and resolution.

### v0.7 — Reliability and telemetry — Implemented

- Reconnect policy.
- Reconnect/error counters.
- Uptime and time in state.
- Manual retry.
- Partial failure isolation.

### v0.8 — Audio routing — Implemented

- OBS Track 1-6 selection per destination.
- Shared-video plus dedicated-audio mode.
- Independent audio encoders.
- Per-destination audio bitrate.

### RTMP compatibility hardening — Implemented

- Detect Shared Encoder conflict with primary Multitrack/Enhanced Broadcasting.
- Show useful errors directly in the Multistream dock.
- Select H.264 for ordinary Twitch/Kick independent RTMP outputs.
- Kick-safe limits: H.264, up to 1920x1080, up to 8000 kbps, CBR, 2-second keyframe.
- Avoid cloning Twitch Enhanced Broadcasting HEVC into ordinary RTMP.
- Detect duplicate use of the primary stream key.

## Unified Chat

### Stage 9.1 — Unified Chat dock and message model — Implemented

- Native Himothee Chat dock.
- All / Twitch / YouTube / Kick filters.
- Common message model.
- Common provider interface.
- Thread-safe bounded history.
- Connection-state summary.
- Send-target/composer foundation.
- Application-level chat manager for later alerts/triggers/overlays.

### Stage 9.2 — Twitch account and live chat — Implemented, runtime validation next

- Reuse the Twitch account connected in OBS/Himothee.
- Refresh and validate Twitch OAuth.
- Check user:read:chat.
- Default to Auto - My Channel.
- Optional Custom Channel login.
- Twitch API channel lookup.
- EventSub WebSocket session handling.
- channel.chat.message subscription.
- Event deduplication and reconnect handling.
- Map Twitch messages into Unified Chat.
- Visible account/channel/error state.
- Windows CI passed for v0.9.4.

### Stage 9.3 — YouTube account and live chat — Next

- Reuse connected YouTube account.
- Discover the current active live broadcast.
- Default to Auto - Current Live Broadcast.
- Optional manual broadcast selection.
- Feed YouTube live chat into the common model.

### Stage 9.4 — Kick account and live chat

- Authenticate/connect Kick account.
- Default to authenticated account channel.
- Optional custom channel.
- Feed Kick chat into the common model.

### Stage 9.5 — Send to one/all platforms

- Send to Twitch, YouTube, or Kick.
- Send one message to all connected platforms.
- Outgoing queue and platform rate-limit handling.
- Partial-send reporting.

### Stage 9.6 — Chat reliability

- Reconnect/backoff hardening.
- Rate-limit telemetry.
- Ordering/deduplication audit.
- Better connection diagnostics.

### Stage 9.7 — Moderation

- Delete message where supported.
- Timeout/ban actions.
- Permission-aware context menu.

### Stage 9.8 — Platform events

- Twitch subscriptions/events.
- YouTube memberships/Super Chats.
- Kick subscriptions/events.
- Common event representation.

### Stage 9.9 — Chat to Alerts/Triggers

- Feed chat/events into Himothee rules.
- Trigger media/macros.
- Overlay/event integrations.

## Production tools

After Multistream and Unified Chat are stable:

- Built-in alerts.
- Media triggers.
- Macros.
- Counters and timers.
- Browser overlay server.
- Stream Deck / external control.
- Production automation.

## Release/security work

Before a public release:

- Windows Authenticode signing.
- Timestamped signatures.
- SHA-256 checksums.
- GitHub Release packaging.
- GPL/source notice audit.
- Migration and clean-install testing.

## Release principle

A stage is complete only when previous OBS functionality still works
and the new feature has a reproducible build and runtime test path.
