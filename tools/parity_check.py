"""Render presets through Vital 1.6.4 and Audial Synth and report per-preset residuals.

  ../genetic_vital/.venv/bin/python tools/parity_check.py --out qualification/parity/2026-09-30
"""
import argparse
import glob
import json
import sys
from pathlib import Path

import numpy as np
import soundfile as sf

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE.parent / "genetic_vital" / "src"))
from scripts.v2.vital_vst3 import load_plugin, apply_preset_state, render_midi  # noqa: E402
from sound2vital.diagnostics import compare_audio  # noqa: E402

DEFAULT_PRESETS = "/Users/zachfarrell/Desktop/Coding_Projects/genetic/queries/results/engine_v3/editable_corpus_parallel_v8/run/native/*/work/preset.vital"


def render(plugin, state, seconds):
    apply_preset_state(plugin, state)
    return render_midi(plugin, duration=seconds, note_duration=seconds, midi_note=60, sample_rate=44100)


def residual_db(reference, candidate):
    n = min(reference.shape[1], candidate.shape[1])
    a, b = reference[:, :n], candidate[:, :n]
    ref_rms = np.sqrt(np.mean(a ** 2)) + 1e-12
    return float(20 * np.log10(np.sqrt(np.mean((a - b) ** 2)) / ref_rms + 1e-12))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--presets", default=DEFAULT_PRESETS)
    parser.add_argument("--preset-list", type=Path,
                         help="Text file of preset paths, one per line. Overrides --presets when given.")
    parser.add_argument("--reference", type=Path, default=Path("/Library/Audio/Plug-Ins/VST3/Vital.vst3"))
    parser.add_argument("--fork", type=Path, default=HERE / "plugin/builds/osx/build/Release/AudialSynth.vst3")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--limit", type=int)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    reference = load_plugin(args.reference)
    fork = load_plugin(args.fork)
    if args.preset_list:
        paths = [line.strip() for line in args.preset_list.read_text().splitlines() if line.strip()]
    else:
        paths = sorted(glob.glob(args.presets))
    rows = []
    for path in paths[: args.limit]:
        preset = Path(path)
        state = json.loads(preset.read_text())
        report = preset.with_name("report.json")
        seconds = json.loads(report.read_text()).get("source_duration_s", 2.0) if report.is_file() else 2.0
        seconds = float(min(max(seconds, 0.5), 20.0)) + 0.25
        a = render(reference, state, seconds)
        b = render(fork, state, seconds)
        row = {"preset": str(preset), "seconds": seconds, "residual_db": residual_db(a, b),
               "compare_audio": compare_audio(a, b)}
        rows.append(row)
        print(json.dumps({"preset": preset.parent.parent.name, "residual_db": round(row["residual_db"], 2)}), flush=True)
        key = preset.parent.parent.name
        sf.write(args.out / f"{key}_vital164.wav", a.T, 44100)
        sf.write(args.out / f"{key}_audialsynth.wav", b.T, 44100)
    rows.sort(key=lambda r: r["residual_db"], reverse=True)
    values = np.array([r["residual_db"] for r in rows])
    summary = {"count": len(rows), "median_residual_db": float(np.median(values)),
               "worst_residual_db": float(values.max()), "below_minus_40_db": int((values < -40).sum()),
               "rows": rows}
    (args.out / "parity.json").write_text(json.dumps(summary, indent=2, default=float))
    print(json.dumps({k: summary[k] for k in ("count", "median_residual_db", "worst_residual_db", "below_minus_40_db")}))


if __name__ == "__main__":
    main()
