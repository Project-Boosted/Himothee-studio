# Stage 10.4 — Darts Widgets

Himothee Studio v0.10.4 adds the first darts-specific widgets to the existing localhost Browser Source overlay engine.

## Widgets

### Darts 180 Counter

Manual session counter for 180s.

Controls:

- -1
- +1
- Reset

The default style uses the Neon preset values.

### Darts 140+ Counter

Manual counter for 140+ visits.

Controls:

- -1
- +1
- Reset

### Darts 100+ Counter

Manual counter for 100+ visits.

Controls:

- -1
- +1
- Reset

### Darts Legs Won

Manual legs-won counter.

Controls:

- -1
- +1
- Reset

### Darts Match Wins

Manual match-win counter.

Controls:

- -1
- +1
- Reset

### Darts Average

Displays a two-decimal 3-dart average.

The value is editable in the Himothee Overlays dock.

Example:

```text
AVERAGE
93.90
3-DART AVG
```

### Darts Checkout

Short-lived animated checkout notification.

Editable values:

- checkout score from 0–170,
- checkout route text,
- display duration from 1–15 seconds.

Controls:

- Trigger
- Clear

Example:

```text
CHECKOUT
141
T20 T19 D12
```

The widget can remain enabled in OBS but is only rendered while its checkout notification is active.

## Browser behaviour

Existing overlay URLs remain stable:

```text
http://127.0.0.1:3293/overlay/<id>
```

Darts counters and averages update through the same JSON polling path used by existing widgets.

Checkout notifications use a runtime visibility window. The configured overlay remains enabled, while the Browser Source becomes visible only for the requested notification duration.

## Persistence

Stage 10.4 extends `overlays.json` with:

- `decimal_value`
- `checkout_score`
- `checkout_route`
- `notification_duration_ms`

The temporary checkout visibility deadline is not persisted across restart.

Older Text, Timer, Counter, Progress, Uptime, and Designer widgets remain compatible.

## First-pass workflow

The darts widgets are manual in v0.10.4. This gives the UI and browser renderer a stable runtime before connecting a live darts source.

## Next darts integration

A later darts event pass can connect HimotheeLink / Autodarts so the widgets update automatically:

- 180 detected -> increment Darts 180 Counter,
- 140+ visit -> increment Darts 140+ Counter,
- 100+ visit -> increment Darts 100+ Counter,
- leg won -> increment Legs Won,
- match won -> increment Match Wins,
- live average -> update Darts Average,
- checkout -> trigger Darts Checkout notification.

## Runtime test checklist

1. Create each darts widget type.
2. Create an OBS Browser Source.
3. Test +1 / -1 / Reset on all darts counters.
4. Enter an average and Save; verify Browser Source updates.
5. Configure a checkout score/route and press Trigger.
6. Verify checkout appears, animates, and hides automatically.
7. Trigger again after it has hidden.
8. Test Clear while a checkout is visible.
9. Restart Himothee Studio and verify persistent darts values/settings return.
10. Confirm Stage 10.1–10.3 widgets still load normally.
