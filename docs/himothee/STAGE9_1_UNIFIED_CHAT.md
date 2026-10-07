# Stage 9.1 — Unified Chat Dock & Message Model

Stage 9.1 creates the common chat foundation for Himothee Studio.

## Native Chat dock

Himothee Studio now has a dockable **Himothee Chat** panel with:

- All / Twitch / YouTube / Kick filters.
- Combined chronological message timeline.
- Platform, user, message, and time columns.
- Platform connection-state summary.
- Send-target selector.
- Message composer foundation.
- Bounded recent chat history.
- Layout persistence through the standard OBS dock system.

The Multistream and Chat docks are initially tabbed together on the right side of the main window and can be moved, floated, hidden, or restored through the Docks menu.

## Unified message model

Every provider converts its native events into `HimotheeChatMessage`.

The common model includes:

- Platform.
- Message type.
- Platform message/user IDs.
- Display name.
- Message text.
- Broadcaster/moderator/subscriber flags.
- Timestamp.
- Internal sequence number.

No OAuth tokens, passwords, stream keys, or other credentials are stored in chat messages.

## Provider interface

Platform integrations implement a common `HimotheeChatProvider` interface:

- Connect.
- Disconnect.
- Send message.
- Report platform.
- Report connection state.
- Report account name.

The central `HimotheeChatManager` owns provider instances, provider state, and a thread-safe recent-message queue.

This lets future features such as alerts, triggers, moderation tools, browser overlays, and Stream Deck actions consume one unified chat stream.

## Stage 9.1 connection state

Twitch, YouTube, and Kick intentionally show **Offline** in Stage 9.1 because their authentication/API adapters are not included yet.

The composer remains disabled until at least one provider reports Connected.

## Next sub-stages

- Stage 9.2 — Twitch authentication and live chat.
- Stage 9.3 — YouTube authentication and live chat.
- Stage 9.4 — Kick authentication and live chat.
- Stage 9.5 — Send to one/all connected platforms.
- Stage 9.6 — Reconnect and rate-limit hardening.
- Stage 9.7 — Moderation actions.
- Stage 9.8 — Subs, memberships, Super Chats and platform events.
- Stage 9.9 — Chat to Himothee Alerts/Triggers integration.

## Test checklist

- [ ] Windows CI build passes.
- [ ] Application reports Himothee Studio 0.9.0.
- [ ] Himothee Chat appears in the Docks menu.
- [ ] Chat dock can be moved, hidden, floated and restored.
- [ ] All/Twitch/YouTube/Kick filters switch without errors.
- [ ] Initial system message appears in All.
- [ ] Provider summary displays all three platforms.
- [ ] Composer is disabled while no provider is connected.
- [ ] Normal OBS docks and saved layout still work.
- [ ] Multistream Manager remains functional.
