# Stage 10.1 — Overlay Engine & Dock

Himothee Studio v0.10.0 introduces the first native overlay-control foundation.

## Architecture

The overlay system is split into two layers:

1. **Himothee Overlays dock** — native controls inside Himothee Studio.
2. **Transparent browser-source renderer** — HTML served from a local Himothee HTTP server.

This keeps runtime graphics separate from the control UI and gives future widgets a common rendering path.

## Local overlay server

The first implementation listens on:

```text
http://127.0.0.1:3293
```

Each overlay receives a stable URL:

```text
http://127.0.0.1:3293/overlay/<overlay-id>
```

A JSON state endpoint is also available internally:

```text
http://127.0.0.1:3293/api/overlay/<overlay-id>
```

The browser overlay polls its state endpoint without reloading the Browser Source.

The server only binds to localhost.

## Persistence

Overlay definitions are stored per OBS/Himothee profile in:

```text
overlays.json
```

Stage 10.1 stores:

- ID
- name
- type
- visible state
- browser width
- browser height
- title
- text/value

## Dock controls

The **Himothee Overlays** dock supports:

- New Overlay
- Delete
- Save
- Show / Hide
- Preview in the system browser
- Copy URL
- Create OBS Source

The first renderer type is a basic **Text** overlay. It is intentionally simple so later stages can build timer/counter/darts/game widgets on the same engine.

## Create OBS Source

**Create OBS Source** creates a normal OBS Browser Source in the current scene with:

- the overlay's localhost URL,
- the overlay's configured width,
- the overlay's configured height,
- transparent page background.

Existing OBS scene transform/position controls remain responsible for positioning in Stage 10.1.

## Planned follow-up

### Stage 10.2

- Countdown timer
- Count-up / stopwatch
- Generic counter
- Kill counter
- Streak counter
- Challenge/progress counter
- Stream uptime

### Stage 10.3

- Theme/style designer
- Font/size/background/opacity
- Position presets and custom positioning
- Animations
- Image/GIF/video visual layers

### Stage 10.4

- Darts 180 counter
- 140+ / 100+ counts
- legs/wins/averages
- checkout notifications
- HimotheeLink/Autodarts event integration

### Stage 10.5

- Gaming presets
- kills/deaths/assists
- wins/losses
- attempts/deaths/PBs

### Stage 10.6

- OBS hotkeys
- Stream Deck actions
- media triggers
- sounds
- rules/macros

## Stage 10.1 runtime test

After the Windows build passes:

1. Open **Docks > Himothee Overlays**.
2. Create a new overlay.
3. Enter a title and text.
4. Press **Preview**.
5. Confirm the transparent overlay page renders.
6. Press **Create OBS Source**.
7. Confirm the Browser Source appears in the current scene.
8. Toggle **Hide/Show** and confirm the source updates without reloading.
9. Restart Himothee Studio and confirm the overlay definition persists.
