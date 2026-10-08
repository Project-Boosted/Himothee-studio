# Stage 10.6.3 — Native Elgato Stream Deck Plugin

Status: source implementation; Windows and hardware validation pending.

## Scope

The source plugin lives under `integrations/streamdeck/`. It uses the official Elgato
Stream Deck Node SDK v3 and connects to Himothee Studio's Stage 10.6.2 local bridge.
There is **no dependency on the Himothee Studio overlay editor being open**, no
OBS WebSocket connection, and no separate browser extension. The Studio executable
must be running.

- Studio bridge: `http://127.0.0.1:3294` (fixed localhost; never exposed to LAN).
- `/v1/health` verifies the name and bridge protocol version.
- `/v1/actions` populates the available action list and parameter definitions.
- `/v1/state` populates destination, scene, and overlay lists plus live key feedback.
- `POST /v1/execute` performs actions. The plugin never retries an ambiguous POST.
- New `scenes` array in the state snapshot allows selecting scene buttons by name.

## Controls (keypad)

| Stream Deck action | Behavior |
| --- | --- |
| Streaming | Explicit start/stop based on Studio's current primary stream status |
| Recording | Explicit start/stop based on recording status |
| Multistream Destination | Select destination by discovered ID; start/stop that output only |
| Switch Scene | Select from the Studio scene list |
| Overlay Visibility | Show/hide the selected overlay |
| Counter + | Increment selected overlay counter (1–100 per press) |
| Timer Command | Start, pause, or reset the selected timer overlay |
| Registry Action | Choose any currently registered action plus parameters, including darts and gaming stats |

Each key polls the actual Studio state at approximately 1.5-second intervals.
Offline keys show `STUDIO / OFFLINE`; unconfigured keys show `SELECT`.
Requests are guarded against concurrent/bounced key presses. Failed actions
trigger Stream Deck's warning feedback and are logged, not silently retried.

The property inspector communicates through the Stream Deck SDK WebSocket,
not directly with the bridge from browser JavaScript. Only the Node plugin
makes HTTP requests to Studio.

## Build on Windows

Prerequisites: Himothee Studio with Stage 10.6.2, Stream Deck 7.1+, Node.js 24+.

```powershell
cd integrations/streamdeck
npm install
npm test
npm run build
npx streamdeck validate com.himothee.studio.sdPlugin --no-update-check
npx streamdeck link com.himothee.studio.sdPlugin
```

Open Stream Deck, find the **Himothee Studio** category and drag a control
onto a key. Choose a destination, scene or overlay in the property inspector.
With Studio open, the status switches from offline to connected.

To make an installer (for Windows Stream Deck users):

```powershell
npm run pack
```

The CLI writes a `.streamDeckPlugin` installer into `integrations/streamdeck/dist`.
The GitHub workflow `streamdeck-plugin.yml` also uploads the package as a
downloadable build artifact after validation.

## Tests

`npm test` covers explicit stream commands, per-destination selection,
counter validation and offline/live feedback without OBS running.

Windows end-to-end test still required:
1. Launch the newly built Studio binary and confirm `/v1/health` responds.
2. Install the packaged plugin in Stream Deck.
3. Stop/start the primary stream from one key; no duplicate start/stop.
4. Start Kick while Twitch or YouTube primary is live and verify only the
   chosen secondary starts; confirm the primary-stream dependency is respected.
5. Change a Studio scene, overlay visibility, counter and timer; check
   physical button status and OBS Browser Source output.
6. Close/reopen Studio; confirm offline/reconnected feedback and no stuck state.
7. Confirm unsupported actions and missing overlay IDs show alerts.

## Limits and follow-up

Current Stage 10.6.3 supports keys, **not Stream Deck+ encoder rotation**.
A dial-specific action, state-push notifications, local client pairing, and
a signed release installer can be added later. Remote/LAN access is out of scope.
The Stream Deck SDK and hardware should be tested on a Windows desktop before
this is called a validated release.
