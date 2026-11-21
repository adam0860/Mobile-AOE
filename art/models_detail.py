# Detail pass (exec'd by gen_assets.py after every model file): richer roofs/timber/windows/doors, props, pads, trees, decor.
# Redefines helper functions so that every existing model picks up the extra detail automatically.

# ------------------------------------------------------------------ roofs with eaves, fascia and ridge caps
_prism_base = prism_roof
_hip_base = hip_roof
_cone_base = cone

def prism_roof(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
    o = _prism_base(x, y, z, sx, sy, h, mat, over, along, name, ridge_ext)
    hx, hy = sx / 2 + over, sy / 2 + over
    trim = M['woodDark']
    if along == 'x':
        L = 2 * (hx + ridge_ext)
        for s in (-1, 1):
            box(x, y + s * hy, z - 0.015, L + 0.02, 0.05, 0.07, trim)                 # fascia board on both eaves
        box(x, y, z + h - 0.01, L + 0.02, 0.1, 0.07, trim)                           # ridge cap
        for i in range(max(2, int(L / 0.32))):                                        # rafter tails
            px = x - L / 2 + 0.1 + i * (L - 0.2) / max(1, int(L / 0.32) - 1)
            for s in (-1, 1):
                box(px, y + s * (hy + 0.01), z - 0.05, 0.05, 0.06, 0.05, M['wood'])
    else:
        L = 2 * (hy + ridge_ext)
        for s in (-1, 1):
            box(x + s * hx, y, z - 0.015, 0.05, L + 0.02, 0.07, trim)
        box(x, y, z + h - 0.01, 0.1, L + 0.02, 0.07, trim)
        for i in range(max(2, int(L / 0.32))):
            py = y - L / 2 + 0.1 + i * (L - 0.2) / max(1, int(L / 0.32) - 1)
            for s in (-1, 1):
                box(x + s * (hx + 0.01), py, z - 0.05, 0.06, 0.05, 0.05, M['wood'])
    return o

def hip_roof(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
    o = _hip_base(x, y, z, sx, sy, h, mat, over, top, name)
    hx, hy = sx / 2 + over, sy / 2 + over
    trim = M['woodDark']
    box(x, y - hy, z - 0.01, 2 * hx + 0.02, 0.05, 0.07, trim)
    box(x, y + hy, z - 0.01, 2 * hx + 0.02, 0.05, 0.07, trim)
    box(x - hx, y, z - 0.01, 0.05, 2 * hy + 0.02, 0.07, trim)
    box(x + hx, y, z - 0.01, 0.05, 2 * hy + 0.02, 0.07, trim)
    if top < 0.05:
        cone(x, y, z + h - 0.02, 0.05, 0.16, M['woodDark'], verts=6)                 # finial
    return o

def cone(x, y, z, r, h, mat, verts=12, name='cone'):
    o = _cone_base(x, y, z, r, h, mat, verts, name)
    if r > 0.22 and h > 0.3 and mat in (M['tile'], M['slate'], M['imperial'], M['thatch']):
        cyl(x, y, z + h - 0.02, 0.025, 0.14, M['gold'] if mat is M['imperial'] else M['woodDark'], verts=5)   # finial on tower roofs
        cyl(x, y, z - 0.01, r * 1.03, 0.04, M['woodDark'], verts=verts)               # eave ring
    return o

# ------------------------------------------------------------------ timber frames, windows, doors
def beams(hx, hy, z0, z1, mat=None):
    mat = mat or M['woodDark']
    t = 0.11
    for sx in (-hx, hx):
        for sy in (-hy, hy):
            box(sx, sy, z0, t, t, z1 - z0, mat)
    box(0, -hy, z1 - 0.07, 2 * hx + t, 0.09, 0.08, mat)                               # top plate
    box(hx, 0, z1 - 0.07, 0.09, 2 * hy + t, 0.08, mat)
    box(0, -hy, z0, 2 * hx + t, 0.09, 0.07, mat)                                      # sill plate
    box(hx, 0, z0, 0.09, 2 * hy + t, 0.07, mat)
    # studs + diagonal braces on the two visible faces
    for face in ('y', 'x'):
        span = 2 * (hx if face == 'y' else hy)
        n = max(1, int(span / 0.55))
        for i in range(1, n):
            p = -span / 2 + i * span / n
            if face == 'y':
                box(p, -hy, z0, 0.07, 0.08, z1 - z0, mat)
            else:
                box(hx, p, z0, 0.08, 0.07, z1 - z0, mat)
        seg = span / n
        for i in range(n):
            p0 = -span / 2 + i * seg + seg / 2
            if i % 2 == 0:
                ang = math.atan2(z1 - z0, seg)
                ln = math.hypot(z1 - z0, seg) * 0.97
                if face == 'y':
                    d = box(p0, -hy - 0.005, (z0 + z1) / 2 - 0.03, ln, 0.06, 0.06, mat)
                    d.rotation_euler = (0, -ang if i % 4 == 0 else ang, 0)
                else:
                    d = box(hx + 0.005, p0, (z0 + z1) / 2 - 0.03, 0.06, ln, 0.06, mat)
                    d.rotation_euler = (ang if i % 4 == 0 else -ang, 0, 0)

def window(x, y, face='y', w=0.22, h=0.22):
    wm = M['woodDark']
    if face == 'y':
        box(x, y, 0.0, w, 0.05, h, M['black'])
        box(x, y - 0.015, h / 2 - 0.01, w, 0.045, 0.03, wm)                            # mullions
        box(x, y - 0.015, 0.0, 0.03, 0.045, h, wm)
        for dz, hh in ((-0.03, 0.04), (h - 0.01, 0.04)):
            box(x, y - 0.02, dz, w + 0.08, 0.06, hh, M['woodLight'])                   # sill + lintel
        for s in (-1, 1):
            box(x + s * (w / 2 + 0.04), y - 0.025, 0.0, 0.05, 0.05, h, M['woodLight'])
            sh = box(x + s * (w / 2 + 0.11), y - 0.06, 0.0, 0.1, 0.03, h, M['blue'] if (int(x * 10) % 2) else M['woodLight'])    # shutters
            sh.rotation_euler = (0, 0, s * 0.25)
        if int(x * 13) % 3 == 0:                                                       # flower box
            box(x, y - 0.07, -0.07, w + 0.06, 0.07, 0.06, M['wood'])
            for i in range(4):
                sphere(x - w / 2 + 0.04 + i * (w / 3.4), y - 0.08, -0.0, 0.035, M[('flower1', 'flower2', 'flower3')[i % 3]], seg=6)
    else:
        box(x, y, 0.0, 0.05, w, h, M['black'])
        box(x + 0.015, y, h / 2 - 0.01, 0.045, w, 0.03, wm)
        for s in (-1, 1):
            box(x + 0.025, y + s * (w / 2 + 0.04), 0.0, 0.05, 0.05, h, M['woodLight'])
        box(x + 0.02, y, -0.03, 0.06, w + 0.08, 0.04, M['woodLight'])
        box(x + 0.02, y, h - 0.01, 0.06, w + 0.08, 0.04, M['woodLight'])

def door(x, y, face='x', w=0.26, h=0.5, mat=None):
    mat = mat or M['woodDark']
    fr = M['woodLight']
    if face == 'x':
        box(x, y, 0, 0.05, w, h, mat)
        for yy in (-w / 2, w / 2):
            box(x + 0.012, y + yy * 1.0, 0, 0.06, 0.05, h + 0.04, fr)
        box(x + 0.012, y, h - 0.01, 0.07, w + 0.1, 0.06, fr)
        for z in (h * 0.25, h * 0.7):
            box(x + 0.03, y, z, 0.02, w * 0.9, 0.025, M['iron'])
        sphere(x + 0.04, y + w * 0.32, h * 0.46, 0.016, M['gold'], seg=6)
        box(x + 0.1, y, -0.0, 0.14, w + 0.14, 0.05, M['stoneDark'])                    # step
    else:
        box(x, y, 0, w, 0.05, h, mat)
        for xx in (-w / 2, w / 2):
            box(x + xx, y - 0.012, 0, 0.05, 0.06, h + 0.04, fr)
        box(x, y - 0.012, h - 0.01, w + 0.1, 0.07, 0.06, fr)
        for z in (h * 0.25, h * 0.7):
            box(x, y - 0.03, z, w * 0.9, 0.02, 0.025, M['iron'])
        box(x, y - 0.1, 0.0, w + 0.14, 0.14, 0.05, M['stoneDark'])

# ------------------------------------------------------------------ props
def barrel(x, y, z=0, r=0.1, h=0.2):
    cyl(x, y, z, r * 0.93, h, M['wood'], verts=12, r2=r * 0.93)
    cyl(x, y, z + h * 0.25, r * 1.04, h * 0.5, M['wood'], verts=12, r2=r * 1.04)
    for fz in (0.12, 0.45, 0.78):
        cyl(x, y, z + h * fz, r * 1.06, 0.022, M['iron'], verts=12)
    cyl(x, y, z + h - 0.005, r * 0.88, 0.012, M['woodDark'], verts=12)

def crate(x, y, z=0, s=0.18, rz=0.0):
    box(x, y, z, s, s, s, M['woodLight'], rz=rz)
    for q in (-1, 1):
        box(x + math.cos(rz) * q * s * 0.46, y + math.sin(rz) * q * s * 0.46, z, 0.02, s * 1.01, s * 1.02, M['wood'], rz=rz + math.pi / 2)
    box(x, y, z + s - 0.004, s * 1.04, s * 1.04, 0.02, M['wood'], rz=rz)

def sack(x, y, z=0, r=0.11):
    sphere(x, y, z + r * 0.8, r, M['canvas'], sz=0.9, seg=8)
    cyl(x, y, z + r * 1.5, r * 0.45, 0.04, M['canvas'], verts=8, r2=r * 0.3)
    cyl(x, y, z + r * 1.45, r * 0.5, 0.02, M['hide'], verts=8)

def haybale(x, y, z=0, rz=0.0):
    box(x, y, z, 0.34, 0.2, 0.18, M['hay'], rz=rz)
    for q in (-1, 1):
        box(x + math.cos(rz) * q * 0.09, y + math.sin(rz) * q * 0.09, z - 0.005, 0.015, 0.21, 0.19, M['woodDark'], rz=rz)

def basket(x, y, z=0):
    cyl(x, y, z, 0.1, 0.12, M['woodLight'], verts=10, r2=0.13)
    for i in range(5):
        sphere(x + (i - 2) * 0.03, y + (i % 2) * 0.03, z + 0.12, 0.05, M[('crop', 'flower2', 'wheat', 'crop', 'flower1')[i]], seg=6)

def bucket(x, y, z=0):
    cyl(x, y, z, 0.07, 0.1, M['wood'], verts=10, r2=0.085)
    cyl(x, y, z + 0.095, 0.075, 0.012, M['iron'], verts=10)

def woodpile(x, y, z=0, length=0.5, along='x'):
    log_pile(x, y, z, length, (3, 2), 0.045, along)

def lantern(x, y, h=0.8):
    cyl(x, y, 0, 0.025, h, M['woodDark'], verts=6)
    box(x + 0.07, y, h - 0.02, 0.16, 0.03, 0.03, M['woodDark'])
    box(x + 0.15, y, h - 0.12, 0.06, 0.06, 0.1, M['fire'])
    cone(x + 0.15, y, h - 0.02, 0.05, 0.05, M['iron'], verts=4)

def ladder(x, y, h=0.8, rz=0.0, lean=0.2):
    root = []
    for s in (-1, 1):
        c = cyl(x, y + s * 0.07, 0, 0.015, h, M['wood'], verts=5)
        c.rotation_euler = (0, lean, rz)
    for i in range(1, 6):
        b = box(x + math.sin(lean) * h * i / 6 * 0.0, y, h * i / 6, 0.03, 0.17, 0.018, M['wood'])

def bench(x, y, rz=0.0):
    box(x, y, 0.1, 0.42, 0.12, 0.04, M['woodLight'], rz=rz)
    for q in (-1, 1):
        box(x + math.cos(rz) * q * 0.16, y + math.sin(rz) * q * 0.16, 0, 0.04, 0.1, 0.1, M['wood'], rz=rz)

def well(x, y):
    cyl(x, y, 0, 0.26, 0.28, M['stone'], verts=14)
    cyl(x, y, 0.27, 0.2, 0.02, M['black'], verts=14)
    for s in (-1, 1):
        box(x, y + s * 0.24, 0.28, 0.04, 0.04, 0.5, M['woodDark'])
    box(x, y, 0.76, 0.06, 0.58, 0.05, M['woodDark'])
    prism_roof(x, y, 0.8, 0.3, 0.5, 0.18, M['tile'], over=0.06, along='y')

def tuft(x, y, s=1.0, seed=0):
    rnd = random.Random(seed * 17 + int(x * 100) + int(y * 77))
    for i in range(6):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0.0, 0.05) * s
        c = cone(x + math.cos(a) * d, y + math.sin(a) * d, 0, 0.018 * s, rnd.uniform(0.1, 0.2) * s, M['grassBlade'], verts=4)
        c.rotation_euler = (rnd.uniform(-0.35, 0.35), rnd.uniform(-0.35, 0.35), 0)

def bush(x, y, s=1.0, seed=0, flowers=False):
    rnd = random.Random(seed * 31 + int(x * 100) + int(y * 50))
    for i in range(5):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0, 0.12) * s
        sphere(x + math.cos(a) * d, y + math.sin(a) * d, 0.1 * s + rnd.uniform(0, 0.08) * s, rnd.uniform(0.11, 0.17) * s, M['leaf1'] if i % 2 else M['leaf2'], seg=8, noise=0.25, seed=seed * 7 + i, sz=0.8)
    if flowers:
        for i in range(7):
            a = rnd.uniform(0, 6.28); d = rnd.uniform(0.05, 0.17) * s
            sphere(x + math.cos(a) * d, y + math.sin(a) * d, 0.15 * s + rnd.uniform(0, 0.08), 0.022 * s, M[('flower1', 'flower2', 'flower3')[i % 3]], seg=5)

def pebbles(x, y, n=4, seed=0):
    rnd = random.Random(seed * 13 + int(x * 100))
    for i in range(n):
        sphere(x + rnd.uniform(-0.12, 0.12), y + rnd.uniform(-0.12, 0.12), 0.015, rnd.uniform(0.03, 0.07), M['pebble'], seg=6, noise=0.3, seed=i, sz=0.6)

def fence_run(x0, y0, x1, y1, h=0.3):
    n = max(2, int(math.hypot(x1 - x0, y1 - y0) / 0.28))
    for i in range(n + 1):
        t = i / n
        cyl(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 0, 0.024, h, M['wood'], verts=6)
    for z in (0.1, 0.22):
        mx, my = (x0 + x1) / 2, (y0 + y1) / 2
        ln = math.hypot(x1 - x0, y1 - y0)
        b = box(mx, my, z, ln, 0.022, 0.03, M['woodLight'])
        b.rotation_euler = (0, 0, math.atan2(y1 - y0, x1 - x0))

def awning(x, y, z, w, d, face='x', mat=None, tilt=0.35):
    """striped cloth awning over a door/window; face = the wall it is attached to"""
    mat = mat or M['awningBlue']
    if face == 'x':
        a = box(x + d / 2, y, z, d, w, 0.03, mat); a.rotation_euler = (0, tilt, 0)
        for s in (-1, 1):
            cyl(x + d, y + s * w / 2, 0, 0.02, z - 0.04, M['woodDark'], verts=6)
        box(x + d + 0.02, y, z - 0.04, 0.03, w, 0.06, mat)
    else:
        a = box(x, y - d / 2, z, w, d, 0.03, mat); a.rotation_euler = (-tilt, 0, 0)
        for s in (-1, 1):
            cyl(x + s * w / 2, y - d, 0, 0.02, z - 0.04, M['woodDark'], verts=6)
        box(x, y - d - 0.02, z - 0.04, w, 0.03, 0.06, mat)

def pad(fw, fh, seed=0, spread=0.38, mat=None):
    """irregular packed-earth pad under a building, with cobbles and grass tufts breaking up the edge"""
    rnd = random.Random(seed)
    mat = mat or M['dirt']
    n = 18
    verts = [(0, 0, 0.012)]
    for i in range(n):
        a = 2 * math.pi * i / n
        # rounded rectangle radius profile
        ca, sa = math.cos(a), math.sin(a)
        rx, ry = fw / 2 + spread * rnd.uniform(0.35, 1.0), fh / 2 + spread * rnd.uniform(0.35, 1.0)
        t = 1.0 / max(abs(ca) / rx, abs(sa) / ry)
        verts.append((ca * t * 0.97, sa * t * 0.97, 0.012))
    faces = [(0, 1 + i, 1 + (i + 1) % n) for i in range(n)]
    mesh_obj('pad', verts, faces, mat)
    for i in range(int(8 + (fw + fh) * 2)):
        a = rnd.uniform(0, 6.28)
        ca, sa = math.cos(a), math.sin(a)
        t = 1.0 / max(abs(ca) / (fw / 2 + spread * 0.9), abs(sa) / (fh / 2 + spread * 0.9))
        r = rnd.uniform(0.9, 1.08)
        px, py = ca * t * r, sa * t * r
        if rnd.random() < 0.5:
            tuft(px, py, rnd.uniform(0.8, 1.3), i)
        else:
            pebbles(px, py, 2, i)

def decorate(name, fw, fh, seed):
    rnd = random.Random(seed)
    pad(fw, fh, seed)
    hx, hy = fw / 2, fh / 2
    spots = []
    for i in range(int(3 + (fw + fh) * 0.9)):
        side = rnd.choice(('x+', 'y-', 'x-', 'y+', 'x+', 'y-'))
        o = rnd.uniform(0.2, 0.42)
        t = rnd.uniform(-0.8, 0.8)
        if side == 'x+': spots.append((hx + o, t * hy))
        elif side == 'x-': spots.append((-hx - o, t * hy))
        elif side == 'y+': spots.append((t * hx, hy + o))
        else: spots.append((t * hx, -hy - o))
    kinds = [barrel, crate, sack, basket, bucket, haybale, bush, bush, woodpile, bench, lantern]
    for k, (px, py) in enumerate(spots):
        c = rnd.choice(kinds)
        if c is barrel: barrel(px, py)
        elif c is crate: crate(px, py, 0, rnd.uniform(0.14, 0.2), rnd.uniform(0, 1.5))
        elif c is sack: sack(px, py)
        elif c is basket: basket(px, py)
        elif c is bucket: bucket(px, py)
        elif c is haybale: haybale(px, py, 0, rnd.uniform(0, 3))
        elif c is bush: bush(px, py, rnd.uniform(0.9, 1.4), k, rnd.random() < 0.5)
        elif c is woodpile: woodpile(px, py, 0, 0.4, 'x' if rnd.random() < 0.5 else 'y')
        elif c is bench: bench(px, py, rnd.choice((0, 1.57)))
        elif c is lantern: lantern(px, py, 0.75)
