"""Pack the civilization building renders (art/renders_civ, made by `blender -b -P art/gen_assets.py -- --civ <code>`) into
   assets/civ_<code>.txt + assets/atlas_civ_<code>_<n>.png  (same text format as atlas.txt; page numbers are local to the civ)
   plus assets/civprev.png / civprev.txt: one small preview picture per civilization for the ascension screen.
   python tools/pack_civs.py [--quant]      --quant: store the civ pages as 8-bit palette PNGs (about a third of the size)
"""
import json, os, sys, glob
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'art', 'renders_civ')
BASE_SRC = os.path.join(ROOT, 'art', 'renders')
DST = os.path.join(ROOT, 'assets')
BASE_PXM = 45.2548
PAGE = 2048
QUANT = '--quant' in sys.argv
CODES = ['sl', 'fr', 'ce', 'ja', 'sp', 'mo']

man = json.load(open(os.path.join(SRC, 'manifest.json')))
base_man = json.load(open(os.path.join(BASE_SRC, 'manifest.json')))

def trimmed(path, m):
    im = Image.open(path).convert('RGBA')
    bb = im.getchannel('A').point(lambda v: 255 if v > 3 else 0).getbbox() or (0, 0, 1, 1)
    return im.crop(bb), m['ax'] - bb[0], m['ay'] - bb[1], m['pxm'] / BASE_PXM

def pack(items, pad=2):
    pages, cur = [], {'items': [], 'h': 0}
    x = y = rowh = 0
    for it in sorted(items, key=lambda t: -t[1].height):
        w, h = it[1].size
        if x + w + pad > PAGE: x = 0; y += rowh + pad; rowh = 0
        if y + h > PAGE: pages.append(cur); cur = {'items': [], 'h': 0}; x = y = rowh = 0
        cur['items'].append((it, x, y)); x += w + pad; rowh = max(rowh, h); cur['h'] = max(cur['h'], y + h)
    pages.append(cur)
    return pages

def save_png(im, path):
    if QUANT:
        q = im.quantize(colors=255, method=Image.FASTOCTREE, dither=Image.NONE)
        q.save(path, optimize=True)
    else:
        im.save(path, optimize=True)

for f in os.listdir(DST):
    if f.startswith('atlas_civ_') or f.startswith('civ_') or f.startswith('civprev'):
        os.remove(os.path.join(DST, f))

total = 0
for code in CODES:
    names = sorted(n for n in man if n.startswith(code + '_'))
    items = []
    for n in names:
        im, ax, ay, sc = trimmed(os.path.join(SRC, n + '.png'), man[n])
        items.append((n, im, ax, ay, sc))
    decl, out = [], []
    for pi, pg in enumerate(pack(items)):
        H = 1
        while H < pg['h']: H *= 2
        atlas = Image.new('RGBA', (PAGE, H), (0, 0, 0, 0))
        fname = 'atlas_civ_%s_%d.png' % (code, pi)
        for (n, im, ax, ay, sc), px, py in pg['items']:
            atlas.paste(im, (px, py))
            out.append('%s %d %d %d %d %d %.2f %.2f %.4f' % (n, pi, px, py, im.width, im.height, ax, ay, sc))
        save_png(atlas, os.path.join(DST, fname))
        sz = os.path.getsize(os.path.join(DST, fname)); total += sz
        decl.append('@ %d %s' % (pi, fname))
        print(fname, atlas.size, len(pg['items']), 'sprites', sz // 1024, 'KB')
    open(os.path.join(DST, 'civ_%s.txt' % code), 'w').write('\n'.join(decl + out) + '\n')
print('civ pages total', total // 1024, 'KB')

# ---- previews: town centre + house + tower composed on a small canvas
def prev(get, code):
    W, H = 360, 230
    cv = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    def put(name, scale, cx, by):
        im = get(name)
        if im is None: return
        im = im.resize((max(1, int(im.width * scale)), max(1, int(im.height * scale))), Image.LANCZOS)
        cv.alpha_composite(im, (int(cx - im.width / 2), int(by - im.height)))
    put('tower_3', 0.40, 278, 168)
    put('townhall_6', 0.47, 172, 214)
    put('house_1', 0.44, 86, 220)
    return cv
def getciv(code):
    def g(name):
        n = code + '_' + name
        if n not in man: return None
        return trimmed(os.path.join(SRC, n + '.png'), man[n])[0]
    return g
def getbase(name):
    if name not in base_man: return None
    return trimmed(os.path.join(BASE_SRC, name + '.png'), base_man[name])[0]
sheet = Image.new('RGBA', (360 * 4, 230 * 2), (0, 0, 0, 0)); ent = []
for k, code in enumerate(['by'] + CODES):
    cv = prev(getbase if code == 'by' else getciv(code), code)
    sheet.alpha_composite(cv, ((k % 4) * 360, (k // 4) * 230))
    ent.append('civprev_%s 0 %d %d 360 230 180.00 230.00 1.0000' % (code, (k % 4) * 360, (k // 4) * 230))
sheet.save(os.path.join(DST, 'civprev.png'), optimize=True)
open(os.path.join(DST, 'civprev.txt'), 'w').write('\n'.join(['@ 0 civprev.png'] + ent) + '\n')
print('previews written')
