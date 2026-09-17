"""Load the Linux VST3 headlessly through pedalboard and render one note."""
import sys

import numpy as np
from mido import Message
from pedalboard import load_plugin

bundle = sys.argv[1]
plugin = load_plugin(bundle)
assert plugin.is_instrument, "not an instrument"
audio = plugin([Message("note_on", note=60, velocity=100, time=0.0),
                Message("note_off", note=60, time=1.0)], duration=1.5, sample_rate=44100)
peak = float(np.abs(audio).max())
print(f"rendered shape={audio.shape} peak={peak:.4f}")
assert audio.shape[0] == 2 and audio.shape[1] >= 44100, "unexpected render shape"
assert peak > 1e-4, "silent render"
print("OK")
