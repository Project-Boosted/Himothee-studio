# Stage 10.6.2 — Local Stream Deck Control Bridge

Himothee Studio v0.10.8 introduces an HTTP JSON local transport for the Stage 10.6.1 Action Registry. The native Stream Deck button plugin is a later stage; this is the controller-facing protocol foundation.

## Discovery

The bridge listens only on `127.0.0.1:3294`. Controllers discover it by probing `GET http://127.0.0.1:3294/v1/health`; no IP or port entry is required for a standard local setup.

- `GET /v1/health` — identify the application and protocol version.
- `GET /v1/actions` — available stable action IDs and parameter definitions.
- `GET /v1/state` — current action-registry state snapshot.
- `POST /v1/execute` — execute a registered action.

All responses are JSON. The bridge is single-threaded with the Qt UI, matching the registry's execution contract.

### Local controller request

```http
POST /v1/execute HTTP/1.1
Host: 127.0.0.1:3294
Content-Type: application/json
X-Himothee-Client: stream-deck

{"action_id":"counter.increment","params":{"overlay_id":"YOUR_OVERLAY_ID","amount":1}}
```

To start primary streaming: `{"action_id":"stream.start","params":{}}`.

To stop a particular secondary: `{"action_id":"destination.stop","params":{"destination_id":"YOUR_DESTINATION_ID"}}`.

Use explicit start/stop where possible rather than a toggle. This avoids unintended second presses. The previous Stage 10.6.1 requirement that primary OBS streaming be active before a secondary starts still applies.

## Security and operational constraints

- Binds to IPv4 loopback only; does not listen on LAN or internet interfaces.
- Accepts loopback peers and local Host headers only.
- Mutating requests require a non-simple `X-Himothee-Client` header and `application/json`, with no permissive CORS policy.
- Requests are limited to 64 KiB; chunked transfer is rejected.
- No network transport or Stream Deck plugin from Stage 10.6.1 is removed.
- Port 3293 remains dedicated to OBS Browser Source overlays.
- Localhost access is **not authentication**. Do not expose port 3294 through a reverse proxy or firewall port-forward. Add a paired-controller authorization mechanism before enabling remote/LAN control.

## Manual Windows smoke tests

1. Launch Himothee Studio and verify it opens without errors.
2. PowerShell: `Invoke-RestMethod http://127.0.0.1:3294/v1/health`
3. List actions: `Invoke-RestMethod http://127.0.0.1:3294/v1/actions`
4. Retrieve state: `Invoke-RestMethod http://127.0.0.1:3294/v1/state`
5. With an existing counter overlay, use PowerShell:

```powershell
$body = @{ action_id='counter.increment'; params=@{ overlay_id='YOUR_OVERLAY_ID'; amount=1 } } | ConvertTo-Json -Depth 5
Invoke-RestMethod -Uri 'http://127.0.0.1:3294/v1/execute' -Method Post -ContentType 'application/json' -Headers @{'X-Himothee-Client'='stream-deck'} -Body $body
```

6. Confirm the counter updates once in the Himothee dock and OBS Browser Source.
7. Repeat with a missing overlay ID and verify a structured failure response.
8. Verify OBS still streams and records; verify secondary destinations, overlay display, and existing Botrix Custom Browser Docks still work.

## Next steps

Add the native Stream Deck plugin with button discovery/configuration, live button states and Stream Deck+ support; consider a versioned state-push channel and paired local clients. Builds and physical Stream Deck hardware tests are required before treating the bridge as production validated.
