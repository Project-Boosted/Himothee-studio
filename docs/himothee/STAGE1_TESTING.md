# Stage 1 Windows Baseline Test

This checklist validates the untouched OBS Studio 32.2.2 baseline used by Himothee Studio before branding or native multistream changes are introduced.

## CI gate

The `Himothee Stage 1 - Windows Baseline` workflow must:

- Build Windows x64 successfully.
- Package a portable Windows ZIP.
- Upload the ZIP as a GitHub Actions artifact.

## Manual smoke test

After downloading and extracting the CI artifact:

1. Launch the application.
2. Create a new scene collection and profile dedicated to baseline testing.
3. Add Display Capture and confirm the preview updates.
4. Add Window Capture and confirm a selected application is visible.
5. Add Game Capture and confirm a supported game/application can be captured.
6. Add a Video Capture Device and confirm camera/capture-card video.
7. Confirm Desktop Audio meters move.
8. Confirm Microphone/Aux meters move.
9. Make a short local recording and play the file back.
10. Enable the replay buffer, save a replay, and confirm the file is usable.
11. Configure one test streaming service and make a short private/unlisted test stream.
12. Close and reopen the application and confirm the profile/scene collection persists.

## Pass criteria

Stage 1 passes when:

- CI produces a runnable Windows x64 artifact.
- The application launches without an immediate crash.
- Core video capture works.
- Core audio capture works.
- Recording works.
- Replay buffer works.
- Standard single-destination streaming works.
- Settings and scene collections persist across restart.

Any baseline defect discovered here should be documented before Himothee-specific code is added.

## Safety

Do not attach logs containing stream keys, OAuth tokens, passwords, or other secrets.
