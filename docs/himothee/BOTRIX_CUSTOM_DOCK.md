# Botrix Multi Chat as a Custom Dock

Himothee Studio uses the normal OBS Custom Browser Dock system for multiplatform
chat instead of maintaining separate Twitch, YouTube, and Kick chat APIs.

## Setup

1. Open Botrix and configure Multi Chat for the platforms you use.
2. Copy the Botrix Multi Chat browser URL from Botrix.
3. In Himothee Studio open **Docks > Custom Browser Docks**.
4. Add a new dock:
   - Dock Name: **Botrix Multi Chat**
   - URL: paste the Botrix Multi Chat URL.
5. Click **Apply**.
6. Move, resize, or tab the dock anywhere in the Himothee Studio layout.

The dock is then saved using the normal OBS/Himothee dock-layout system.

## Why this approach

- One maintained chat service instead of three platform API connectors.
- Combined Twitch, YouTube, and Kick chat.
- No duplicate Himothee OAuth/EventSub/live-chat stack.
- Lower maintenance risk when platform APIs change.
- Chat remains independent of the native Multistream Manager.

## Security

Treat account-specific Botrix URLs as private if they contain session,
authentication, or account identifiers.

Do not commit private Botrix URLs or tokens to GitHub.
