# Multistream Live Controls and Resolution

Himothee Studio v0.9.6 separates destination availability from automatic start behaviour and makes the video scaling path explicit.

## Destination controls

Each secondary destination has two independent settings:

- **Destination enabled** — the destination is prepared and is available to start during the streaming session.
- **Go live automatically with primary stream** — the destination starts automatically when the OBS primary stream starts.

This allows an enabled destination to remain ready without going live automatically.

While the primary stream is live the Multistream dock provides:

- **Go Live Enabled** — start every enabled secondary that is not already live.
- **End Secondaries** — stop all secondary destinations without stopping the primary.
- **Go Live Selected** — start only the selected enabled secondary.
- **End Selected** — stop only the selected secondary.

Stopping one secondary does not intentionally stop the primary or other healthy secondary destinations.

## Input vs output resolution

Himothee uses one shared OBS video input.

The **Input** column is the OBS video output that feeds the encoders. It comes from OBS Settings > Video and is shared by all destinations.

The **Output** column is the actual encoded resolution sent by that destination.

Independent Encoder destinations can scale their output separately using **Output width** and **Output height**.

Shared Encoder destinations necessarily use the same encoded video resolution as the primary encoder.

## Twitch 1440p + Kick 1080p

Recommended configuration:

```text
OBS / Twitch primary
  Enhanced Broadcasting
  Twitch 2K / 2560x1440
  Primary stream controlled by OBS

Kick secondary
  Destination enabled: Yes
  Auto-start: Yes or No
  Encoder mode: Independent
  Output width: 1920
  Output height: 1080
  Video bitrate: up to 8000 kbps
  Video codec: H.264
```

With Auto-start disabled for Kick, pressing **Start Primary + Auto** starts Twitch only. Kick remains prepared and can be started later with **Go Live Selected**.

With Auto-start enabled, Twitch and Kick start together.

## Platform limits

Kick secondary outputs are limited to 1920x1080 or lower in Himothee because Kick's current standard ingest maximum is 1080p.

Twitch 1440p/2K is handled through Twitch Enhanced Broadcasting as the primary output. Ordinary Himothee Twitch secondary RTMP remains a standard H.264 path and is limited to 1920x1080 in this implementation.

## Current architecture

The primary stream is still the OBS-configured service.

Secondary outputs are prepared from the primary streaming session. Therefore an arbitrary secondary cannot currently be the only active platform while the OBS primary remains configured to a different platform.

For example:

- Twitch only: supported by disabling/ending Kick secondaries.
- Twitch + Kick: supported.
- Start Twitch, then add/remove Kick live: supported.
- Kick only while OBS primary remains configured as Twitch: not yet a standalone-secondary session.

A future symmetric-output pass can remove that final primary/secondary limitation if required.
