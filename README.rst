Himothee Studio
===============

Himothee Studio is a streaming and production application based on OBS Studio:
https://github.com/obsproject/obs-studio

The project starts from OBS Studio 32.2.2 and focuses on native
multi-destination streaming and production tooling while retaining OBS capture,
scenes, sources, audio, recording, Replay Buffer, encoders, plugins, and
browser/custom-dock functionality.

Project Status
--------------

Early development / pre-release.

Current Himothee Studio development version: 0.10.0

Underlying OBS baseline: OBS Studio 32.2.2
(commit ba2f32bdf791005443988a4955e963663e16b1ed).

The experimental native Unified Chat/Twitch EventSub implementation has been
removed. Multiplatform chat is intentionally handled through Botrix Multi Chat
inside a normal OBS/Himothee Custom Browser Dock.

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
* Duplicate-primary-stream-key protection.
* Separate destination Enabled and Auto-start behaviour.
* Per-destination live Start/Stop controls plus Start All Enabled / Stop Secondaries.
* Live Input and Output resolution reporting for each destination.
* Independent per-destination output scaling, such as Twitch 2560x1440 primary + Kick 1920x1080.
* Normal OBS Custom Browser Docks for external tools such as Botrix Multi Chat.
* Native Himothee Overlays dock.
* Localhost transparent browser-source overlay engine.
* Per-profile overlay persistence.
* Show/Hide, Preview, Copy URL, and Create OBS Source controls.

Chat
----

Himothee Studio no longer maintains its own Twitch/YouTube/Kick chat API stack.

Recommended setup:

* Use Botrix Multi Chat.
* Add Botrix through Docks > Custom Browser Docks.
* Position the dock anywhere in the Himothee Studio layout.

See docs/himothee/BOTRIX_CUSTOM_DOCK.md.

Development Roadmap
-------------------

v0.1 - OBS Studio 32.2.2 baseline.
v0.2 - Himothee identity and config separation.
v0.3 - Native multi-output core.
v0.4 - Multistream Manager.
v0.5 - Shared encoder production mode.
v0.6 - Independent encoders.
v0.7 - Destination resilience and telemetry.
v0.8 - Per-destination audio routing.
v0.9.x - Runtime/RTMP compatibility hardening, native-chat removal, and live destination control audit.
v0.10.0 - Stage 10.1 Overlay Engine + Himothee Overlays dock.

Next overlay stages add timers, counters, kill counters, darts widgets,
themes/animations, hotkeys, Stream Deck actions, and automation.

Security
--------

Never commit stream keys, OAuth tokens, passwords, certificates, private
Botrix URLs/tokens, or other secrets.

Current experimental Windows artifacts are unsigned and may trigger browser or
Windows reputation warnings until a signing pipeline exists.

Upstream and License
--------------------

Himothee Studio is an independent derivative of OBS Studio and is not an
official OBS Project distribution.

This repository retains the OBS Studio GNU General Public License version 2 or
later licensing. See COPYING and applicable source notices.
