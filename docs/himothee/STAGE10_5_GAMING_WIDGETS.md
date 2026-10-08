# Stage 10.5 — Gaming Widgets

Himothee Studio v0.10.6 adds reusable gaming widgets to the existing Overlay Engine.

## Gaming K/D/A

Displays:

```text
K / D / A
12 / 4 / 8
KILLS / DEATHS / ASSISTS
```

Editable values:

- Kills
- Deaths
- Assists

Quick controls:

- + Kill
- + Death
- + Assist
- Reset Stats

## Gaming Wins / Losses

Displays:

```text
W / L
6 / 2
WINS / LOSSES
```

Quick controls:

- + Win
- + Loss
- Reset Stats

## Gaming Round

Simple non-negative round counter.

Controls:

- -1
- +1
- Reset

## Gaming Attempts

Simple non-negative attempts counter.

Controls:

- -1
- +1
- Reset

## Gaming Deaths

Simple non-negative deaths counter for games where deaths/respawns are the primary tracked stat.

Controls:

- -1
- +1
- Reset

## Gaming Personal Best

Free-text PB widget.

Examples:

```text
1:42.53
Rank 12
107.3 AVG
Wave 38
42 Kills
```

## Gaming Session Stats

Combined panel:

```text
SESSION STATS
K 12  D 4  A 8
W 6  L 2
```

Quick controls:

- + Kill
- + Death
- + Assist
- + Win
- + Loss
- Reset Stats

## Designer compatibility

All Stage 10.5 gaming widgets support the existing Stage 10.3 designer features:

- themes,
- fonts,
- colours,
- opacity,
- rounded corners,
- nine-position placement,
- entry animations,
- image/GIF/video media layers.

## Browser Source runtime

Gaming widgets use the same stable Browser Source URL model:

```text
http://127.0.0.1:<active-port>/overlay/<id>
```

The v0.10.5 automatic port fallback remains active. Himothee prefers port 3293 and uses the first free port through 3313.

## Persistence

Stage 10.5 extends `overlays.json` with:

- `gaming_kills`
- `gaming_deaths`
- `gaming_assists`
- `gaming_wins`
- `gaming_losses`
- `personal_best`

Older overlay profiles remain compatible.

## Runtime test checklist

1. Create Gaming K/D/A.
2. Test + Kill / + Death / + Assist / Reset Stats.
3. Create Gaming Wins / Losses and test both quick controls.
4. Create Round, Attempts, and Deaths counters and test -1/+1/Reset.
5. Create Personal Best and test several free-text values.
6. Create Session Stats and verify K/D/A plus W/L update together.
7. Create OBS Browser Sources and confirm updates happen without source reload.
8. Test designer themes, position and animations on gaming widgets.
9. Restart Himothee Studio and confirm gaming stats persist.
10. Confirm the v0.10.5 overlay port fallback still works if 3293 is occupied.

## Next

Stage 10.6 adds automation and control integration:

- OBS hotkeys,
- Stream Deck/external actions,
- overlay trigger rules,
- media/sound triggers,
- counter/timer actions,
- reusable macros.
