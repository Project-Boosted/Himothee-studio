# Himothee Studio Development

## Baseline

Himothee Studio is based on OBS Studio **32.2.2**, commit
ba2f32bdf791005443988a4955e963663e16b1ed.

## Branch strategy

- **master** — upstream-facing branch.
- **himothee-dev** — integration branch for accepted Himothee work.
- Feature branches — one focused development stage at a time.

Recent feature branches:

- feature/branding
- feature/multistream-core
- feature/shared-encoder-mode
- feature/independent-encoders
- feature/destination-resilience
- feature/audio-routing
- feature/unified-chat
- feature/twitch-chat — current Stage 9.2 branch

Feature branches should merge into himothee-dev only after CI and the
required runtime tests pass.

## Current development

Version: **0.9.4**

Focus: **Stage 9.2 — Twitch Account & Live Chat**

Current Windows CI:
https://github.com/Project-Boosted/Himothee-studio/actions/runs/37699863869

## Development rules

1. Keep each change focused.
2. Retain OBS attribution and licensing.
3. Preserve normal single-stream OBS behaviour.
4. Preserve recording, Replay Buffer, Virtual Camera, scenes, sources, and plugins.
5. Prefer extending libobs/OBS abstractions over duplicating them.
6. Isolate destination/provider failures.
7. Add useful lifecycle/error logging without credentials.
8. Never commit stream keys, OAuth tokens, passwords, certificates, or secrets.
9. Treat compile success and runtime success as separate gates.
10. Keep config/profile migration backward compatible where practical.
11. Keep chat integrations behind the common HimotheeChatProvider interface.
12. Reuse platform authentication instead of copying tokens into chat/profile data.

## Build and CI

Each stage-specific Windows workflow should:

- Build x64 RelWithDebInfo.
- Validate Himothee product/version metadata.
- Validate the stage's core implementation.
- Package a portable Windows ZIP.
- Upload a short-retention Actions artifact.

Runtime/network features require local testing because platforms can
reject otherwise valid builds due to account permissions, OAuth scopes,
codec rules, rate limits, or service policy.

## Pull request expectations

Feature PRs should state:

- Problem solved.
- Subsystems changed.
- Test path.
- Effect on standard OBS behaviour.
- Migration/config impact.
- Known limitations.
- Platform scopes/credentials required.
- Runtime testing still outstanding.

## Versioning

Himothee remains in the 0.x range while experimental.

The OBS baseline is documented separately from the Himothee product
version. Runtime hotfixes may increment the Himothee patch version even
when CI compiled the prior build successfully.

## Security

Do not log or commit secrets.

Chat providers should reuse platform auth credentials from the auth
layer and keep access tokens out of the common chat message model.

Public Windows releases should eventually be Authenticode signed and
timestamped. Current CI artifacts are unsigned.
