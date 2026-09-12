"""Package the source-translated browser game and original edition content."""
from pathlib import Path
import shutil,json,struct
ROOT=Path(__file__).resolve().parents[1]
dist=ROOT/'dist'
if dist.exists():shutil.rmtree(dist)
dist.mkdir()
for source in (ROOT/'web').iterdir():
    if source.is_file() and (source.suffix in ['.html','.css','.mjs','.wasm','.woff'] or source.name=='_headers'):shutil.copy2(source,dist/source.name)
shutil.copy2(ROOT/'build/corncob-math.wasm',dist/'corncob.wasm')
for source,target in [('docs/world-runtime.json','world.json'),('reference/original-cockpit.indices','cockpit.indices'),('reference/original-initial-ds.bin','initial-ds.bin'),('reference/original-ground-tables.bin','ground-tables.bin'),('reference/original-mode10-palette.bin','mode-palette.bin'),('reference/captured-pilot.scr','initial-pilot.scr'),('reference/theater-definitions-original.bin','theater-definitions.bin'),('docs/theater-definitions-original.json','theaters.json'),('docs/briefings-original.json','briefings.json'),('reference/pilot-menu-labels.json','pilot-menu-labels.json')]:shutil.copy2(ROOT/source,dist/target)
original=ROOT/'originals/shareware-v342/extracted';(dist/'missions').mkdir(exist_ok=True)
shutil.copy2(original/'3.ADL',dist/'3.adl')
for stem in ['MORNING','DUSK','WASTELND','WHTSANDS','DEFTOWER']:
    for ext in ['CCT','TWR']:
        source=original/f'{stem}.{ext}'
        if source.exists():shutil.copy2(source,dist/'missions'/f'{stem.lower()}.{ext.lower()}')
(dist/'images').mkdir(exist_ok=True)
for number in [2,6,13,9,8,7,4,3,5,10]:shutil.copy2(original/f'3D{number}.IMG',dist/'images'/f'3d{number}.img')
(dist/'manual.txt').write_text((original/'CORNCOB.DOC').read_bytes().decode('cp437'))
(dist/'credits.txt').write_text('Corncob 3D by Kevin Stokes / Pie in the Sky Software.\nBrowser source translation based on preserved release3.42.\n\n'+(original/'LICENSE.DOC').read_bytes().decode('cp437')+'\n\nYM3812 emulation: ymfm by Aaron Giles and contributors.\n'+(ROOT/'third_party/ymfm/THIRD_PARTY.md').read_text()+'\n'+(ROOT/'third_party/ymfm/LICENSE').read_text())
with (dist/'credits.txt').open('a') as credits:
    credits.write('\n\nLater preservation permission record:\nTideGear reports permission from Kevin Stokes and George Welch to release the games as freeware, and separately reports broad source reuse permission from Stokes. The original page was preserved by Common Crawl on 2018-05-20; its HTML and WARC payload digest are retained with this project. This account does not specify a standard open-source license. Original third-party ROL/BNK music is supplied only through user import.\nOriginal page: http://tidegear.net/corncob/index.html\n')
# Retained instruction-range verifier still uses this deterministic original
# mission tile fixture. It is not a startup snapshot for the browser app.
mission=next(m for m in json.loads((ROOT/'docs/mission-inventory.json').read_text()) if m['file']=='MORNING.CCT')
tiles=bytearray(64*3456)
for obj in mission['objects']:
    x,y=obj['tile'];index=int(obj['id'].split(':')[-1]);p=(y*8+x)*3456+index*23
    raw=bytearray.fromhex(obj['raw_hex']);struct.pack_into('<H',raw,21,obj['template']);tiles[p:p+23]=raw
(dist/'world-tiles.bin').write_bytes(tiles)
for old in ['renderer.mjs','geometry.json','flight-state.bin']:(dist/old).unlink(missing_ok=True)


shutil.copy2(ROOT/'third_party/adplug/LICENSE',dist/'adplug-license.txt')
with (dist/'credits.txt').open('a') as credits:
    credits.write('\n\nROL/BNK player: JavaScript derivation of AdPlug by the AdPlug authors. LGPL-2.1-or-later.\nEditable source is rol-music.mjs; the complete license is adplug-license.txt.\n'+(ROOT/'third_party/adplug/PROVENANCE.md').read_text())

(dist/'presentation').mkdir(exist_ok=True)
for source in (ROOT/'reference/presentation').iterdir():
    if source.suffix in ['.bin','.json']:shutil.copy2(source,dist/'presentation'/source.name)

shareware={
    'id':0,'key':'shareware','label':'Shareware 3.42',
    'files':dict(wasm='corncob.wasm',snapshot='world.json',initialDS='initial-ds.bin',cockpit='cockpit.indices',pilotFile='initial-pilot.scr',definitions='theater-definitions.bin',theaters='theaters.json',briefings='briefings.json',groundTables='ground-tables.bin',modePalette='mode-palette.bin',soundAdl='3.adl',defaultSoundAdl='3.adl',pilotLabels='pilot-menu-labels.json',resultScenes='presentation/result-picture-scenes.json',titleScenes='presentation/title-presentation-original.json'),
    'worlds':{stem:f'missions/{stem}.cct' for stem in ['morning','dusk','wastelnd','whtsands']},
    'towers':{stem:f'missions/{stem}.twr' for stem in ['morning','dusk','wastelnd','whtsands','deftower']},
    'images':{str(i):f'images/3d{i}.img' for i in [2,6,13,9,8,7,4,3,5,10]}
}
# Other Worlds uses its own verified cold state, plain DEF files, roster,
# briefings, title and result artwork. Shared assets were byte-compared.
profile=ROOT/'research/editions/other-worlds-profile'
manifest=json.loads((profile/'asset-manifest.json').read_text())
ow_original=ROOT/'originals/other-worlds/extracted'
prefix='editions/other-worlds/'
ow_dist=dist/prefix;ow_dist.mkdir(parents=True)
shutil.copy2(ow_original/'3.ADL',ow_dist/'3.adl')
shutil.copy2(ow_original/'DEF3.ADL',ow_dist/'def3.adl')
files=dict(shareware['files']);files['soundAdl']=prefix+'3.adl';files['defaultSoundAdl']=prefix+'def3.adl'
for key,source,target in [
    ('initialDS',profile/manifest['initialDS'],'initial-ds.bin'),
    ('groundTables',profile/manifest['groundTables'],'ground-tables.bin'),
    ('pilotFile',profile/manifest['pilot']['file'],'initial-pilot.scr'),
    ('definitions',profile/manifest['definitions'],'theater-definitions.bin'),
    ('theaters',profile/manifest['catalog'],'theaters.json'),
    ('briefings',profile/manifest['briefings'],'briefings.json'),
    ('pilotLabels',profile/manifest['pilotLabels'],'pilot-menu-labels.json')]:
    shutil.copy2(source,ow_dist/target);files[key]=prefix+target
shutil.copytree(ROOT/'research/editions/other-worlds-presentation/presentation',ow_dist/'presentation')
files['resultScenes']=prefix+'presentation/result-picture-scenes.json'
files['titleScenes']=prefix+'presentation/title-presentation-original.json'
(ow_dist/'missions').mkdir()
worlds={};towers={}
for row in manifest['worlds']:
    stem=row['stem']
    for kind,extension in [('world','def'),('tower','twr')]:
        target=f'missions/{stem}.{extension}';shutil.copy2(ow_original/row[kind],ow_dist/target)
        (worlds if kind=='world' else towers)[stem]=prefix+target
shutil.copy2(ow_original/manifest['default_tower'],ow_dist/'missions/deftower.twr')
towers['deftower']=prefix+'missions/deftower.twr'
(ow_dist/'manual.txt').write_text((ow_original/'CORNCOB.DOC').read_bytes().decode('cp437'))
(ow_dist/'information.txt').write_text((ow_original/'MOAG.INF').read_bytes().decode('cp437'))
# Unchanged original packaged recordings and original-executed NDX catalog.
(ow_dist/'demos').mkdir()
demo_catalog=json.loads((ROOT/'reference/packaged-demos/packaged-demos.json').read_text())
shutil.copy2(ROOT/'reference/packaged-demos/packaged-demos.json',ow_dist/'demos/packaged-demos.json')
shutil.copy2(ow_original/'DMO/DEMO.NDX',ow_dist/'demos/DEMO.NDX')
for row in demo_catalog['records']:
    shutil.copy2(ow_original/'DMO'/f"{row['stem'].upper()}.DMO",ow_dist/'demos'/row['file'])
files['demoCatalog']=prefix+'demos/packaged-demos.json'
other_worlds={'id':2,'key':'other-worlds','label':'Other Worlds','files':files,'worlds':worlds,'towers':towers,'images':dict(shareware['images'])}
(dist/'editions.json').write_text(json.dumps([shareware,other_worlds],indent=2)+'\n')
print('Packaged shareware and Other Worlds original edition content in dist/')

shutil.copy2(ROOT/'third_party/dosbox-font/LICENSE',dist/'dosbox-font-license.txt')
with (dist/'credits.txt').open('a') as credits:
    credits.write('\n\nDOS text-mode font: DOSBox0.74-3, copyright2002–2018 The DOSBox Team. GPL-2.0-or-later; see dosbox-font-license.txt. Editable font data is iscore-font16.mjs. dos-font.woff is a rectangle-outline conversion of those same glyphs; reproducible generator: tools/build_menu_font.py (requires fontTools).\n')
