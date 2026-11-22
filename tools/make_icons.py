"""Builds every icon size from art/icon/emblem.png (see art/make_icon.py):
   art/icon/icon_1024.png, art/icon/DawnOfKings.ico, assets/icon.png and the Android launcher icons (legacy + adaptive)."""
import os, math
from PIL import Image, ImageDraw, ImageFilter, ImageChops

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
emb = Image.open(os.path.join(ROOT, 'art', 'icon', 'emblem.png')).convert('RGBA')
bb = emb.getchannel('A').point(lambda v: 255 if v > 8 else 0).getbbox()
emb = emb.crop(bb)

def background(n):
    """royal blue radial glow with soft light rays and a vignette (no rounding)"""
    cx, cy = n * 0.5, n * 0.42
    img = Image.new('RGB', (n, n))
    px = img.load()
    for y in range(n):
        for x in range(n):
            d = math.hypot(x - cx, y - cy) / (n * 0.75)
            t = min(1.0, d)
            r = int(48 * (1 - t) + 6 * t); g = int(86 * (1 - t) + 12 * t); b = int(190 * (1 - t) + 38 * t)
            px[x, y] = (r, g, b)
    rays = Image.new('L', (n, n), 0); dr = ImageDraw.Draw(rays)
    for k in range(14):
        a0 = k * (2 * math.pi / 14); a1 = a0 + math.pi / 28
        dr.polygon([(cx, cy), (cx + math.cos(a0) * n, cy + math.sin(a0) * n), (cx + math.cos(a1) * n, cy + math.sin(a1) * n)], fill=34)
    rays = rays.filter(ImageFilter.GaussianBlur(n / 60))
    img = Image.composite(Image.new('RGB', (n, n), (150, 190, 255)), img, rays)
    return img

def emblem_on(canvas, frac, dy=0.0):
    n = canvas.size[0]; s = frac * n / max(emb.size)
    e = emb.resize((max(1, int(emb.width * s)), max(1, int(emb.height * s))), Image.LANCZOS)
    sh = Image.new('RGBA', canvas.size, (0, 0, 0, 0)); shadow = Image.new('RGBA', e.size, (0, 0, 0, 150)); shadow.putalpha(e.getchannel('A').point(lambda v: v * 0.55))
    ox, oy = (n - e.width) // 2, int((n - e.height) / 2 + dy * n)
    sh.alpha_composite(shadow, (ox + int(n * 0.012), oy + int(n * 0.022))); sh = sh.filter(ImageFilter.GaussianBlur(n / 90))
    canvas.alpha_composite(sh); canvas.alpha_composite(e, (ox, oy))
    return canvas

def rounded(img, radius_frac=0.22):
    n = img.size[0]; m = Image.new('L', (n, n), 0); ImageDraw.Draw(m).rounded_rectangle([0, 0, n - 1, n - 1], radius=int(n * radius_frac), fill=255)
    out = img.copy(); out.putalpha(m); return out

N = 1024
bg = background(N).convert('RGBA')
full = bg.copy()
d = ImageDraw.Draw(full)
inset = int(N * 0.035); d.rounded_rectangle([inset, inset, N - inset, N - inset], radius=int(N * 0.19), outline=(244, 190, 80, 255), width=int(N * 0.012))
inset2 = int(N * 0.05); d.rounded_rectangle([inset2, inset2, N - inset2, N - inset2], radius=int(N * 0.175), outline=(120, 80, 20, 200), width=int(N * 0.004))
full = emblem_on(full, 0.80, 0.0)
icon = rounded(full)
os.makedirs(os.path.join(ROOT, 'art', 'icon'), exist_ok=True)
icon.save(os.path.join(ROOT, 'art', 'icon', 'icon_1024.png'))
icon.resize((256, 256), Image.LANCZOS).save(os.path.join(ROOT, 'assets', 'icon.png'))
icon.save(os.path.join(ROOT, 'art', 'icon', 'DawnOfKings.ico'), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])

res = os.path.join(ROOT, 'android', 'app', 'src', 'main', 'res')
legacy = {'mdpi': 48, 'hdpi': 72, 'xhdpi': 96, 'xxhdpi': 144, 'xxxhdpi': 192}
for k, px in legacy.items():
    d_ = os.path.join(res, 'mipmap-' + k); os.makedirs(d_, exist_ok=True)
    icon.resize((px, px), Image.LANCZOS).save(os.path.join(d_, 'ic_launcher.png'))
    circ = Image.new('L', (N, N), 0); ImageDraw.Draw(circ).ellipse([0, 0, N - 1, N - 1], fill=255); r = full.copy(); r.putalpha(circ)
    r.resize((px, px), Image.LANCZOS).save(os.path.join(d_, 'ic_launcher_round.png'))
adaptive = {'mdpi': 108, 'hdpi': 162, 'xhdpi': 216, 'xxhdpi': 324, 'xxxhdpi': 432}
fg = Image.new('RGBA', (N, N), (0, 0, 0, 0)); fg = emblem_on(fg, 0.58, 0.0)
bgonly = background(N)
for k, px in adaptive.items():
    d_ = os.path.join(res, 'mipmap-' + k)
    fg.resize((px, px), Image.LANCZOS).save(os.path.join(d_, 'ic_launcher_foreground.png'))
    bgonly.resize((px, px), Image.LANCZOS).save(os.path.join(d_, 'ic_launcher_background.png'))
os.makedirs(os.path.join(res, 'mipmap-anydpi-v26'), exist_ok=True)
xml = '<?xml version="1.0" encoding="utf-8"?>\n<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n    <background android:drawable="@mipmap/ic_launcher_background"/>\n    <foreground android:drawable="@mipmap/ic_launcher_foreground"/>\n</adaptive-icon>\n'
for nm in ('ic_launcher.xml', 'ic_launcher_round.xml'):
    open(os.path.join(res, 'mipmap-anydpi-v26', nm), 'w', encoding='utf-8').write(xml)
print('icons written')
