# Himothee Studio — Stream Deck

Independent Elgato Stream Deck plugin for the Himothee Studio Action Registry.

- Requires Stream Deck 7.1+, Windows 10+, and Stage 10.6.2 bridge in Studio.
- The plugin bundles the official Stream Deck SDK; no manual IP setup.
- Build: `npm install && npm run build`.
- Tests: `npm test`.
- Install for development: `npx streamdeck link com.himothee.studio.sdPlugin`.
- Package: `npm run pack`.

See [Stage 10.6.3 developer notes](../../docs/himothee/STAGE10_6_3_STREAM_DECK_PLUGIN.md)
for feature coverage and Windows smoke tests.

## Windows installer compatibility hotfix

The Elgato CLI's `.streamDeckPlugin` ZIP can use ZIP64 entries even for a tiny
plugin. On affected Stream Deck installations this may show a misleading
"requires Stream Deck 0.0 or later" error before the plugin is installed.

Release installers are now **repacked to ZIP32** with Python 3.12 and checked
for an intact manifest, entrypoint, CRC, and identical inner files. The
`Software.MinimumVersion` remains **7.1**, which the Stream Deck SDK v3
requires. Local packaging (`npm run pack`) also requires Python 3.12.
Do not simply rename `.zip` to `.streamDeckPlugin` or lower the minimum
version to bypass this error.

The fixed GitHub Actions artifact name ends in `-FIXED`; download the archive,
extract the installer and double-click the contained `.streamDeckPlugin`.
