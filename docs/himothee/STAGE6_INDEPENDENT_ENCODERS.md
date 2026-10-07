# Stage 6 — Independent Encoder Mode

Stage 6 adds optional per-destination encoders to Himothee Studio multistreaming.

## Encoder modes

Each destination can now use one of two modes:

### Shared Encoder

The destination reuses the primary stream's video and audio encoders.

Use this when:

- All destinations can use the same codec/quality/resolution.
- Low GPU/CPU usage is the priority.
- The same encoded stream is suitable for every platform.

### Independent Encoder

The destination gets its own video and audio encoder instances.

The first implementation clones the primary encoder type and current encoder settings, then applies destination-specific overrides.

Independent mode currently supports:

- Per-destination video bitrate.
- Per-destination audio bitrate.
- Per-destination output width.
- Per-destination output height.
- Independent output start/stop/reconnect/error state.

A value of **Match primary** leaves the cloned primary setting unchanged.

## Example

A typical setup can now be:

```text
Primary Twitch
  1920x1080
  8000 kbps
  NVENC

YouTube secondary — Independent
  2560x1440
  12000 kbps
  cloned NVENC encoder

Kick secondary — Shared
  same encoder as primary
```

## Resource usage

Shared mode sends the same encoded packets to multiple outputs and is the lowest-overhead option.

Independent mode performs an additional encode for each independent destination. Hardware encoders can have simultaneous-session limits depending on the GPU, driver, and encoder type.

If an independent encoder cannot be created or started, the destination enters Error state without intentionally stopping the primary stream or healthy secondary outputs.

## Persistence

The profile-level `multistream.json` now stores:

- `encoder_mode`
- `video_bitrate_kbps`
- `audio_bitrate_kbps`
- `output_width`
- `output_height`

Existing Stage 3–5 destination files remain compatible and default to Shared mode.

## Multistream dock

The destination editor now exposes:

- Encoder mode: Shared Encoder / Independent Encoder.
- Video bitrate.
- Audio bitrate.
- Output width.
- Output height.

Independent-only settings are disabled while Shared mode is selected.

The destination table shows whether each output is running in Shared or Independent mode.

## Test checklist

- [ ] Windows CI build passes.
- [ ] Application reports Himothee Studio 0.6.0.
- [ ] Existing Stage 5 profiles load as Shared mode.
- [ ] Shared output still works.
- [ ] Independent output starts using cloned primary encoder types.
- [ ] Independent video bitrate override works.
- [ ] Independent audio bitrate override works.
- [ ] Independent resolution override works.
- [ ] One Shared and one Independent destination can run together.
- [ ] Two Independent destinations can run if hardware/software encoder capacity permits.
- [ ] Stopping one Independent destination does not stop the primary.
- [ ] Independent encoder failure does not stop healthy destinations.
- [ ] Recording remains functional.
- [ ] Replay Buffer remains functional.

## Current limitation

This stage does not yet expose a completely different encoder type per destination. Independent mode starts by cloning the primary encoder type because that gives us a safer compatibility path.

A later encoder-settings pass can add explicit per-destination encoder selection and advanced encoder properties after this lifecycle is proven stable.
