# Stage 8 — Per-Destination Audio Routing

Stage 8 allows each Himothee Studio multistream destination to select its own OBS audio mix.

## Audio tracks

Every destination can select one of OBS Track 1 through Track 6.

The Multistream Manager also displays any custom track names configured in OBS Advanced Output settings.

Example:

```text
Twitch
  Video: Shared
  Audio: Track 1
  Mix: Game + Mic + Alerts

YouTube
  Video: Independent
  Audio: Track 2
  Mix: Game + Mic + Music + Alerts

Kick
  Video: Shared
  Audio: Track 3
  Mix: Game + Mic
```

## Encoder behaviour

Video and audio are now handled independently.

### Shared video + shared audio

If a destination selects the same audio mixer as the primary stream and does not override audio bitrate, it reuses the primary audio encoder.

### Shared video + dedicated audio

If a destination:

- selects a different OBS audio track, or
- specifies its own audio bitrate,

Himothee Studio creates a dedicated audio encoder for that destination while continuing to reuse the primary video encoder.

This provides different platform audio mixes without paying for another video encode.

### Independent video

Independent Encoder destinations continue to use their own video encoder and now create their audio encoder from the selected OBS audio track.

## Persistence

The active profile's `multistream.json` now stores:

- `audio_track`

Values are 1 through 6.

Stage 3–7 profiles without this field load as Track 1.

## Multistream Manager

Stage 8 adds:

- Audio Track selector.
- Track 1–6 labels.
- Custom OBS audio track names when configured.
- Audio Track column in destination status.
- Audio routing information in destination tooltips.
- Dedicated/shared audio encoder indication.
- Audio bitrate editing for both Shared and Independent video modes.

## Failure isolation

Failure to create a routed audio encoder puts only that destination into Error state. It does not intentionally stop:

- the primary stream,
- other healthy secondary destinations,
- recording,
- Replay Buffer.

## Test checklist

- [ ] Windows CI build passes.
- [ ] Himothee Studio reports version 0.8.0.
- [ ] Existing Stage 7 profile loads and defaults to Track 1.
- [ ] Track 1 destination streams normally.
- [ ] Track 2 destination sends the Track 2 mix.
- [ ] Track 3–6 can be selected and persisted.
- [ ] Custom OBS track names appear in the selector.
- [ ] Shared video + same audio track can reuse the primary audio encoder.
- [ ] Shared video + different audio track creates a dedicated audio encoder.
- [ ] Shared video + audio bitrate override creates a dedicated audio encoder.
- [ ] Independent video uses the selected audio track.
- [ ] Two destinations can use different OBS audio tracks simultaneously.
- [ ] One routed-audio failure does not stop the primary or other secondaries.
- [ ] Audio track is visible in the live destination table.
- [ ] Recording still works.
- [ ] Replay Buffer still works.

## Next stage

The next major stage can focus on platform presets/integrations for Twitch, YouTube, and Kick so users do not have to manually enter as much RTMP configuration.
