# Stage 3 — Native Multistream Core

Stage 3 introduces Himothee Studio's first native multi-destination streaming engine.

## Scope

This stage is the **core**, not the final user interface.

The standard OBS streaming path remains the primary output. Himothee can additionally create one or more secondary RTMP outputs that reuse the primary stream's active video and audio encoders.

## Shared encoder architecture

```text
OBS scene/render pipeline
          |
          v
   video/audio encoders
          |
          +----> Primary OBS stream
          |
          +----> Himothee destination 1
          |
          +----> Himothee destination 2
          |
          +----> Himothee destination 3
```

OBS's encoder implementation supports multiple encoded-packet callbacks, so secondary outputs do not require a second video encode in shared mode.

## Destination configuration

Until the Stage 4 Multistream Manager UI is implemented, destinations are read from `multistream.json` in the active Himothee profile directory.

Example:

```json
{
  "destinations": [
    {
      "id": "youtube",
      "name": "YouTube",
      "enabled": true,
      "server": "rtmps://YOUR_INGEST_SERVER/app",
      "key": "YOUR_STREAM_KEY",
      "use_auth": false,
      "username": "",
      "password": "",
      "max_retries": -1,
      "retry_delay_seconds": -1
    },
    {
      "id": "kick",
      "name": "Kick",
      "enabled": false,
      "server": "rtmps://YOUR_INGEST_SERVER/app",
      "key": "YOUR_STREAM_KEY",
      "use_auth": false,
      "username": "",
      "password": "",
      "max_retries": -1,
      "retry_delay_seconds": -1
    }
  ]
}
```

**Never commit a real stream key to GitHub.**

A value of `-1` for retry settings means inherit the normal OBS output setting.

## Runtime behaviour

Each destination has independent runtime state:

- Disabled
- Idle
- Prepared
- Starting
- Active
- Reconnecting
- Stopping
- Error

A secondary destination failing to connect does not intentionally stop healthy destinations or the primary stream.

The manager records byte count, total frames, dropped frames, state, and last error for later use by the Stage 4 UI.

## Current limitations

- Secondary destinations use Custom RTMP/RTMPS.
- Shared mode uses the primary video encoder and primary audio encoder.
- Per-destination bitrate/resolution/encoder settings are not part of this stage.
- Per-destination audio tracks are not part of this stage.
- OBS multitrack-video output is excluded from shared-destination mode for now.
- Destination editing is JSON-only until Stage 4.
- OAuth/account integrations are not part of Stage 3.

## Stage 3 test

1. Build and launch Himothee Studio v0.3.0.
2. Configure the normal primary OBS stream.
3. Add one safe test destination to the profile's `multistream.json`.
4. Start Streaming.
5. Confirm both the primary stream and secondary destination become live.
6. Stop Streaming and confirm both outputs stop.
7. Repeat with two secondary destinations.
8. Intentionally use an invalid endpoint for one secondary and verify the other streams remain active.
9. Verify reconnect behaviour.
10. Verify recording and replay buffer still operate while multistreaming.

## Security

Stream keys and authentication passwords are secrets. They must never be written to logs or committed to the repository.
