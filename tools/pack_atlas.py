"""Pack Blender renders (art/renders) into texture atlas pages (each <= 2048x2048) for the game.
   python tools/pack_atlas.py  ->  assets/atlas_*.png, assets/ground.png, assets/atlas.txt
atlas.txt:   '@ <page> <file>' lines declare pages, then one line per sprite:
   name page x y w h ax ay scale
   (ax,ay) = anchor pixel inside the packed rect (footprint centre on the ground); scale = sprite px per game px at zoom 1.
"""
import json, os
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'art', 'renders')
DST = os.path.join(ROOT, 'assets')
BASE_PXM = 45.2548   # px per metre in the game's zoom-1 world
PAGE = 2048

man = json.load(open(os.path.join(SRC, 'manifest.json')))
os.makedirs(DST, exist_ok=True)
for f in os.listdir(DST):
    if (f.startswith('atlas_') or f.startswith('ground')) and f.endswith('.png'):
        os.remove(os.path.join(DST, f))

def load(names, pad=2):
    items = []
    for n in names:
        im = Image.open(os.path.join(SRC, n + '.png')).convert('RGBA')
        bb = im.getchannel('A').point(lambda v: 255 if v > 3 else 0).getbbox() or (0, 0, 1, 1)
        im = im.crop(bb)
        m = man[n]
        items.append((n, im, m['ax'] - bb[0], m['ay'] - bb[1], m['pxm'] / BASE_PXM))
    items.sort(key=lambda t: -t[1].height)
    return items

def pack_pages(items, pad=2):
    pages, cur = [], None
    x = y = rowh = 0
    def new_page():
        return {'items': [], 'h': 0}
    cur = new_page()
    for it in items:
        w, h = it[1].size
        if x + w + pad > PAGE:
            x = 0; y += rowh + pad; rowh = 0
        if y + h > PAGE:                      # start a new page
            pages.append(cur); cur = new_page(); x = y = rowh = 0
        cur['items'].append((it, x, y)); x += w + pad; rowh = max(rowh, h)
        cur['h'] = max(cur['h'], y + h)
    pages.append(cur)
    return pages

names = sorted(man.keys())
units = [n for n in names if n.startswith('u_') or n.startswith('uw_')]
grounds = [n for n in names if n.startswith('ground')]
world = [n for n in names if n not in units and n not in grounds]
out = []; decl = []
page_no = 0
for tag, group in (('world', world), ('units', units)):
    for pg in pack_pages(load(group)):
        H = 1
        while H < pg['h']: H *= 2
        atlas = Image.new('RGBA', (PAGE, H), (0, 0, 0, 0))
        fname = 'atlas_%s_%d.png' % (tag, page_no)
        for (n, im, ax, ay, sc), px, py in pg['items']:
            atlas.paste(im, (px, py))
            out.append('%s %d %d %d %d %d %.2f %.2f %.4f' % (n, page_no, px, py, im.width, im.height, ax, ay, sc))
        atlas.save(os.path.join(DST, fname), optimize=True)
        print(fname, atlas.size, len(pg['items']), 'sprites')
        decl.append('@ %d %s%s' % (page_no, fname, ' units' if tag == 'units' else '')); page_no += 1
for gname in grounds:                      # ground chunks: one texture each, untrimmed so the anchors stay exact
    g = Image.open(os.path.join(SRC, gname + '.png')).convert('RGBA')
    g.save(os.path.join(DST, gname + '.png'), optimize=True)
    m = man[gname]
    decl.append('@ %d %s.png' % (page_no, gname))
    out.append('%s %d 0 0 %d %d %.2f %.2f %.4f' % (gname, page_no, g.width, g.height, m['ax'], m['ay'], m['pxm'] / BASE_PXM)); page_no += 1
open(os.path.join(DST, 'atlas.txt'), 'w').write('\n'.join(decl + out) + '\n')
print('wrote', len(out), 'entries,', len(decl), 'pages')
