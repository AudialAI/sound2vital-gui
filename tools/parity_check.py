"""Render presets through Vital 1.6.4 and Audial Synth and report per-preset residuals.

  ../genetic_vital/.venv/bin/python tools/parity_check.py --out qualification/parity/2026-09-30
"""
import argparse
import glob
import json
import sys
import traceback
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
    """Raw (unaligned) residual — intentionally the same quantity as the
    engine's ``compare_audio`` ``raw_error_dB``, not its latency-aligned
    metric. Alignment picks whichever window correlates best, which can
    false-positive (report a much better score than warranted) on presets
    with long delay/reverb tails; staying unaligned means a bad match at the
    start of the note is never hidden by a lucky later window.
    """
    if reference.ndim != 2 or candidate.ndim != 2:
        raise ValueError(
            f"Expected 2-D (channels, samples) arrays, got shapes {reference.shape} and {candidate.shape}"
        )
    n = min(reference.shape[1], candidate.shape[1])
    a, b = reference[:, :n], candidate[:, :n]
    ref_rms = float(np.sqrt(np.mean(a ** 2)))
    cand_rms = float(np.sqrt(np.mean(b ** 2)))
    if ref_rms < 1e-6:
        return float("-inf") if cand_rms < 1e-6 else float("inf")
    return float(20 * np.log10(np.sqrt(np.mean((a - b) ** 2)) / ref_rms + 1e-12))


def _json_default(obj):
    if isinstance(obj, np.ndarray):
        return obj.tolist()
    if isinstance(obj, np.bool_):
        return bool(obj)
    if isinstance(obj, np.generic):
        return obj.item()
    raise TypeError(f"Object of type {type(obj).__name__} is not JSON serializable")


def build_summary(rows):
    ok_rows = [r for r in rows if "residual_db" in r]
    error_count = len(rows) - len(ok_rows)
    values = np.array([r["residual_db"] for r in ok_rows], dtype=float)
    finite_mask = np.isfinite(values)
    finite = values[finite_mask]
    non_finite_count = int((~finite_mask).sum())
    return {
        "count": len(rows),
        "errors": error_count,
        "non_finite": non_finite_count,
        "median_residual_db": float(np.median(finite)) if finite.size else None,
        "worst_residual_db": float(finite.max()) if finite.size else None,
        "below_minus_40_db": int((finite < -40).sum()) if finite.size else 0,
        "rows": rows,
    }


def write_output(out_dir, rows):
    sorted_rows = sorted(rows, key=lambda r: r.get("residual_db", float("-inf")), reverse=True)
    summary = build_summary(sorted_rows)
    (out_dir / "parity.json").write_text(json.dumps(summary, indent=2, default=_json_default))
    return summary


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
        try:
            if not preset.is_file():
                raise FileNotFoundError(f"listed preset not found: {preset}")
            state = json.loads(preset.read_text())
            report = preset.with_name("report.json")
            seconds = json.loads(report.read_text()).get("source_duration_s", 2.0) if report.is_file() else 2.0
            seconds = float(min(max(seconds, 0.5), 20.0)) + 0.25
            a = render(reference, state, seconds)
            b = render(fork, state, seconds)
            row = {"preset": str(preset), "seconds": seconds, "residual_db": residual_db(a, b),
                   "compare_audio": compare_audio(a, b)}
            key = preset.parent.parent.name
            sf.write(args.out / f"{key}_vital164.wav", a.T, 44100)
            sf.write(args.out / f"{key}_audialsynth.wav", b.T, 44100)
            print(json.dumps({"preset": key, "residual_db": round(row["residual_db"], 2)}), flush=True)
        except Exception as exc:  # noqa: BLE001 - keep going; every failure is recorded, not fatal.
            row = {"preset": str(preset), "error": f"{type(exc).__name__}: {exc}"}
            print(json.dumps({"preset": preset.name, "error": row["error"]}), file=sys.stderr, flush=True)
            traceback.print_exc()
        rows.append(row)
        write_output(args.out, rows)
    summary = write_output(args.out, rows)
    print(json.dumps({k: summary[k] for k in
                       ("count", "errors", "non_finite", "median_residual_db", "worst_residual_db", "below_minus_40_db")}))


if __name__ == "__main__":
    main()
