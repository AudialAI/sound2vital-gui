"""Stand-in for the Audial API so the Resynth flow can be exercised offline.

  python tools/mock_audial_api.py --preset /path/to/preset.vital --port 8766 --delay 6
"""
import argparse
import json
import re
import threading
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

UPLOAD = re.compile(r"^/api/files/([^/]+)/execution/([^/]+)/reference/([^/]+)$")
EXECUTION = re.compile(r"^/api/db/([^/]+)/execution/([^/]+)$")
executions = {}
lock = threading.Lock()


def make_handler(preset: Path, port: int, delay: float, fail: bool, unsubscribed: bool = False):
    class Handler(BaseHTTPRequestHandler):
        def _json(self, status, payload):
            data = json.dumps(payload).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def _authorised(self):
            return bool(self.headers.get("x-api-key")) and bool(self.headers.get("x-user-id"))

        def do_PUT(self):
            match = UPLOAD.match(self.path)
            if not match or not self._authorised():
                return self._json(403, {"error": "Unauthorized"})
            self.rfile.read(int(self.headers.get("Content-Length", "0")))
            user, exe, name = match.groups()
            self._json(200, {"url": f"http://localhost:{port}/files/{user}/{exe}/{name}"})

        def do_POST(self):
            if self.path != "/api/functions/run/sound2vital" or not self._authorised():
                return self._json(403, {"error": "Unauthorized"})
            if unsubscribed:  # what the real API returns for an account without an active subscription
                return self._json(402, {"error": "This feature needs an active Audial subscription. "
                                                 "Subscribe at audialmusic.ai and try again.",
                                        "code": "SUBSCRIPTION_REQUIRED"})
            body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", "0"))))
            exe = str(uuid.uuid4())
            with lock:
                executions[exe] = {"exeId": exe, "state": "created", "original": body["original"],
                                   "created": time.time(), "exeType": "preset"}
            self._json(200, executions[exe])

        def do_GET(self):
            if self.path == "/files/preset.vital":
                data = preset.read_bytes()
                self.send_response(200); self.send_header("Content-Length", str(len(data))); self.end_headers()
                return self.wfile.write(data)
            match = EXECUTION.match(self.path)
            if not match or not self._authorised():
                return self._json(403, {"error": "Unauthorized"})
            exe = match.group(2)
            with lock:
                record = executions.get(exe)
                if record is None:
                    return self._json(404, {"error": "not found"})
                age = time.time() - record["created"]
                if age >= delay:
                    if fail:
                        record.update(state="failed", error="Input is 25.0 s; the limit is 20 s")
                    else:
                        record.update(state="completed", preset={"presetvital": {
                            "filename": "preset.vital", "url": f"http://localhost:{port}/files/preset.vital"}})
                elif age >= 1:
                    record["state"] = "processing"
                self._json(200, record)

        def log_message(self, fmt, *args):
            print("mock-audial", self.command, self.path)
    return Handler


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", type=Path, required=True)
    parser.add_argument("--port", type=int, default=8766)
    parser.add_argument("--delay", type=float, default=6.0)
    parser.add_argument("--fail", action="store_true", help="complete every job as failed")
    parser.add_argument("--unsubscribed", action="store_true", help="refuse every run with 402 SUBSCRIPTION_REQUIRED")
    args = parser.parse_args()
    ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(args.preset.resolve(), args.port, args.delay, args.fail, args.unsubscribed)).serve_forever()
