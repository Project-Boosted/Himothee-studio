# Stage 9.2 — Twitch Account & Live Chat

Stage 9.2 connects the Unified Chat foundation to Twitch.

## Channel selection

Twitch defaults to **Auto — My Channel**.

When a Twitch account is connected in OBS/Himothee Studio:

1. Himothee validates the active Twitch OAuth token.
2. Twitch returns the authenticated account's user ID and login.
3. That account becomes the default broadcaster/chat channel.
4. Himothee subscribes to that channel's `channel.chat.message` EventSub stream.

Users do not need to type their own Twitch channel name every stream.

A **Custom Channel** mode is also available for a different Twitch login. The login is resolved through the Twitch Users API and used as the EventSub broadcaster target.

## Authentication

The first implementation reuses the Twitch account already connected under **Settings > Stream**.

Himothee refreshes that OBS Twitch token before use and then validates it using Twitch's token validation endpoint.

The token must include:

- `user:read:chat`

If the connected token does not have that scope, the Chat dock reports the missing permission instead of silently failing.

OAuth access tokens are never copied into chat messages or written to normal Himothee logs.

## EventSub

Desktop chat uses Twitch EventSub WebSockets.

The provider:

- connects to Twitch's EventSub WebSocket endpoint,
- waits for `session_welcome`,
- creates a `channel.chat.message` subscription,
- handles keepalive messages,
- handles Twitch-requested reconnect URLs,
- retries unexpected disconnections,
- deduplicates repeated EventSub envelope IDs,
- converts Twitch chat notifications into `HimotheeChatMessage`.

## Unified Chat mapping

A Twitch chat event is mapped into the common Himothee message model:

- platform = Twitch,
- message ID,
- chatter user ID,
- display name,
- plain message text,
- broadcaster/moderator/subscriber flags,
- timestamp/sequence handled by the shared manager.

The existing **All** tab therefore receives Twitch messages automatically, while the **Twitch** tab filters to Twitch/system messages.

## Dock controls

The Himothee Chat dock now adds:

- Twitch channel mode:
  - Auto — My Channel
  - Custom Channel
- optional custom Twitch channel login,
- Connect Twitch / Disconnect Twitch,
- visible Twitch connection errors,
- account/channel information in the top provider summary.

Message sending intentionally remains disabled until Stage 9.5.

## Build capability

Qt WebSockets is enabled when available in the OBS Qt dependency bundle.

If the build does not contain Qt WebSockets, the provider reports that limitation in the dock rather than crashing.

## Test checklist

- [ ] Windows CI build passes.
- [ ] Himothee Studio reports version 0.9.4.
- [ ] Twitch provider is registered in Unified Chat.
- [ ] Auto — My Channel is the default.
- [ ] Connected OBS Twitch account is detected.
- [ ] OAuth token is refreshed before use.
- [ ] Token validation succeeds with a compatible Twitch token.
- [ ] Missing `user:read:chat` produces a clear dock error.
- [ ] Authenticated Twitch login becomes the automatic channel.
- [ ] Custom channel login resolves through the Twitch Users API.
- [ ] EventSub WebSocket receives `session_welcome`.
- [ ] `channel.chat.message` subscription is accepted.
- [ ] Incoming Twitch messages appear in All and Twitch tabs.
- [ ] Duplicate EventSub deliveries are ignored.
- [ ] Twitch reconnect messages move the connection without duplicating subscriptions.
- [ ] Disconnect Twitch stops the EventSub session.
- [ ] No access token is logged.
- [ ] Multistream and normal OBS controls remain functional.

## Next

Stage 9.3 will add YouTube account/broadcast discovery and YouTube live chat.
