# Stage 10.2 — Timers & Counters

Himothee Studio v0.10.1 builds the first useful widgets on top of the Stage 10.1 browser-source overlay engine.

## Widget types

### Text

The original Stage 10.1 text overlay remains supported.

### Counter

A general-purpose integer counter.

Controls:

- -1
- +1
- Reset

The generic counter may use negative values.

### Kill Counter

A counter preset intended for game kills.

Controls:

- -1
- +1
- Reset

Kill counts are clamped at zero.

### Streak Counter

A counter for win streaks, kill streaks, checkout streaks, or similar session streaks.

Streak values are clamped at zero.

### Challenge Progress

A current value plus configurable target.

Example:

```text
74 / 100
74% complete
```

The browser overlay also renders a progress bar.

### Countdown

A configurable countdown duration in seconds.

Controls:

- Start
- Pause
- Reset

If a completed countdown is started again, it begins from the configured duration.

### Stopwatch

Count-up timer.

Controls:

- Start
- Pause
- Reset

### Stream Uptime

Tracks the current Himothee/OBS primary streaming session.

It shows:

- OFFLINE when the primary stream is stopped.
- LIVE plus the current session elapsed time while streaming.

Stage 10.2 uptime starts tracking when the overlay engine observes the stream becoming active. It is not reconstructed from a previous process after restarting Himothee Studio.

## Dock changes

The Himothee Overlays dock now includes:

- **New Widget** type picker.
- Type selector for an existing widget.
- Live value column.
- Live value readout in the editor.
- Counter quick controls.
- Timer quick controls.
- Target field for Challenge Progress.
- Duration field for Countdown.
- Existing Show/Hide, Preview, Copy URL, Save, Delete, and Create OBS Source controls.

## Browser renderer

The common transparent browser page understands all Stage 10.2 widget types.

It continues polling the local JSON state endpoint every 250 ms, so changing a counter or timer does not reload the Browser Source.

The current basic renderer displays:

- title,
- live value,
- timer status where relevant,
- progress percentage/bar for Challenge Progress.

Visual theming and layout design remain Stage 10.3.

## Persistence

The existing per-profile `overlays.json` format is extended with:

- `value`
- `target`
- `duration_ms`
- `elapsed_ms`
- `running`
- `started_at_ms`

Older Stage 10.1 Text widgets load with safe defaults.

## Runtime test checklist

After the Windows build passes:

1. Create each widget type.
2. Create an OBS Browser Source from each.
3. Verify +1 / -1 / Reset update counters without Browser Source reload.
4. Verify Challenge Progress updates its percentage and bar.
5. Start, pause, resume and reset Countdown.
6. Start, pause, resume and reset Stopwatch.
7. Start the primary stream and verify Stream Uptime changes to LIVE and counts upward.
8. Stop streaming and verify Stream Uptime returns to OFFLINE.
9. Restart Himothee Studio and verify widget definitions/values persist.
10. Confirm the original Text widget still works.

## Next

Stage 10.3 will add the Overlay Designer: themes, fonts, sizing, backgrounds, opacity, position presets, animations, and visual media layers.
