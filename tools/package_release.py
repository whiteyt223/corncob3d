"""Package the existing dist output at the ZIP root for Cloudflare Pages."""
from pathlib import Path
import subprocess,zipfile
root=Path(__file__).resolve().parents[1]
subprocess.run(['node',str(root/'tools/check_bundle.mjs')],cwd=root,check=True)
output=root/'build/corncob3d-cloudflare.zip'
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted((root/'dist').rglob('*')):
        if p.is_file():z.write(p,p.relative_to(root/'dist'))
print(output)
