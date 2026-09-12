"""Rebuild both Wasm cores and package the browser application from this checkout."""
from pathlib import Path
import importlib.util
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
zig = shutil.which('zig')
if not zig:
    package = importlib.util.find_spec('ziglang')
    if not package or not package.origin:
        raise SystemExit('Install tools/requirements.txt in your Python environment first.')
    zig = str(Path(package.origin).parent / 'zig')
subprocess.run([sys.executable, str(ROOT / 'tools/build_math.py')], cwd=ROOT, check=True)
subprocess.run(['sh', str(ROOT / 'tools/build_ym3812_wasm.sh')], cwd=ROOT,
               env={**os.environ, 'ZIG': zig, 'YMFM_BUILD_DIR': str(ROOT / 'build/ymfm')}, check=True)
subprocess.run([sys.executable, str(ROOT / 'tools/build_web.py')], cwd=ROOT, check=True)
print('Built simulation, YM3812 audio and complete static browser package.')
