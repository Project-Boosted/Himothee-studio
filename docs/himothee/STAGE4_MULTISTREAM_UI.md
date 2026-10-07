# Stage 4 — Multistream Manager UI

Stage 4 adds the first native user interface for Himothee Studio multistreaming.

## What this stage adds

The main Himothee Studio window now contains a **Multistream** dock.

The dock lets the user:

- Add and remove destinations.
- Set a friendly destination name.
- Choose a platform label: Custom RTMP, Twitch, YouTube, or Kick.
- Enable or disable each destination.
- Set an RTMP or RTMPS server.
- Enter a stream key with password masking.
- Enable optional username/password authentication.
- Override reconnect count and reconnect delay, or inherit normal OBS settings.
- Save destination settings into the active Himothee profile.
- Reload the saved destination list.
- See live per-destination state.
- See dropped frames and transferred data.
- See the last output error as a tooltip.
- Start or stop the overall stream from the Multistream dock.
- Start or stop a selected secondary destination while the primary stream is live.

## Configuration storage

The UI writes the same profile-level `multistream.json` file introduced in Stage 3.

Stream keys and passwords are masked in the UI and must never be committed to the repository or written to normal logs.

## Shared encoder behaviour

Stage 4 still uses the Stage 3 shared-encoder architecture:

```text
Primary OBS video/audio encoders
            |
            +--> Primary stream
            +--> Secondary destination 1
            +--> Secondary destination 2
            +--> Secondary destination 3
```

This stage does not yet add per-destination encoders or per-destination resolutions.

## Test checklist

- [ ] The Multistream dock appears in the Docks menu.
- [ ] The dock can be shown/hidden and moved like other OBS docks.
- [ ] A destination can be added.
- [ ] A destination can be removed.
- [ ] Server/key values save and reload correctly.
- [ ] Stream keys/passwords are masked by default.
- [ ] Enabled/disabled destinations persist.
- [ ] The main Start Streaming button in the dock starts the normal OBS primary output.
- [ ] Enabled secondary destinations start after the primary output starts.
- [ ] Live state updates in the dock.
- [ ] Dropped-frame and data counters update.
- [ ] A selected secondary output can be stopped without stopping the primary output.
- [ ] A stopped selected secondary output can be started again while the primary output remains live.
- [ ] Stopping the primary stream stops all secondary outputs.
- [ ] A failed secondary destination does not stop healthy destinations.
- [ ] Recording still works while multistreaming.
- [ ] Replay Buffer still works while multistreaming.
- [ ] Stock OBS configuration remains untouched.

## Current limitations

- Platform selection is currently a label/preset category; server URLs are still entered manually.
- OAuth account login is not part of this stage.
- Per-destination bitrate, resolution, codec, and encoder selection are not part of this stage.
- Per-destination audio routing is not part of this stage.
- Secondary destinations are RTMP/RTMPS only.
