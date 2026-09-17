# Parity: Audial Synth (upstream 1.0.6 DSP) vs Vital 1.6.4

Run: `qualification/parity/2026-09-17` (`parity.json`)
Presets: 207 qualified engine results, drawn from `editable_corpus_v1` and
`editable_corpus_parallel_v1`..`v8` under
`genetic/queries/results/engine_v3/`. The brief's default glob only matched 5
files in that corpus layout; the controller supplied the 207 absolute preset
paths in `qualification/parity_presets.txt` (git-ignored). `tools/parity_check.py`
was extended with a `--preset-list PATH` flag (a newline-delimited file of
preset paths) that replaces the `--presets` glob when given; the `report.json`
sibling lookup for render duration is unchanged. Both runs below used
`--preset-list qualification/parity_presets.txt`.

Median residual: **-33.49 dB**   Worst: **+0.86 dB**
Below -40 dB: **83/207**   Below -20 dB: **142/207**

Both plugins loaded in the same pedalboard process without incident (fork
loaded first, reference second, matching the brief's script) — no
`_PLUGIN_LOAD_LOCK` workaround was needed.

## Distribution

- 147/207 presets have max oscillator unison voice count < 9: median residual
  **-41.58 dB**, worst **-6.78 dB**. This population looks like near-identical
  output between the two engines.
- 60/207 presets have max oscillator unison voice count >= 9 (up to the 16-voice
  ceiling): median residual **-6.16 dB**, worst **+0.86 dB**. This population
  drives essentially all of the bad tail.
- Every one of the 29 presets with residual above -6 dB uses `osc_{1,2,3}_unison_voices
  = 16` on all active oscillators. Of the 65 presets above -20 dB, 53 use
  unison voice count >= 9. No other flag examined (sample layer, filters,
  distortion, chorus/flanger/phaser, reverb/delay, compressor, EQ, or which
  corpus a preset came from) separates the bad tail from the rest — unison
  voice count is the only one that does cleanly.
- For the worst 10, correcting for a simple broadband gain mismatch
  (`gain_fitted_error_dB`) only recovers ~1-2 dB versus the raw residual, so
  the divergence is not just a level difference — the rendered waveform shape
  itself differs once heavy unison stacking is active. `side_error_dB` (stereo
  width) is also elevated and inconsistent across the worst 10, consistent
  with a difference in per-voice detune/phase/pan spread across unison
  voices between Vital 1.0.6 and 1.6.4.

## Worst ten

Residual (raw, matches `parity.json` `residual_db`), the gain-fitted residual
after removing a broadband level mismatch, the stereo-width (`side_error_dB`)
residual, and each preset's oscillator unison voice counts — the metric that
looks most different across the group is `side_error_dB`, which varies
2-13 dB across presets that otherwise look similar, on top of a raw residual
that gain correction barely improves.

| # | preset key | corpus | residual dB | gain-fitted dB | side dB | unison voices (osc1/2/3) |
|---|---|---|---|---|---|---|
| 1 | fe2d27b0302f7e42 | editable_corpus_parallel_v7 | 0.86 | -0.68 | -6.67 | 16/16/16 |
| 2 | ff75f20163828dcf | editable_corpus_parallel_v2 | -0.57 | -1.53 | -7.88 | 16/16/16 |
| 3 | 3a9d56d267fcbe24 | editable_corpus_parallel_v4 | -0.77 | -1.82 | -0.75 | 16/16/16 |
| 4 | 04a7f4346002c537 | editable_corpus_parallel_v6 | -0.78 | -1.71 | -0.57 | 16/16/16 |
| 5 | e415b00581ef4283 | editable_corpus_parallel_v2 | -1.00 | -2.00 | -0.89 | 16/16/16 |
| 6 | 41be551c9e5f285f | editable_corpus_parallel_v6 | -1.05 | -1.97 | -1.09 | 16/16/16 |
| 7 | 2250f3869ce00ff3 | editable_corpus_parallel_v3 | -1.06 | -2.01 | -1.46 | 16/16/16 |
| 8 | 0abeac57523a8864 | editable_corpus_v1 | -1.09 | -2.06 | -1.27 | 16/16/16 |
| 9 | 9ee7a88d352545e1 | editable_corpus_parallel_v2 | -1.15 | -2.09 | -1.05 | 16/16/16 |
| 10 | 0d615fb0a663dcc0 | editable_corpus_parallel_v1 | -1.16 | -2.07 | -1.64 | 16/16/16 |

Observation (from measured diagnostics only; see "Listening" below): all ten
worst presets max out unison voices on every active oscillator (16/16/16),
whereas the best- and middle-ranked presets in this run consistently use
unison voice count 1 and a sample layer. The module responsible for the
divergence is most likely the **oscillator unison/supersaw voice generator**
(per-voice detune, phase, and stereo spread across unison voices), which
changed between upstream Vital 1.0.6 (the fork's DSP) and the proprietary
1.6.4 the presets were qualified against.

**Listening is pending.** The ten worst-pair wavs (`<key>_vital164.wav` /
`<key>_audialsynth.wav`, plus all 207 other pairs) are written to
`qualification/parity/2026-09-17/` but have not been auditioned, and
spectrograms have not been viewed via `compare_visual.py`. This write-up is
based solely on the numeric residuals and `compare_audio` fields above.

## Decision

**Investigate before pinning `PLUGIN_COMMIT`** — 29/207 presets (14%) have
residual above -6 dB, and the bad tail is concentrated cleanly in presets
using high oscillator unison voice counts (>=9, and specifically 16 for the
worst 10). This is not a rounding-error-scale gap: a 0 dB residual means the
fork and the reference disagree by as much as the signal itself. Given the
clean split by unison voice count and the fact that gain correction does not
close the gap, the likely fix is in the fork's oscillator unison/supersaw
voice implementation (detune curve, per-voice phase seeding, or stereo
spread), not a global level or filter issue. The 147 presets with low/no
unison stacking already look like a clean pass (median -41.6 dB, worst
-6.8 dB) and do not block anything on their own.

The controller makes the final decision on whether to proceed, fix the
unison/supersaw path first, or narrow the corpus used for qualification to
exclude heavy-unison presets in the interim.
