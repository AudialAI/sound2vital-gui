> This checklist was not executed in this session (Task 9 verified the mock API only, via curl); it is pending a run by a person at the screen.

# Manual checklist (run after every GUI change)

Start the mock: `python3 tools/mock_audial_api.py --preset <any engine preset.vital> --delay 6`
Open `standalone/builds/osx/build/Release/AudialSynth.app`, click RESYNTH.

1. Credentials: base URL `http://localhost:8766`, user `u1`, key `k1`, Save. Quit and reopen: fields are restored.
2. Drop a 21 s wav: status "Sample is 21.0 s; the limit is 20 s"; nothing is sent (mock log empty).
3. Drop a 3 s wav: status goes Uploading → Submitting → Processing... N s → Downloading; the panel closes and the header preset name is the new `<sample>_<stamp>` file; sound plays from the keyboard; the preset browser lists it under User/Resynth.
4. Restart the mock with `--fail`, drop the 3 s wav: status shows "Input is 25.0 s; the limit is 20 s" and the panel stays open.
5. Start a job, click Cancel during Processing: status "Cancelled", Browse visible again.
6. Restart the mock with `--unsubscribed`, drop the 3 s wav: the upload succeeds, then status shows "Run failed: This feature needs an active Audial subscription. Subscribe at audialmusic.ai and try again." and the panel stays open. (Against the real API this is what an account without an active subscription sees.)
6. Stop the mock, drop the 3 s wav: status "Upload failed: no connection (check base URL / network)".
7. Load the VST3 in a DAW (Ableton or Logic) and repeat step 3.
8. Ableton Live: with the RESYNTH panel closed, drag an audio clip from the Arrangement or Session view onto the synth window. The panel opens on drag-enter and the drop starts the job (Live hands the clip over as a file promise; it is received into a temp folder first). A MIDI clip is refused (no drop cursor).
