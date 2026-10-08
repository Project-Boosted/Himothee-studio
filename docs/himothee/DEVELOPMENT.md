# Himothee Studio Development

## Baseline

Himothee Studio is based on OBS Studio **32.2.2**, commit
ba2f32bdf791005443988a4955e963663e16b1ed.

## Branch strategy

- master — upstream-facing branch.
- himothee-dev — integration branch for accepted Himothee work.
- feature branches — focused development stages.

Current feature branch:

- feature/gaming-widgets

The earlier feature/unified-chat and feature/twitch-chat branches are
experimental history and are not the forward product path.

## Current development

Version: **0.10.6**

Focus: **Stage 10.5 — Gaming Widgets**.

The overlay engine now includes reusable K/D/A, Wins/Losses, Session Stats,
round/attempt/death counters, and a free-text Personal Best display. Stage 10.5
continues using the same localhost Browser Source runtime, designer, startup
lifecycle guards, and automatic port fallback introduced in earlier Stage 10
builds.

## Development rules

1. Keep changes focused.
2. Retain OBS attribution and licensing.
3. Preserve normal single-stream OBS behaviour.
4. Preserve recording, Replay Buffer, Virtual Camera, scenes, sources, and plugins.
5. Prefer extending libobs/OBS abstractions over duplicating them.
6. Isolate destination failures.
7. Add useful logging without credentials.
8. Never commit stream keys, OAuth tokens, passwords, certificates, or private Botrix URLs/tokens.
9. Treat compile success and runtime success as separate gates.
10. Keep config/profile migration backward compatible where practical.
11. Use OBS Custom Browser Docks for external web tools instead of duplicating platform chat APIs unless there is a strong product reason.

## Build and CI

Windows workflows should:

- Build x64 RelWithDebInfo.
- Validate Himothee product/version metadata.
- Validate the relevant feature/removal.
- Package a portable ZIP.
- Upload a short-retention Actions artifact.

## Security

Do not log or commit secrets.

Botrix account/session URLs should be treated as private if they contain
authentication/session identifiers.

Public Windows releases should eventually be Authenticode signed and timestamped.
