#!/usr/bin/env python3
"""
Local runner for FreqSync — builds backend, serves frontend, runs both for testing.
Backend: http://localhost:8001
Frontend: http://localhost:8080
"""
import os
import sys
import subprocess
import signal
import time
import platform
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BACKEND_DIR = ROOT / "backend"
BUILD_DIR = BACKEND_DIR / "build"
FRONTEND_PORT = 8080
BACKEND_PORT = 8001


def find_backend_exe():
    """Locate speakerlearn_serverd executable (platform-specific)."""
    if platform.system() == "Windows":
        candidates = [
            BUILD_DIR / "Release" / "speakerlearn_serverd.exe",
            BUILD_DIR / "Debug" / "speakerlearn_serverd.exe",
            BUILD_DIR / "speakerlearn_serverd.exe",
        ]
    else:
        candidates = [
            BUILD_DIR / "speakerlearn_serverd",
            BUILD_DIR / "Release" / "speakerlearn_serverd",
        ]
    for p in candidates:
        if p.exists():
            return str(p)
    return None


def build_backend():
    """Build backend with CMake."""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    print("[run] Configuring CMake...")
    subprocess.run(
        ["cmake", ".."],
        cwd=BUILD_DIR,
        check=True,
        capture_output=True,
    )
    print("[run] Building...")
    cfg = "Release" if platform.system() == "Windows" else "Release"
    subprocess.run(
        ["cmake", "--build", ".", "--config", cfg],
        cwd=BUILD_DIR,
        check=True,
    )


def main():
    os.chdir(ROOT)

    # 1. Build backend if needed
    exe = find_backend_exe()
    if not exe:
        print("[run] Backend not built. Building now...")
        try:
            build_backend()
            exe = find_backend_exe()
        except subprocess.CalledProcessError as e:
            print(f"[run] Build failed: {e}")
            sys.exit(1)
        if not exe:
            print("[run] Build succeeded but executable not found.")
            sys.exit(1)
    print(f"[run] Backend executable: {exe}")

    # 2. Start backend
    backend_proc = subprocess.Popen(
        [exe],
        cwd=BACKEND_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    time.sleep(0.5)
    if backend_proc.poll() is not None:
        out, _ = backend_proc.communicate()
        print(f"[run] Backend failed to start:\n{out.decode(errors='replace')}")
        sys.exit(1)

    # 3. Start frontend (Python HTTP server) — serve from project root
    import http.server
    import socketserver

    class QuietHandler(http.server.SimpleHTTPRequestHandler):
        def log_message(self, format, *args):
            pass  # Suppress per-request logs

    handler = QuietHandler
    handler.extensions_map[".js"] = "application/javascript"

    try:
        with socketserver.TCPServer(("", FRONTEND_PORT), handler) as httpd:
            httpd.allow_reuse_address = True
            print()
            print("=" * 50)
            print("  FreqSync — Local Test Runner")
            print("=" * 50)
            print(f"  Frontend:  http://localhost:{FRONTEND_PORT}/homePage.html")
            print(f"  Backend:   http://localhost:{BACKEND_PORT}/")
            print("=" * 50)
            print("  Press Ctrl+C to stop both servers")
            print()
            httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        print("\n[run] Shutting down...")
        backend_proc.terminate()
        backend_proc.wait(timeout=3)


if __name__ == "__main__":
    main()
