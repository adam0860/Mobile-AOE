"""Contact sheet of civ renders for quick visual checks:  python tools/civ_sheet.py <dir> <out.png> name1 name2 ...  (names without extension)"""
import sys, os
from PIL import Image
d, out, names = sys.argv[1], sys.argv[2], sys.argv[3:]
ims = []
for n in names:
    p = os.path.join(d, n + '.png')
    if os.path.exists(p):
        im = Image.open(p).convert('RGBA'); bb = im.getchannel('A').point(lambda v: 255 if v > 3 else 0).getbbox()
        ims.append(im.crop(bb) if bb else im)
if not ims: sys.exit('nothing')
S = 0.62
ims = [i.resize((int(i.width * S), int(i.height * S)), Image.LANCZOS) for i in ims]
cols = min(4, len(ims)); rows = (len(ims) + cols - 1) // cols
cw = max(i.width for i in ims) + 10; ch = max(i.height for i in ims) + 10
sheet = Image.new('RGBA', (cols * cw, rows * ch), (96, 132, 78, 255))
for k, i in enumerate(ims):
    sheet.alpha_composite(i, ((k % cols) * cw + (cw - i.width) // 2, (k // cols) * ch + (ch - i.height) // 2))
sheet.convert('RGB').save(out)
print(sheet.size)
