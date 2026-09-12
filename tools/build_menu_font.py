"""Convert the existing licensed DOSBox 8x16 glyphs to a browser font.
Font source and license: web/iscore-font16.mjs, third_party/dosbox-font/.
Each lit bitmap run becomes an unhinted rectangle; no new font artwork.
"""
from pathlib import Path
import re
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT/'web/iscore-font16.mjs').read_text()
data = list(map(int, re.search(r'\[([\d,]+)\]', source).group(1).split(',')))
font = FontBuilder(1024, isTTF=True)
names = ['.notdef'] + [f'cp{i}' for i in range(256)]
font.setupGlyphOrder(names)
glyphs, metrics = {}, {}
for i, name in enumerate(names):
    pen = TTGlyphPen(None)
    leftmost = 8
    if i:
        for y in range(16):
            bits, x = data[(i-1)*16+y], 0
            while x < 8:
                if not bits & (128 >> x):
                    x += 1
                    continue
                start = x
                leftmost = min(leftmost, start)
                while x < 8 and bits & (128 >> x):
                    x += 1
                left, right, top, bottom = start*64, x*64, (14-y)*64, (13-y)*64
                pen.moveTo((left,bottom)); pen.lineTo((left,top))
                pen.lineTo((right,top)); pen.lineTo((right,bottom)); pen.closePath()
    glyphs[name] = pen.glyph()
    metrics[name] = (512, 0 if leftmost == 8 else leftmost*64)
font.setupGlyf(glyphs)
font.setupHorizontalMetrics(metrics)
font.setupHorizontalHeader(ascent=896, descent=-128)
font.setupCharacterMap({ord(bytes([i]).decode('cp437')):f'cp{i}' for i in range(32,256)})
font.setupNameTable({'familyName':'Corncob DOS','styleName':'Regular','uniqueFontIdentifier':'CorncobDOSBitmap','fullName':'Corncob DOS','psName':'CorncobDOS'})
font.setupOS2(sTypoAscender=896,sTypoDescender=-128,usWinAscent=896,usWinDescent=128)
font.setupPost()
font.font.flavor = 'woff'
font.save(ROOT/'web/dos-font.woff')
