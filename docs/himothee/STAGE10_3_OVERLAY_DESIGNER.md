# Stage 10.3 — Overlay Designer

Himothee Studio v0.10.2 adds the visual design layer to the browser-source overlay engine.

## Theme presets

The dock now includes:

- Himothee Dark
- Minimal
- Neon
- Transparent
- Custom

A preset updates the editable visual values; users can then continue adjusting individual fields.

## Typography

Per overlay:

- Font family
- Value font size
- Text colour

Initial font choices:

- Segoe UI
- Arial
- Impact
- Trebuchet MS
- Georgia
- Consolas

Additional saved font-family values are preserved if they already exist in a profile.

## Panel styling

Per overlay:

- Background colour
- Background opacity from 0–100%
- Corner radius from 0–200 px

The browser source remains transparent outside the overlay panel.

## Position presets

The full browser canvas can position the widget in nine locations:

- Top Left
- Top Centre
- Top Right
- Middle Left
- Centre
- Middle Right
- Bottom Left
- Bottom Centre
- Bottom Right

The Browser Source itself can remain full-canvas sized while the widget panel moves within it.

## Entry animations

Available animation presets:

- None
- Fade
- Pop
- Slide Up
- Slide Left

The animation is triggered when the overlay becomes visible and when the selected animation changes.

## Media layer

An overlay can optionally use:

- PNG
- JPG / JPEG
- WEBP
- GIF
- MP4
- WEBM
- MOV
- M4V
- HTTPS media URL

Local media is served through the Himothee localhost overlay server. This means the OBS Browser Source does not need direct file access to the original path.

Per media layer:

- opacity
- loop video toggle

The media layer sits behind the text/value content and behind the configurable colour tint.

## Persistence

Stage 10.3 extends per-profile `overlays.json` with:

- `theme`
- `font_family`
- `font_size`
- `text_color`
- `background_color`
- `background_opacity`
- `corner_radius`
- `position`
- `animation`
- `media_path`
- `media_opacity`
- `media_loop`

Existing Stage 10.1 and Stage 10.2 profiles load with safe defaults.

## Runtime endpoints

Existing endpoints remain stable:

```text
http://127.0.0.1:3293/overlay/<id>
http://127.0.0.1:3293/api/overlay/<id>
```

Local designer media is exposed only through the localhost service:

```text
http://127.0.0.1:3293/media/<id>
```

## Runtime test checklist

After the Windows build passes:

1. Open Himothee Overlays.
2. Create a Counter or Timer widget.
3. Try each theme preset.
4. Change font, size, colours and opacity.
5. Test all nine position presets.
6. Test each animation by hiding/showing the widget.
7. Add a PNG/JPG/GIF media layer.
8. Add a local MP4/WEBM and verify autoplay/loop.
9. Create an OBS Browser Source and confirm designer changes appear after Save.
10. Restart Himothee Studio and confirm design settings persist.
11. Confirm older Text/Timer/Counter widgets still load correctly.

## Next

Stage 10.4 will add darts-specific widgets: 180 counter, 140+/100+ counts, legs/wins/averages, checkout notifications, and later HimotheeLink/Autodarts event integration.
