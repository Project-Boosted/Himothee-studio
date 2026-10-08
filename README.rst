Himothee Studio
===============

Himothee Studio is a streaming and production application based on
OBS Studio: https://github.com/obsproject/obs-studio

The project starts from OBS Studio 32.2.2 and adds native
multi-destination streaming, unified multiplatform chat, and production
tooling while retaining OBS capture, scenes, sources, audio, recording,
Replay Buffer, encoders, and plugin support.

Project Status
--------------

Early development / pre-release.

Current Himothee Studio development version: 0.9.4

Underlying OBS baseline: OBS Studio 32.2.2
(commit ba2f32bdf791005443988a4955e963663e16b1ed).

The integration branch is himothee-dev. Current feature work is on
feature/twitch-chat. The master branch is kept close to upstream OBS.

Current Features
----------------

* Native Multistream Manager.
* Twitch, YouTube, Kick, and Custom RTMP destination configuration.
* Shared-encoder streaming.
* Independent per-destination video/audio encoders.
* Per-destination bitrate and output resolution.
* Per-destination OBS Track 1-6 audio routing.
* Reconnect policy, bitrate, dropped frames, uptime, congestion, and errors.
* Platform-safe H.264 handling for ordinary Twitch and Kick RTMP outputs.
* Protection against accidentally duplicating the primary stream key.
* Native Himothee Chat dock.
* All / Twitch / YouTube / Kick chat filters.
* Common chat message/provider architecture.
* Twitch live-chat receive through EventSub WebSockets.
* Twitch Auto - My Channel mode and optional Custom Channel mode.

Recent Development
------------------

v0.9.0
  Added the native Unified Chat dock and common chat architecture.

v0.9.1
  Fixed an early-startup Multistream config-access crash.

v0.9.2
  Added visible Multistream diagnostics and better shared/multitrack errors.

v0.9.3
  Added Twitch/Kick RTMP compatibility hardening, including platform-safe
  H.264 handling and Kick-safe output limits.

v0.9.4
  Added Twitch account detection, Auto - My Channel, OAuth validation,
  EventSub WebSocket chat receive, reconnect handling, and message mapping.

The v0.9.4 Windows CI build completed successfully.

Development Roadmap
-------------------

v0.1 - OBS Studio 32.2.2 baseline.

v0.2 - Himothee Studio identity and config separation.

v0.3 - Native multi-output streaming core.

v0.4 - Multistream Manager UI.

v0.5 - Shared-encoder production mode.

v0.6 - Independent per-destination encoders.

v0.7 - Destination resilience and telemetry.

v0.8 - Per-destination audio routing.

Stage 9.1 - Unified Chat dock and common message model.

Stage 9.2 - Twitch account and live chat receive.

Next: YouTube live chat, Kick live chat, one/all-platform sending,
moderation, platform events, and Chat-to-Alerts/Triggers integration.

Documentation
-------------

Project documentation is in docs/himothee/:

* DEVELOPMENT.md - workflow and branch strategy.
* ROADMAP.md - implementation plan and current progress.
* CHANGELOG.md - Himothee development-version changes.
* MULTISTREAM_ARCHITECTURE.md - multi-output architecture.
* STAGE8_AUDIO_ROUTING.md - per-destination audio routing.
* STAGE9_1_UNIFIED_CHAT.md - Unified Chat foundation.
* STAGE9_2_TWITCH_CHAT.md - Twitch live-chat integration.
* UPSTREAM.md - OBS upstream tracking.

Building
--------

Himothee-specific Windows CI workflows build and package each stage
using the OBS Studio 32.2.2 build system.

Current Stage 9.2 Windows CI:
https://github.com/Project-Boosted/Himothee-studio/actions/runs/37699863869

Security
--------

Never commit stream keys, OAuth access/refresh tokens, passwords,
certificates, or other secrets.

Current experimental Windows artifacts are unsigned and may trigger
browser or Windows reputation warnings until a signing pipeline exists.

Upstream OBS Studio
-------------------

Himothee Studio is a derivative of OBS Studio. OBS Studio is developed
by the OBS Project and its contributors. Himothee Studio is independent
and is not an official OBS Project distribution.

License
-------

This repository retains the OBS Studio GNU General Public License
version 2 or later licensing. See COPYING and applicable source notices.
