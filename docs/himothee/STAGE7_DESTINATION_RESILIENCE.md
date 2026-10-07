# Stage 7 — Destination Resilience & Health

Stage 7 hardens multistream reliability and diagnostics.

## Reconnect policy

Each destination now has an explicit reconnect policy:

- **Inherit OBS** — use the normal OBS reconnect setting.
- **Always reconnect** — enable reconnect for this destination even if the primary profile has reconnect disabled.
- **Never reconnect** — disable reconnect for this destination.

Max retries and retry delay can still inherit the normal OBS values or be overridden per destination.

If reconnect is enabled but inherited retry values are unusable, Himothee Studio falls back to a conservative default of 20 retries with a 2-second delay.

## Runtime health

Each destination now tracks:

- Current state.
- Time spent in the current state.
- Session uptime.
- Reconnect count.
- Error count.
- Last error.
- Live bitrate.
- Dropped frames.
- Connect time.
- Congestion/health.
- Total transferred data.
- Shared/Independent encoder mode and codecs.

## Manual recovery

When a selected destination is in Error state, the normal **Start Selected** control changes to **Retry Selected**.

Retrying a failed secondary output does not intentionally restart the primary stream or healthy secondary outputs.

## Failure isolation

Destination failures remain isolated from:

- The primary OBS stream.
- Other healthy secondary destinations.
- Recording.
- Replay Buffer.

## Profile compatibility

The profile-level `multistream.json` adds:

- `reconnect_policy`

Older Stage 3–6 profile files remain compatible and default to **Inherit OBS**.

## Test checklist

- [ ] Windows CI build passes.
- [ ] Himothee Studio reports version 0.7.0.
- [ ] Existing Stage 6 profile loads with Inherit OBS reconnect policy.
- [ ] Always reconnect works when global OBS reconnect is disabled.
- [ ] Never reconnect suppresses retries for that destination.
- [ ] Reconnect count increments during reconnect attempts.
- [ ] Error count increments on failed start/terminal output failure.
- [ ] Uptime increases while a destination is live.
- [ ] State timer resets on state changes.
- [ ] Last error remains visible after recovery for diagnostics.
- [ ] Retry Selected can recover a failed secondary while the primary remains live.
- [ ] One failing secondary does not stop other healthy destinations.
- [ ] Shared and Independent modes both retain health telemetry.
- [ ] Recording and Replay Buffer remain functional.

## Next stage

Stage 8 will add per-destination audio routing.
