# Stage 10.6.4 — Stream Deck+ Dials and Advanced Controls

**Version 0.10.10 · Source implemented · CI and hardware verification recorded separately**

Stage 10.6.4 extends the existing Stage 10.6.3 native plugin. It does not
replace the Stage 10.6.1 Action Registry or Stage 10.6.2 localhost bridge.

## Three new Stream Deck+ encoder actions

| Encoder action | Rotate | Push / tap | Long-touch |
| --- | --- | --- | --- |
| Counter Dial | Clockwise +, anticlockwise − (1, 2, 5 or 10 per tick) | Increment by selected step | Reset **only if enabled** |
| Scene Selector Dial | Preview the previous/next registered scene, including wraparound | Switch to the previewed scene | No action |
| Timer Dial | Clockwise starts, anticlockwise pauses | Toggle between start and pause using Studio's actual running state | Reset **only if enabled** |

The Stream Deck+ touchscreen displays the dial's name and live Studio value
using the official Elgato built-in `$A1` feedback layout. Offline status is
shown directly on the touch strip. Scenes remain unchanged during rotation and
only switch on a push/tap. Dials ignore rotations while pressed.

The key-based Counter action now supports increment, decrement and reset
as separately selectable **explicit** commands. Existing counter keys continue
to increment by default, with previous settings preserved.

## Protocol and safety

- Studio's `/v1/state` overlays now include `counter`, `timer` and `running`.
  The new properties are optional to older key actions; timer toggling
  requires `running` and fails safely if the bridge is too old.
- Physical dial rotations are signed integers. Reject invalid tick values
  or a single tick event above 24 steps; per-step values are limited to 1–10.
- Positive and negative rotations produce explicit
  `counter.increment`/`counter.decrement` registry actions.
  Requests are broken into chunks of at most 100, queued in order for
  each dial context (up to 24 pending requests), and executed once.
- Failed or indeterminate POST requests are **never retried automatically**
  because a retry could increment a counter twice. Display a dial alert.
- Long-touch reset is **disabled by default** for counters and timers.
- The plugin never opens a LAN listener, does not contact OBS WebSocket,
  and continues to use localhost only: `127.0.0.1:3294`.
- Existing keypad controls and OBS overlay/browser-source port 3293 are retained.

## Install or update (Windows)

1. Launch a Himothee Studio build containing Stage 10.6.4's state snapshot.
2. Build the Stream Deck plugin from `integrations/streamdeck`:

```powershell
npm install
npm test
npm run typecheck
npm run build
npx streamdeck validate com.himothee.studio.sdPlugin --no-update-check
npx streamdeck link com.himothee.studio.sdPlugin
```

Alternatively, download the packaged `.streamDeckPlugin` from the
**Himothee Stream Deck Plugin** GitHub Actions workflow artifact.
Install it and drag one of the three new dial actions to a Stream Deck+ dial.
Choose its overlay in the property inspector. Scene dials browse the Studio
scene list automatically.

## Verification checklist

- `npm test` passes dial direction, bounds, scene wraparound, timer state
  and reset opt-in checks.
- `npm run typecheck` validates the official SDK dial event types.
- Elgato CLI must validate the new `Encoder` manifest actions.
- On physical Stream Deck+, confirm clockwise/anticlockwise counter
  movement and fast turns do not double-apply.
- Test +/- increments on darts 180 and gaming kill counters.
- Confirm scene preview never changes live output before press/touch.
- Confirm reset cannot trigger from a normal dial press or short touch.
- Verify timer start/pause status, offline/reconnect feedback, and
  separate Twitch/YouTube/Kick controls on existing keypad actions.

**Not yet included:** cross-LAN pairing, state-push websocket, signed installer,
or a bundled Stream Deck profile. Stream Deck+ hardware validation is required
before declaring production readiness.
