# Stage 2 — Branding and Configuration Separation

Stage 2 establishes Himothee Studio as a distinct application while retaining the OBS Studio 32.2.2 engine and upstream licensing.

## Product identity

- Product name: **Himothee Studio**
- Himothee version: **0.2.0**
- Engine baseline: **OBS Studio 32.2.2**
- Windows executable remains `obs64.exe` during this stage to avoid breaking OBS assumptions before compatibility testing.

## Configuration separation

Himothee Studio uses its own `himothee-studio` user-data tree instead of the stock `obs-studio` tree.

On a normal Windows installation this means Himothee profiles, scenes, logs, crashes, themes, plugin configuration and application settings do not overwrite a user's standard OBS Studio configuration.

Portable mode also uses the Himothee-specific configuration subtree.

## Side-by-side operation

The Windows single-instance mutex has been renamed for Himothee Studio, allowing stock OBS Studio and Himothee Studio to be treated as separate applications.

## Updater safety

The inherited OBS automatic updater is disabled in Himothee Studio v0.2.0. This prevents the fork from accidentally replacing itself with an official OBS build. A Himothee-specific update channel can be added later.

## Attribution

The About dialog identifies Himothee Studio as an independent application based on OBS Studio and retains upstream authors/license information.

## Stage 2 test checklist

- [ ] Windows CI build passes.
- [ ] Windows file metadata reports `Himothee Studio`.
- [ ] Application launches.
- [ ] Main window title starts with `Himothee Studio 0.2.0`.
- [ ] About dialog shows Himothee Studio and OBS attribution.
- [ ] Stock OBS profiles/scenes remain unchanged.
- [ ] Himothee creates and reloads its own profile and scene collection.
- [ ] Himothee and stock OBS can launch side by side.
- [ ] Display/Game/Window Capture still work.
- [ ] Audio devices still work.
- [ ] Recording and replay buffer still work.

## Deferred branding

A custom Himothee application icon and installer identity are intentionally deferred until the functional branding/config separation build passes. This avoids mixing binary asset changes with the first compatibility test.
