# Stage 10.6.1 — Himothee Action Registry

Himothee Studio v0.10.7 introduces a central internal action API for native UI controls, future Stream Deck integration, hotkeys, macros, and other external controllers.

## Architecture

```text
Himothee UI / Stream Deck / Hotkeys / Macros
                    |
                    v
          HimotheeActionRegistry
                    |
        +-----------+-----------+
        |           |           |
   OBS controls  Multistream  Overlay Engine
```

Stage 10.6.1 does **not** expose a new network service. Transport and Stream Deck connectivity are Stage 10.6.2.

## Action discovery

The registry provides a discoverable list of actions with:

- stable action ID,
- display name,
- category,
- description,
- parameter schema.

It also provides a state snapshot containing:

- OBS ready state,
- primary streaming state,
- recording state,
- replay-buffer state,
- current scene,
- multistream destination states,
- overlay IDs/types/visibility/live display values.

## Action result

Every action returns a structured result:

```json
{
  "success": true,
  "code": "ok",
  "message": "Counter updated.",
  "data": {}
}
```

Failures return a stable error code and human-readable message.

## Registered actions

### Streaming

- `stream.start`
- `stream.stop`
- `stream.toggle`

### Recording

- `record.start`
- `record.stop`
- `record.toggle`

### Replay Buffer

- `replay.start`
- `replay.stop`
- `replay.toggle`
- `replay.save`

### Scenes

- `scene.switch`

Parameter:

- `scene`

### Audio

- `audio.mute`
- `audio.unmute`
- `audio.toggle`

Parameter:

- `source`

### Multistream

- `destination.start`
- `destination.stop`
- `destination.start_all`
- `destination.stop_all`

Single-destination actions use:

- `destination_id`

The current multistream architecture still requires the OBS primary stream to be live before a secondary can start.

### Overlays

- `overlay.show`
- `overlay.hide`
- `overlay.toggle`

Parameter:

- `overlay_id`

### Counters

- `counter.increment`
- `counter.decrement`
- `counter.reset`

Parameters:

- `overlay_id`
- optional `amount`

### Timers

- `timer.start`
- `timer.pause`
- `timer.reset`

Parameter:

- `overlay_id`

### Darts

- `darts.average.set`
- `darts.checkout.trigger`
- `darts.checkout.clear`

Checkout trigger supports:

- `overlay_id`
- `score`
- optional `route`
- optional `duration_ms`

### Gaming

- `gaming.stat.increment`
- `gaming.stat.decrement`
- `gaming.stats.reset`
- `gaming.pb.set`

Gaming stat values:

- `kills`
- `deaths`
- `assists`
- `wins`
- `losses`

## Startup safety

The registry is created during the main-window construction, but it does not read profile/output state from its constructor.

Actions and state snapshots wait until `OBSBasic::Config()` is available. This preserves the startup-lifecycle safety introduced by the Stage 10 overlay hotfix.

## Threading contract

Stage 10.6.1 executes actions on the Qt application/UI thread.

Calls from future background transports must marshal execution onto the UI thread before invoking `Execute()`. Direct off-thread execution returns `wrong_thread`.

## Stage 10.6.2

The local Stream Deck control bridge is implemented in v0.10.8, pending build and runtime validation:

- localhost-only connection,
- action discovery,
- action execution,
- state polling/push foundation,
- automatic Himothee detection,
- no manual IP configuration.
