# Stage 5 — Shared Encoder Production Mode

Stage 5 hardens Himothee Studio's native shared-encoder multistreaming for normal use.

## Shared encoder mode

All enabled secondary RTMP/RTMPS outputs reuse the primary OBS stream's video and audio encoders.

```text
OBS render
   |
   v
shared video/audio encoders
   |
   +--> primary stream
   +--> secondary 1
   +--> secondary 2
   +--> secondary 3
```

This avoids launching another video encode for every destination.

## Compatibility checks

Before a secondary destination is prepared, Himothee Studio now checks the primary video/audio codecs against the codecs supported by the selected OBS output type.

An incompatible destination is placed into an Error state and is not started. The primary stream and compatible destinations are allowed to continue.

## Destination telemetry

The Multistream dock now shows:

- Mode: Shared.
- Connection state.
- Estimated live bitrate.
- Dropped frames.
- Connection time.
- Network health/congestion.
- Total transferred data.
- Shared video/audio codec information in the status tooltip.
- Last destination error in the status tooltip.

## Aggregate health

The dock summary reports:

- Primary stream state.
- Number of live secondary destinations.
- Number of enabled destinations.
- Number currently reconnecting.
- Number in an error state.

## Failure isolation

A secondary destination failure should not deliberately stop:

- The primary stream.
- Other healthy secondary outputs.
- Recording.
- Replay Buffer.

A selected secondary output can be stopped and restarted while the primary stream remains live.

## Test checklist

- [ ] Windows CI build passes.
- [ ] Himothee Studio reports version 0.5.0.
- [ ] One shared secondary starts with the primary stream.
- [ ] Two or more shared secondaries start from the same encoder.
- [ ] Live bitrate updates independently for each destination.
- [ ] Connect time appears after connection.
- [ ] Health/congestion updates while live.
- [ ] Codec information appears in the status tooltip.
- [ ] Invalid/incompatible secondary shows Error without stopping the primary.
- [ ] One failed secondary does not stop another healthy secondary.
- [ ] Selected secondary can stop and restart while primary stays live.
- [ ] Stopping primary stops all secondary outputs.
- [ ] Recording still works.
- [ ] Replay Buffer still works.
- [ ] Stock OBS config remains separate.

## Next stage

Stage 6 will add an optional **Independent Encoder** mode so a destination can use its own encoder, bitrate, and eventually its own resolution instead of sharing the primary encode.
