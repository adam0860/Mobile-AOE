# Civilization architecture sets (exec'd by gen_assets.py after every model file when --civ <code> is given).
# One Blender run renders one civ: it re-skins the shared building models by
#   1. swapping the material library entries (wall / roof / trim palettes),
#   2. replacing the roof helpers (prism_roof / hip_roof / cone) with civ-specific roof shapes,
#   3. replacing window / door / beams / chimney / crenellations / flag / decorate with civ-specific details,
#   4. providing a hand-built m_house for every civ.
# Footprints and sprite anchors are unchanged, so the game can swap a civ's sprite for the stock one 1:1.
CIV_CODES = ('sl', 'fr', 'ce', 'ja', 'sp', 'mo')
CIV_ALLOW = ('house_', 'townhall_', 'barracks_', 'archery_', 'tower_', 'stable_', 'siege_', 'monastery_', 'keep_', 'blacksmith_', 'market_', 'castle_', 'university_', 'bombard_')

# ------------------------------------------------------------------ geometry helpers
def lathe(x, y, z, prof, mat, verts=14, name='lathe', rz=0.0):
    """solid of revolution from a [(radius, height)] profile (bottom -> top)"""
    v, f, rings = [], [], []
    for (r, h) in prof:
        if r < 1e-3:
            v.append((0, 0, h)); rings.append([len(v) - 1])
        else:
            idx = []
            for i in range(verts):
                a = 2 * math.pi * i / verts
                v.append((math.cos(a) * r, math.sin(a) * r, h)); idx.append(len(v) - 1)
            rings.append(idx)
    for k in range(len(prof) - 1):
        a, b = rings[k], rings[k + 1]
        for i in range(verts):
            j = (i + 1) % verts
            if len(a) == 1 and len(b) == 1:
                continue
            if len(a) == 1:
                f.append((a[0], b[j], b[i]))
            elif len(b) == 1:
                f.append((a[i], a[j], b[0]))
            else:
                f.append((a[i], a[j], b[j], b[i]))
    if len(rings[0]) > 1:
        f.append(tuple(reversed(rings[0])))
    if len(rings[-1]) > 1:
        f.append(tuple(rings[-1]))
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    o.rotation_euler = (0, 0, rz)
    return o

def scaled(prof, r, h):
    return [(a * r, b * h) for (a, b) in prof]

def hip_surf(x, y, z, hx, hy, h, mat, tx=0.0, ty=0.0, expo=1.0, lift=0.0, K=6, name='hipc'):
    """pyramid / hip roof surface: eave half-extents hx,hy rising to a top of half-extents tx,ty; `expo` bends the slope
    (>1 concave like an eastern roof, <1 convex like a dome), `lift` raises the eave corners."""
    v, rings = [], []
    for k in range(K + 1):
        t = k / K
        cx, cy = hx + (tx - hx) * t, hy + (ty - hy) * t
        zz = h * (t ** expo)
        lf = lift * (1 - t) ** 2.2
        if k == K and cx < 1e-3 and cy < 1e-3:
            v.append((0, 0, zz)); rings.append([len(v) - 1]); continue
        pts = [(-cx, -cy, zz + lf), (0, -cy, zz), (cx, -cy, zz + lf), (cx, 0, zz), (cx, cy, zz + lf), (0, cy, zz), (-cx, cy, zz + lf), (-cx, 0, zz)]
        idx = []
        for p in pts:
            v.append(p); idx.append(len(v) - 1)
        rings.append(idx)
    f = []
    for k in range(K):
        a, b = rings[k], rings[k + 1]
        for i in range(8):
            j = (i + 1) % 8
            if len(b) == 1:
                f.append((a[i], a[j], b[0]))
            else:
                f.append((a[i], a[j], b[j], b[i]))
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    return o

def gable_surf(x, y, z, hx, hy, h, mat, along='x', expo=1.0, upturn=0.0, ext=0.0, K=6, gmat=None, name='gablec'):
    """gable roof with a bent cross-section; hx,hy include the eave overhang; ridge runs along `along`"""
    span = hy if along == 'x' else hx
    L = (hx if along == 'x' else hy) + ext
    prof = []
    for k in range(K + 1):
        t = k / K
        prof.append((-span * (1 - t), h * (t ** expo) + upturn * (1 - t) ** 3))
    full = prof + [(-s, zz) for (s, zz) in reversed(prof[:-1])]
    v, a_idx, b_idx = [], [], []
    for (s, zz) in full:
        for sgn, lst in ((-1, a_idx), (1, b_idx)):
            v.append((sgn * L, s, zz) if along == 'x' else (s, sgn * L, zz)); lst.append(len(v) - 1)
    f = []
    for i in range(len(full) - 1):
        f.append((a_idx[i], b_idx[i], b_idx[i + 1], a_idx[i + 1]))
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    if gmat is not None:                                       # filled gable ends in a wall material
        fv = []
        for sgn in (-1, 1):
            base = [((sgn * (L - 0.04)), s, zz) if along == 'x' else (s, sgn * (L - 0.04), zz) for (s, zz) in full]
            idx0 = len(fv)
            fv += base
            mesh_obj(name + '_end', [p for p in base], [tuple(range(len(base)))], gmat).location = (x, y, z)
    return o

def fascia(x, y, z, hx, hy, mat=None, th=0.06, lift=0.0):
    mat = mat or M['woodDark']
    box(x, y - hy, z - 0.04, 2 * hx + 0.02, 0.05, th, mat)
    box(x, y + hy, z - 0.04, 2 * hx + 0.02, 0.05, th, mat)
    box(x - hx, y, z - 0.04, 0.05, 2 * hy + 0.02, th, mat)
    box(x + hx, y, z - 0.04, 0.05, 2 * hy + 0.02, th, mat)

def slope_board(x, y, z, span, h, mat, along='y', w=0.07, thick=0.035, off=0.0):
    """barge board running up each gable slope (both ends, both slopes)"""
    ang = math.atan2(h, span)
    ln = math.hypot(span, h)
    for end in (-1, 1):
        for sd in (-1, 1):
            if along == 'y':
                o = box(x + sd * span / 2, y + end * off, z + h / 2 - w / 2, ln, thick, w, mat)
                o.rotation_euler = (0, sd * ang, 0)
            else:
                o = box(x + end * off, y + sd * span / 2, z + h / 2 - w / 2, thick, ln, w, mat)
                o.rotation_euler = (-sd * ang, 0, 0)

def onion(x, y, z, r, h, mat, tip=True, verts=14):
    lathe(x, y, z, scaled([(0.42, 0), (0.40, 0.06), (0.62, 0.17), (0.94, 0.34), (1.0, 0.45), (0.86, 0.58), (0.55, 0.72), (0.26, 0.86), (0.08, 0.96), (0.0, 1.0)], r, h), mat, verts)
    if tip:
        cyl(x, y, z + h * 0.97, 0.012, 0.16, M['gold'], verts=5)
        box(x, y, z + h * 0.97 + 0.07, 0.07, 0.012, 0.012, M['gold'])
        box(x, y, z + h * 0.97 + 0.1, 0.012, 0.012, 0.05, M['gold'])
        sphere(x, y, z + h * 0.97 + 0.02, 0.025, M['gold'], seg=6)

def spire(x, y, z, r, h, mat, verts=8, gold_tip=True):
    lathe(x, y, z, scaled([(1.0, 0), (0.8, 0.28), (0.55, 0.56), (0.32, 0.8), (0.13, 0.94), (0.0, 1.0)], r, h), mat, verts)
    if gold_tip:
        cyl(x, y, z + h - 0.02, 0.014, 0.16, M['gold'], verts=5)
        sphere(x, y, z + h + 0.1, 0.03, M['gold'], seg=6)

# ------------------------------------------------------------------ shared small props
_RAW_PRISM = _prism_base          # un-decorated gable prism (captured before the civ roofs replace it)

def arch_opening(x, y, face, w, h, mat, kind='round', depth=0.05, frame=None):
    """dark arched opening (door/window) with an optional stone frame; kind = round | point | flat"""
    top = 0.0 if kind == 'flat' else (w / 2 if kind == 'round' else w * 0.85)
    bh = h - top
    if face == 'x':
        box(x, y, 0, depth, w, bh, mat)
        if kind == 'round':
            cyl(x + 0.005, y, bh, w / 2, depth, mat, verts=10, ry=math.pi / 2)
        elif kind == 'point':
            _RAW_PRISM(x + 0.005, y, bh, depth, w, top, mat, 0.0, 'x', 'ptop', 0.0)
        if frame:
            for s in (-1, 1):
                box(x + 0.01, y + s * (w / 2 + 0.035), 0, 0.06, 0.06, bh + (0.05 if kind == 'flat' else 0.0), frame)
            if kind == 'flat':
                box(x + 0.012, y, h, 0.07, w + 0.14, 0.06, frame)
            elif kind == 'round':
                cyl(x + 0.012, y, bh, w / 2 + 0.05, 0.05, frame, verts=10, ry=math.pi / 2)
                cyl(x + 0.02, y, bh, w / 2, 0.05, mat, verts=10, ry=math.pi / 2)
            else:
                _RAW_PRISM(x + 0.012, y, bh, 0.05, w + 0.12, top + 0.06, frame, 0.0, 'x', 'pframe', 0.0)
                _RAW_PRISM(x + 0.02, y, bh, depth, w, top, mat, 0.0, 'x', 'ptop2', 0.0)
    else:
        box(x, y, 0, w, depth, bh, mat)
        if kind == 'round':
            cyl(x, y - 0.005, bh, w / 2, depth, mat, verts=10, rx=math.pi / 2).rotation_euler = (math.pi / 2, 0, 0)
        elif kind == 'point':
            _RAW_PRISM(x, y - 0.005, bh, w, depth, top, mat, 0.0, 'y', 'ptop', 0.0)
        if frame:
            for s in (-1, 1):
                box(x + s * (w / 2 + 0.035), y - 0.01, 0, 0.06, 0.06, bh + (0.05 if kind == 'flat' else 0.0), frame)
            if kind == 'flat':
                box(x, y - 0.012, h, w + 0.14, 0.07, 0.06, frame)
            elif kind == 'round':
                cyl(x, y - 0.012, bh, w / 2 + 0.05, 0.05, frame, verts=10).rotation_euler = (math.pi / 2, 0, 0)
                cyl(x, y - 0.02, bh, w / 2, 0.05, mat, verts=10).rotation_euler = (math.pi / 2, 0, 0)
            else:
                _RAW_PRISM(x, y - 0.012, bh, w + 0.12, 0.05, top + 0.06, frame, 0.0, 'y', 'pframe', 0.0)
                _RAW_PRISM(x, y - 0.02, bh, w, depth, top, mat, 0.0, 'y', 'ptop2', 0.0)

def tree_birch(x, y, s=1.0, seed=0):
    rnd = random.Random(seed * 7 + int(x * 100))
    cyl(x, y, 0, 0.03 * s, 0.7 * s, M['white'], verts=6, r2=0.02 * s)
    for i in range(5):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0, 0.1) * s
        sphere(x + math.cos(a) * d, y + math.sin(a) * d, (0.55 + i * 0.1) * s, rnd.uniform(0.1, 0.15) * s, M['leaf2'], seg=7, noise=0.25, seed=i + seed, sz=0.8)

def tree_cypress(x, y, s=1.0):
    cyl(x, y, 0, 0.025 * s, 0.1 * s, M['bark'], verts=5)
    lathe(x, y, 0.06 * s, scaled([(0.0, 0), (0.12, 0.0), (0.16, 0.3), (0.13, 0.7), (0.07, 0.95), (0.0, 1.1)], s, s * 0.95), M['leafPine'], 9)

def tree_pine_bonsai(x, y, s=1.0):
    cyl(x, y, 0, 0.03 * s, 0.22 * s, M['bark'], verts=6).rotation_euler = (0.2, 0.1, 0)
    for k, (dx, dy, zz, rr) in enumerate(((0, 0, 0.28, 0.17), (0.08, -0.05, 0.38, 0.13), (-0.07, 0.06, 0.2, 0.12))):
        sphere(x + dx * s, y + dy * s, zz * s, rr * s, M['leafPine'], sz=0.55, seg=8, noise=0.2, seed=k)
    cyl(x, y, 0, 0.1 * s, 0.05, M['stoneDark'], verts=8)

def pot(x, y, s=1.0, plant=True):
    cyl(x, y, 0, 0.07 * s, 0.13 * s, M['tile'], verts=10, r2=0.1 * s)
    cyl(x, y, 0.13 * s, 0.1 * s, 0.02, M['tile'], verts=10)
    if plant:
        sphere(x, y, 0.2 * s, 0.1 * s, M['leaf2'], sz=0.7, seg=7, noise=0.25, seed=int(x * 50))
        sphere(x + 0.03, y, 0.27 * s, 0.03, M['flower2'], seg=5)

def jar(x, y, s=1.0):
    lathe(x, y, 0, scaled([(0.0, 0), (0.07, 0), (0.1, 0.1), (0.11, 0.2), (0.08, 0.3), (0.05, 0.36), (0.06, 0.4), (0.0, 0.4)], s, s), M['tile'], 10)

def shield_stand(x, y, rz=0.0):
    for s in (-1, 1):
        cyl(x, y + s * 0.16, 0, 0.02, 0.5, M['woodDark'], verts=5)
    box(x, y, 0.38, 0.03, 0.4, 0.04, M['woodDark'])
    cyl(x - 0.025, y, 0.2, 0.17, 0.035, M['blue'], verts=3, ry=math.pi / 2, rz=0.0)
    box(x - 0.045, y, 0.18, 0.012, 0.3, 0.05, M['gold'])

def standing_stone(x, y, h=0.5, seed=0):
    o = box(x, y, 0, 0.17, 0.12, h, M['stoneDark'], rz=seed * 0.7)
    o.rotation_euler = (0.06 * ((seed % 3) - 1), 0.07, seed * 0.7)
    box(x, y, h * 0.55, 0.19, 0.06, 0.03, M['woodDark'], rz=seed * 0.7 + 0.5)

def high_cross(x, y, h=0.62):
    box(x, y, 0, 0.14, 0.14, 0.1, M['stoneDark'])
    box(x, y, 0.1, 0.07, 0.07, h, M['stone'])
    box(x, y, h * 0.62, 0.3, 0.07, 0.07, M['stone'])
    cyl(x, y, h * 0.62 - 0.07, 0.15, 0.06, M['stone'], verts=14, rx=math.pi / 2, r2=0.15)
    cyl(x, y, h * 0.62 - 0.03, 0.09, 0.07, M['dirt'], verts=12, rx=math.pi / 2)

def cauldron(x, y):
    for a in (0.4, 2.5, 4.6):
        c = cyl(x + math.cos(a) * 0.1, y + math.sin(a) * 0.1, 0, 0.012, 0.42, M['woodDark'], verts=5)
        c.rotation_euler = (-math.sin(a) * 0.2, math.cos(a) * 0.2, 0)
    sphere(x, y, 0.26, 0.12, M['iron'], sz=0.8, seg=8)
    for i in range(5):
        sphere(x + math.cos(i) * 0.14, y + math.sin(i) * 0.14, 0.03, 0.04, M['rock'], seg=5)
    cone(x, y, 0.02, 0.07, 0.15, M['fire'], verts=6)

def heather(x, y, s=1.0, seed=0):
    rnd = random.Random(seed * 5 + int(x * 70))
    for i in range(6):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0, 0.12) * s
        sphere(x + math.cos(a) * d, y + math.sin(a) * d, 0.07 * s, rnd.uniform(0.06, 0.1) * s, M['flower2'] if i % 3 else M['leaf1'], seg=6, noise=0.3, seed=i, sz=0.7)

def stone_lantern(x, y, s=1.0):
    cyl(x, y, 0, 0.07 * s, 0.05, M['stone'], verts=6)
    cyl(x, y, 0.05 * s, 0.03 * s, 0.22 * s, M['stone'], verts=6)
    cyl(x, y, 0.27 * s, 0.1 * s, 0.03, M['stone'], verts=6)
    box(x, y, 0.3 * s, 0.1 * s, 0.1 * s, 0.1 * s, M['stoneDark'])
    box(x, y, 0.31 * s, 0.08 * s, 0.09 * s, 0.07 * s, M['fire'])
    hip_roof(x, y, 0.4 * s, 0.14 * s, 0.14 * s, 0.09 * s, M['stone'], over=0.04, top=0.0)

def bamboo(x, y, s=1.0, seed=0):
    rnd = random.Random(seed * 3 + int(x * 90))
    for i in range(5):
        px, py = x + rnd.uniform(-0.07, 0.07), y + rnd.uniform(-0.07, 0.07)
        hh = rnd.uniform(0.6, 0.95) * s
        c = cyl(px, py, 0, 0.018, hh, M['leaf2'], verts=5, r2=0.014)
        c.rotation_euler = (rnd.uniform(-0.08, 0.08), rnd.uniform(-0.08, 0.08), 0)
        for k in range(2):
            sphere(px, py, hh * (0.65 + 0.3 * k), 0.07, M['leaf1'], sz=0.35, seg=6)

def haystack(x, y, s=1.0):
    cyl(x, y, 0, 0.17 * s, 0.1 * s, M['hay'], verts=10, r2=0.2 * s)
    cone(x, y, 0.1 * s, 0.2 * s, 0.34 * s, M['hay'], verts=10)
    cyl(x, y, 0.42 * s, 0.01, 0.12, M['woodDark'], verts=4)

def horse_post(x, y, rz=0.0):
    box(x, y, 0, 0.9, 0.04, 0.04, M['woodDark'], rz=rz).location = (x, y, 0.32)
    for s in (-1, 1):
        cyl(x + math.cos(rz) * s * 0.42, y + math.sin(rz) * s * 0.42, 0, 0.03, 0.4, M['woodDark'], verts=5)

def tug(x, y, h=0.9, team='blue'):
    """Mongol standard: pole, spear head, disc and hanging horse-hair tassels"""
    cyl(x, y, 0, 0.022, h, M['woodDark'], verts=5)
    cone(x, y, h, 0.035, 0.12, M['gold'], verts=5)
    cyl(x, y, h - 0.1, 0.1, 0.02, M['gold'], verts=10, rx=math.pi / 2)
    for i in range(6):
        a = i / 6 * 2 * math.pi
        o = box(x + math.cos(a) * 0.09, y + math.sin(a) * 0.09, h - 0.34, 0.03, 0.03, 0.26, team_mat(team) if i % 2 else M['white'])
    cyl(x, y, h - 0.14, 0.11, 0.04, M['black'], verts=10)

def felt_rolls(x, y):
    for i in range(3):
        cyl(x + (i - 1) * 0.1, y, 0.08, 0.06, 0.34, M['canvas'], verts=8, rx=math.pi / 2).rotation_euler = (math.pi / 2, 0, 0.3 * i)

def wicker_fence(x0, y0, x1, y1, h=0.28):
    fence_run(x0, y0, x1, y1, h)

def firewood(x, y):
    log_pile(x, y, 0, 0.4, (3, 2), 0.04, 'x')

def beehive(x, y):
    lathe(x, y, 0.1, scaled([(0.08, 0), (0.1, 0.05), (0.09, 0.14), (0.06, 0.2), (0.0, 0.22)], 1, 1), M['hay'], 8)
    for s in (-1, 1):
        cyl(x + s * 0.08, y, 0, 0.015, 0.1, M['wood'], verts=5)
    box(x, y, 0.09, 0.2, 0.2, 0.02, M['wood'])

def well_sweep(x, y):
    cyl(x, y, 0, 0.19, 0.22, M['stone'], verts=12)
    cyl(x, y, 0.21, 0.14, 0.02, M['black'], verts=12)
    cyl(x - 0.28, y, 0, 0.025, 0.7, M['woodDark'], verts=5)
    o = cyl(x - 0.1, y, 0.62, 0.016, 0.6, M['wood'], verts=5)
    o.rotation_euler = (0, math.radians(75), 0)
    bucket(x + 0.18, y, 0.1)

def civ_pad_mats(key, rr):
    return None

# ------------------------------------------------------------------ civilization definitions
def _blank(h):
    return h

def civ_apply(code):
    global M, prism_roof, hip_roof, cone, round_tower, window, door, beams, chimney, crenellations, flag, decorate, m_house, _prism_base, civ_pre, awning, banner_pole
    C = {}
    # ------------------------------------------------------------- palettes
    def mats(**kw):
        for k, v in kw.items():
            M[k] = v
    _prism_orig = prism_roof
    _hip_orig = hip_roof
    _cone_orig = cone
    _round_orig = round_tower
    _beams_orig = beams
    _window_orig = window
    _door_orig = door
    _chim_orig = chimney
    _cren_orig = crenellations
    _flag_orig = flag
    _awn_orig = awning
    def roofmat(mat):
        return mat in (M['tile'], M['slate'], M['imperial'], M['thatch'])
    props = []                       # civ props: callables (px, py, rnd, k) placed around buildings
    civ_pre = lambda name: None

    if code == 'ja':
        # ===================================================== Japanese: white plaster, dark timber, upswept tiled roofs
        mats(plaster=mat2('ja_plaster', stucco('#f1ede0', '#fcf9ee', '#cfc7b0')),
             stone=mat2('ja_stone', masonry('#8e8f8d', '#b0b1ad', '#5a5b59', 0.55, 0.3)),
             stoneDark=mat2('ja_stoneDark', masonry('#60615f', '#7c7d7a', '#3a3b3a', 0.5, 0.28)),
             thatch=mat2('ja_thatch', thatch('#a08a54', '#cdb67c', '#5d4a24')),
             tile=mat2('ja_tile', shingles('#3c4147', '#5b626a', '#1f2327', 0.12, 0.11)),
             slate=mat2('ja_slate', shingles('#2d3237', '#474d54', '#15181b', 0.12, 0.11)),
             imperial=mat2('ja_imp', shingles('#1c2a3e', '#34485f', '#0c131c', 0.12, 0.11)),
             marble=mat2('ja_marble', masonry('#c9c8c2', '#e6e5df', '#8f8e87', 0.55, 0.3, False)),
             marblePlaster=mat2('ja_mplaster', stucco('#fbf8ee', '#ffffff', '#ddd5bf')),
             awningBlue=mat2('ja_awn1', stripes('#23395e', '#f2ede0')), awningRed=mat2('ja_awn2', stripes('#8e2a20', '#f2ede0')),
             dirt=mat2('ja_gravel', earth('#b7b2a4', '#d8d4c6')),
             woodDark=mat2('ja_woodDark', planks('#3a2519', '#573a28', 1.2, 0.09)),
             lacq=mat2('ja_lacq', solid('#a4261a', 0.35)))
        def jp_prism(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
            over2 = over + 0.2 * min(1.0, min(sx, sy) / 1.0)
            hx, hy = sx / 2 + over2, sy / 2 + over2
            o = gable_surf(x, y, z, hx, hy, h * 1.05, mat, along, expo=1.55, upturn=0.1, ext=ridge_ext + 0.12, gmat=M['plaster'], name=name)
            fascia(x, y, z, hx - 0.04, hy - 0.04, M['woodDark'])
            L = (hx if along == 'x' else hy) + ridge_ext + 0.12
            if along == 'x':
                box(x, y, z + h * 1.05 - 0.02, 2 * L, 0.1, 0.07, M['black'])
                for s in (-1, 1):
                    cone(x + s * L, y, z + h * 1.05 - 0.01, 0.045, 0.15, M['gold'], verts=5)
            else:
                box(x, y, z + h * 1.05 - 0.02, 0.1, 2 * L, 0.07, M['black'])
                for s in (-1, 1):
                    cone(x, y + s * L, z + h * 1.05 - 0.01, 0.045, 0.15, M['gold'], verts=5)
            return o
        def jp_hip(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
            big = sx > 1.45
            ov = over + 0.2 * min(1.0, sx / 1.0)
            hx, hy = sx / 2 + ov, sy / 2 + ov
            ridge = hx * 0.5 if top < 0.05 else top / 2
            tyy = 0.03 if top < 0.05 else top / 2
            if big:                                                    # lower skirt roof (mokoshi) under the main roof
                hip_surf(x, y, z - 0.34, hx + 0.2, hy + 0.2, 0.34, mat, tx=sx / 2 - 0.02, ty=sy / 2 - 0.02, expo=1.2, lift=0.14, name=name + '_skirt')
                fascia(x, y, z - 0.34, hx + 0.16, hy + 0.16, M['woodDark'])
            o = hip_surf(x, y, z, hx, hy, h * 1.0, mat, tx=ridge, ty=tyy, expo=1.6, lift=0.16 if big else 0.11, name=name)
            fascia(x, y, z, hx - 0.04, hy - 0.04, M['woodDark'])
            if top < 0.05:
                box(x, y, z + h - 0.02, 2 * ridge + 0.1, 0.1, 0.07, M['black'])
                for s in (-1, 1):
                    cone(x + s * (ridge + 0.04), y, z + h - 0.01, 0.045, 0.16, M['gold'], verts=5)
            return o
        def jp_cone(x, y, z, r, h, mat, verts=12, name='cone'):
            if r > 0.2 and h > 0.3 and roofmat(mat):
                lathe(x, y, z, [(r * 1.18, h * 0.1), (r * 1.06, h * 0.04), (r * 0.82, h * 0.12), (r * 0.55, h * 0.34), (r * 0.3, h * 0.68), (r * 0.1, h * 0.93), (0, h)], mat, 8, name=name, rz=0.4)
                cyl(x, y, z + h - 0.02, 0.02, 0.2, M['gold'], verts=5)
                sphere(x, y, z + h * 0.7, 0.025, M['gold'], seg=5)
                cyl(x, y, z - 0.01, r * 1.1, 0.05, M['woodDark'], verts=8)
                return None
            return _cone_orig(x, y, z, r, h, mat, verts, name)
        def jp_window(x, y, face='y', w=0.22, h=0.22):             # shoji screen: pale paper panel in a dark lattice
            if face == 'y':
                box(x, y, 0.0, w + 0.06, 0.04, h + 0.04, M['woodDark'])
                box(x, y - 0.012, 0.01, w, 0.04, h, M['canvas'])
                for i in (-1, 0, 1):
                    box(x + i * w / 3, y - 0.03, 0.0, 0.02, 0.03, h, M['woodDark'])
                for j in (1, 2):
                    box(x, y - 0.03, h * j / 3, w, 0.03, 0.02, M['woodDark'])
            else:
                box(x, y, 0.0, 0.04, w + 0.06, h + 0.04, M['woodDark'])
                box(x + 0.012, y, 0.01, 0.04, w, h, M['canvas'])
                for i in (-1, 0, 1):
                    box(x + 0.03, y + i * w / 3, 0.0, 0.03, 0.02, h, M['woodDark'])
                for j in (1, 2):
                    box(x + 0.03, y, h * j / 3, 0.03, w, 0.02, M['woodDark'])
        def jp_door(x, y, face='x', w=0.26, h=0.5, mat=None):
            if face == 'x':
                box(x, y, 0, 0.05, w + 0.1, h + 0.06, M['woodDark'])
                for s in (-1, 1):
                    box(x + 0.02, y + s * w / 4, 0.0, 0.04, w / 2 - 0.02, h, M['canvas'])
                for j in (1, 2, 3):
                    box(x + 0.045, y, h * j / 4, 0.02, w + 0.04, 0.02, M['woodDark'])
                box(x + 0.045, y, 0, 0.02, 0.025, h, M['woodDark'])
                box(x + 0.12, y, 0, 0.2, w + 0.22, 0.05, M['stoneDark'])
            else:
                box(x, y, 0, w + 0.1, 0.05, h + 0.06, M['woodDark'])
                for s in (-1, 1):
                    box(x + s * w / 4, y - 0.02, 0.0, w / 2 - 0.02, 0.04, h, M['canvas'])
                for j in (1, 2, 3):
                    box(x, y - 0.045, h * j / 4, w + 0.04, 0.02, 0.02, M['woodDark'])
                box(x, y - 0.12, 0, w + 0.22, 0.2, 0.05, M['stoneDark'])
        def jp_beams(hx, hy, z0, z1, mat=None):                    # posts and a lintel only (no diagonal bracing)
            mat = M['woodDark']
            t = 0.1
            for sx in (-hx, hx):
                for sy in (-hy, hy):
                    box(sx, sy, z0, t, t, z1 - z0, mat)
            for face in ('y', 'x'):
                span = 2 * (hx if face == 'y' else hy)
                n = max(1, int(span / 0.5))
                for i in range(1, n):
                    p = -span / 2 + i * span / n
                    if face == 'y':
                        box(p, -hy, z0, 0.06, 0.08, z1 - z0, mat)
                    else:
                        box(hx, p, z0, 0.08, 0.06, z1 - z0, mat)
            box(0, -hy, z1 - 0.08, 2 * hx + t, 0.09, 0.09, mat); box(hx, 0, z1 - 0.08, 0.09, 2 * hy + t, 0.09, mat)
            box(0, -hy, z0 + 0.1, 2 * hx, 0.07, 0.04, mat); box(hx, 0, z0 + 0.1, 0.07, 2 * hy, 0.04, mat)
        def jp_flag(x, y, z, h, team, w=0.42, ht=0.26, name='flag'):       # nobori: tall narrow banner on a cross-arm
            cyl(x, y, z, 0.02, h, M['woodDark'], verts=6, name=name + '_pole')
            box(x + 0.08, y, z + h - 0.02, 0.18, 0.02, 0.02, M['woodDark']).rotation_euler = (0, 0, math.radians(-35))
            cloth = box(x + 0.1, y - 0.04, z + h * 0.35, 0.02, 0.2, h * 0.62, team_mat(team))
            cloth.rotation_euler = (0, 0, math.radians(-35))
            box(x + 0.1, y - 0.04, z + h * 0.35, 0.025, 0.02, h * 0.62, M['white']).rotation_euler = (0, 0, math.radians(-35))
        def jp_chimney(x, y, z, h=0.7, w=0.2):
            pass                                                    # no chimneys: smoke leaves through the roof
        def jp_cren(x, y, z, sx, sy, mat, n=6, hgt=0.12):          # no merlons; a white plaster parapet band with a tile cap
            hx, hy = sx / 2, sy / 2
            box(x, y - hy + 0.04, z, sx, 0.08, 0.1, M['plaster']); box(x, y + hy - 0.04, z, sx, 0.08, 0.1, M['plaster'])
            box(x - hx + 0.04, y, z, 0.08, sy, 0.1, M['plaster']); box(x + hx - 0.04, y, z, 0.08, sy, 0.1, M['plaster'])
            box(x, y - hy + 0.04, z + 0.1, sx + 0.04, 0.12, 0.04, M['black']); box(x, y + hy - 0.04, z + 0.1, sx + 0.04, 0.12, 0.04, M['black'])
            box(x - hx + 0.04, y, z + 0.1, 0.12, sy + 0.04, 0.04, M['black']); box(x + hx - 0.04, y, z + 0.1, 0.12, sy + 0.04, 0.04, M['black'])
        props += [lambda px, py, r, k: stone_lantern(px, py, r.uniform(0.9, 1.3)), lambda px, py, r, k: tree_pine_bonsai(px, py, r.uniform(1.0, 1.5)),
                  lambda px, py, r, k: bamboo(px, py, r.uniform(0.9, 1.3), k), lambda px, py, r, k: stone_lantern(px, py, 1.0),
                  lambda px, py, r, k: tree_pine_bonsai(px, py, 1.2)]
        prism_roof, hip_roof, cone, window, door, beams, chimney, crenellations, flag = jp_prism, jp_hip, jp_cone, jp_window, jp_door, jp_beams, jp_chimney, jp_cren, jp_flag

        def m_house_civ(style):
            sx, sy = 1.24, 1.06
            base = 0.26
            box(0, 0.05, 0, sx + 0.24, sy + 0.24, 0.1, M['stoneDark'])                                  # stone footing
            for ix in (-1, 0, 1):
                for iy in (-1, 1):
                    cyl(ix * sx / 2.2, 0.05 + iy * (sy / 2 + 0.02), 0.1, 0.045, base - 0.1, M['woodDark'], verts=6)
            box(0, 0.05, base, sx + 0.08, sy + 0.08, 0.06, M['woodLight'])                              # raised floor
            box(0, 0.05, base + 0.06, sx, sy, 0.5, M['plaster'])
            beams(sx / 2 - 0.02, sy / 2 - 0.02, base + 0.06, base + 0.56, M['woodDark'])
            # engawa veranda with rail on the +X face
            box(sx / 2 + 0.22, 0.05, base, 0.4, sy + 0.1, 0.05, M['woodLight'])
            for t in (-0.45, 0.0, 0.45):
                cyl(sx / 2 + 0.4, 0.05 + t, base, 0.03, 0.6, M['woodDark'], verts=6)
            box(sx / 2 + 0.4, 0.05, base + 0.16, 0.03, sy, 0.03, M['woodDark'])
            prism_roof(0, 0.05, base + 0.56, sx, sy, 0.6 + 0.08 * style, _roof(style), over=0.3, along='y')
            if style >= 2:                                                                           # upper roof tier / gold ridge ornaments
                hip_surf(0, 0.05, base + 0.56 - 0.12, sx / 2 + 0.34, sy / 2 + 0.34, 0.2, _roof(style), tx=sx / 2, ty=sy / 2, expo=1.3, lift=0.08)
            door(sx / 2 + 0.01, 0.05, 'x', 0.34, 0.46)
            window(-0.2, -sy / 2 - 0.03 + 0.05, 'y', 0.3, 0.26)
            window(sx / 2 + 0.01, -0.35, 'x', 0.2, 0.24)
            stone_lantern(1.05, -0.8, 1.0)
            if style >= 1:
                tree_pine_bonsai(-0.85, -0.72, 1.1)
        m_house = m_house_civ
        C['pre'] = lambda name: None

    elif code == 'fr':
        # ===================================================== Franks: limestone, half timber, steep blue slate, spires
        mats(plaster=mat2('fr_plaster', stucco('#e7ddc0', '#f6efd6', '#bcae88')),
             stone=mat2('fr_stone', masonry('#cfc8b2', '#e6dfc9', '#97907a', 0.46, 0.23)),
             stoneDark=mat2('fr_stoneDark', masonry('#a29a84', '#c0b9a3', '#6e6858', 0.5, 0.25)),
             thatch=mat2('fr_thatch', thatch('#a58a42', '#d0b566', '#5d4a22')),
             tile=mat2('fr_tile', shingles('#3b5486', '#5873ac', '#1e2d4e', 0.12, 0.085)),
             slate=mat2('fr_slate', shingles('#2f4373', '#4a64a0', '#192541', 0.12, 0.085)),
             imperial=mat2('fr_imp', shingles('#1b2f86', '#3453c4', '#0e1850', 0.12, 0.085)),
             marble=mat2('fr_marble', masonry('#e0dccd', '#f4f1e6', '#aca691', 0.46, 0.23, False)),
             marblePlaster=mat2('fr_mplaster', stucco('#f1ecd9', '#fdfaf0', '#d2c9ac')),
             awningBlue=mat2('fr_awn1', stripes('#2a4a96', '#f2ede0')), awningRed=mat2('fr_awn2', stripes('#2a4a96', '#e8c850')),
             dirt=mat2('fr_dirt', earth('#657a3e', '#85995a')))
        def fr_prism(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
            hh = h * 1.28
            hx, hy = sx / 2 + over * 0.7, sy / 2 + over * 0.7
            o = gable_surf(x, y, z, hx, hy, hh, mat, along, expo=1.0, upturn=0.0, ext=ridge_ext, gmat=M['plaster'], name=name)
            fascia(x, y, z, hx - 0.02, hy - 0.02, M['woodDark'])
            L = (hx if along == 'x' else hy) + ridge_ext
            n = max(3, int(2 * L / 0.35))
            for i in range(n):                                                                      # gold fleur crest along the ridge
                t = -L + (i + 0.5) * 2 * L / n
                px, py = (x + t, y) if along == 'x' else (x, y + t)
                cone(px, py, z + hh - 0.01, 0.03, 0.09, M['gold'], verts=4)
            if along == 'x':
                box(x, y, z + hh - 0.02, 2 * L, 0.07, 0.06, M['stoneDark'])
            else:
                box(x, y, z + hh - 0.02, 0.07, 2 * L, 0.06, M['stoneDark'])
            return o
        def fr_hip(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
            hx, hy = sx / 2 + over * 0.6, sy / 2 + over * 0.6
            o = hip_surf(x, y, z, hx, hy, h * 1.3, mat, tx=top / 2, ty=top / 2, expo=1.0, lift=0.0, name=name)
            fascia(x, y, z, hx - 0.02, hy - 0.02, M['woodDark'])
            if top < 0.05:
                cyl(x, y, z + h * 1.3 - 0.02, 0.018, 0.22, M['gold'], verts=5)
                sphere(x, y, z + h * 1.3 + 0.14, 0.04, M['gold'], seg=6)
            return o
        def fr_cone(x, y, z, r, h, mat, verts=12, name='cone'):
            if r > 0.2 and h > 0.3 and roofmat(mat):
                spire(x, y, z, r * 1.12, h * 1.75, mat, verts=10)
                cyl(x, y, z - 0.01, r * 1.14, 0.05, M['woodDark'], verts=10)
                for a in range(4):                                                                  # little corner pinnacles
                    aa = a * math.pi / 2 + math.pi / 4
                    cone(x + math.cos(aa) * r * 0.9, y + math.sin(aa) * r * 0.9, z, 0.035, h * 0.55, mat, verts=5)
                return None
            return _cone_orig(x, y, z, r, h, mat, verts, name)
        def fr_window(x, y, face='y', w=0.22, h=0.22):
            hh = max(h, 0.3)
            arch_opening(x, y, face, w * 0.8, hh, M['black'], 'point', 0.05, M['stoneDark'])
            if face == 'y':
                box(x, y - 0.03, hh * 0.45, w * 0.8, 0.02, 0.025, M['stoneDark'])
            else:
                box(x + 0.03, y, hh * 0.45, 0.02, w * 0.8, 0.025, M['stoneDark'])
        def fr_door(x, y, face='x', w=0.26, h=0.5, mat=None):
            arch_opening(x, y, face, w, h + 0.1, mat or M['woodDark'], 'point', 0.05, M['stoneDark'])
            if face == 'x':
                for z in (h * 0.3, h * 0.65):
                    box(x + 0.03, y, z, 0.02, w * 0.85, 0.025, M['iron'])
                box(x + 0.1, y, 0, 0.16, w + 0.2, 0.05, M['stoneDark'])
            else:
                for z in (h * 0.3, h * 0.65):
                    box(x, y - 0.03, z, w * 0.85, 0.02, 0.025, M['iron'])
                box(x, y - 0.1, 0, w + 0.2, 0.16, 0.05, M['stoneDark'])
        def fr_crenel(x, y, z, sx, sy, mat, n=6, hgt=0.12):
            _cren_orig(x, y, z, sx, sy, mat, n, hgt)
            for sx_ in (-1, 1):
                for sy_ in (-1, 1):
                    box(x + sx_ * sx / 2, y + sy_ * sy / 2, z, 0.1, 0.1, hgt * 1.8, mat)
        props += [lambda px, py, r, k: shield_stand(px, py), lambda px, py, r, k: tree_cypress(px, py, r.uniform(0.9, 1.3)),
                  lambda px, py, r, k: bush(px, py, 1.1, k, True), lambda px, py, r, k: haybale(px, py, 0, r.uniform(0, 3))]
        prism_roof, hip_roof, cone, window, door, crenellations = fr_prism, fr_hip, fr_cone, fr_window, fr_door, fr_crenel

        def m_house_civ(style):
            sx, sy = 1.18, 1.02
            box(0, 0.05, 0, sx + 0.1, sy + 0.1, 0.3, M['stone'])
            box(0, 0.05, 0.3, sx, sy, 0.42, M['plaster'])
            beams(sx / 2 - 0.02, sy / 2 - 0.02, 0.3, 0.72, M['woodDark'])
            box(0, 0.05, 0.72, sx + 0.14, sy + 0.14, 0.06, M['woodDark'])                          # jetty: the upper storey overhangs
            box(0, 0.05, 0.78, sx + 0.1, sy + 0.1, 0.36, M['plaster'])
            beams(sx / 2 + 0.03, sy / 2 + 0.03, 0.78, 1.14, M['woodDark'])
            prism_roof(0, 0.05, 1.14, sx + 0.1, sy + 0.1, 0.62, _roof(style), over=0.16, along='y')
            box(-0.42, 0.4, 0.9, 0.2, 0.2, 0.62 + 0.12 * style, M['stone'])
            box(-0.42, 0.4, 1.52 + 0.12 * style, 0.26, 0.26, 0.06, M['stoneDark'])
            door(sx / 2 + 0.01, 0.12, 'x', 0.3, 0.44)
            awning(sx / 2 + 0.02, 0.12, 0.58, 0.5, 0.3, 'x', M['awningBlue'])
            window(sx / 2 + 0.06, -0.3, 'x', 0.2, 0.26)
            window(-0.2, -sy / 2 - 0.0, 'y', 0.22, 0.26)
            window(0.1, -sy / 2 - 0.07, 'y', 0.2, 0.22) if False else None
            if style >= 2:                                                                          # corner turret with a spire
                cyl(sx / 2 - 0.02, -sy / 2 + 0.0, 0.5, 0.17, 0.9, M['stone'], verts=10, r2=0.16)
                cone(sx / 2 - 0.02, -sy / 2 + 0.0, 1.4, 0.2, 0.4, _roof(style), verts=10)
            sack(0.95, 0.5)
            barrel(0.88, -0.55)
        m_house = m_house_civ
        C['pre'] = lambda name: None

    elif code == 'ce':
        # ===================================================== Celts: drystone, wattle-and-daub, deep thatch, turf, round forms
        mats(plaster=mat2('ce_plaster', stucco('#d8cfb2', '#ebe2c6', '#a09270')),
             stone=mat2('ce_stone', masonry('#7e7f72', '#a1a290', '#47483c', 0.55, 0.3)),
             stoneDark=mat2('ce_stoneDark', masonry('#5e6054', '#7a7c6c', '#36372d', 0.5, 0.28)),
             thatch=mat2('ce_thatch', thatch('#8d7539', '#c0a456', '#4c3d1b')),
             tile=mat2('ce_tile', thatch('#6f5c2c', '#9d8643', '#3a2f12')),
             slate=mat2('ce_slate', earth('#4b6330', '#6b8446')),
             imperial=mat2('ce_imp', shingles('#4a5a68', '#6c7f90', '#27313a', 0.16, 0.1)),
             marble=mat2('ce_marble', masonry('#aeb0a4', '#cfd1c4', '#6f7164', 0.55, 0.3, False)),
             marblePlaster=mat2('ce_mplaster', stucco('#e8e2cc', '#f6f0dc', '#c0b48e')),
             awningBlue=mat2('ce_awn1', stripes('#2f6a3a', '#e8dcb8')), awningRed=mat2('ce_awn2', stripes('#a8421a', '#e8dcb8')),
             dirt=mat2('ce_dirt', earth('#4c5030', '#6c7242')))
        def ce_prism(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
            hx, hy = sx / 2 + over + 0.14, sy / 2 + over + 0.14
            hh = h * 1.08
            o = gable_surf(x, y, z - 0.04, hx, hy, hh, mat, along, expo=0.82, upturn=-0.04, ext=ridge_ext + 0.08, gmat=M['plaster'], name=name)
            L = (hx if along == 'x' else hy) + ridge_ext + 0.08
            if along == 'x':
                cyl(x - L, y, z - 0.04 + hh - 0.04, 0.06, 2 * L, mat, verts=8, ry=math.pi / 2)
            else:
                cyl(x, y - L, z - 0.04 + hh - 0.04, 0.06, 2 * L, mat, verts=8, rx=-math.pi / 2).rotation_euler = (-math.pi / 2, 0, 0)
            return o
        def ce_hip(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
            hx, hy = sx / 2 + over + 0.16, sy / 2 + over + 0.16
            o = hip_surf(x, y, z - 0.05, hx, hy, h * 1.1, mat, tx=top / 2, ty=top / 2, expo=0.85, lift=-0.03, name=name)
            if top < 0.05:
                cyl(x, y, z - 0.05 + h * 1.1 - 0.05, 0.05, 0.14, M['woodDark'], verts=6)
                cone(x, y, z - 0.05 + h * 1.1 + 0.07, 0.07, 0.1, mat, verts=6)
            return o
        def ce_cone(x, y, z, r, h, mat, verts=12, name='cone'):
            if r > 0.2 and h > 0.3 and roofmat(mat):
                lathe(x, y, z - 0.03, scaled([(1.16, 0), (1.04, 0.1), (0.74, 0.42), (0.42, 0.76), (0.14, 0.96), (0, 1.0)], r, h * 0.95), mat, 12, name=name)
                cyl(x, y, z - 0.02, r * 1.14, 0.045, M['woodDark'], verts=12)
                cyl(x, y, z + h * 0.35, r * 0.78, 0.035, M['woodDark'], verts=12)
                cyl(x, y, z + h * 0.8, r * 0.3, 0.12, M['woodDark'], verts=6)
                return None
            return _cone_orig(x, y, z, r, h, mat, verts, name)
        def ce_window(x, y, face='y', w=0.22, h=0.22):
            arch_opening(x, y, face, w * 0.65, h, M['black'], 'round', 0.05, M['stoneDark'])
        def ce_door(x, y, face='x', w=0.26, h=0.5, mat=None):
            arch_opening(x, y, face, w, h, mat or M['woodDark'], 'round', 0.05, M['stoneDark'])
            if face == 'x':
                box(x + 0.03, y, h * 0.5, 0.02, w * 0.9, 0.03, M['iron']); box(x + 0.12, y, 0, 0.18, w + 0.18, 0.05, M['stoneDark'])
            else:
                box(x, y - 0.03, h * 0.5, w * 0.9, 0.02, 0.03, M['iron']); box(x, y - 0.12, 0, w + 0.18, 0.18, 0.05, M['stoneDark'])
        def ce_beams(hx, hy, z0, z1, mat=None):                    # wattle: corner posts + a few staves
            mat = M['woodDark']
            for sx in (-hx, hx):
                for sy in (-hy, hy):
                    box(sx, sy, z0, 0.1, 0.1, z1 - z0, mat)
            box(0, -hy, z1 - 0.06, 2 * hx + 0.1, 0.08, 0.08, mat); box(hx, 0, z1 - 0.06, 0.08, 2 * hy + 0.1, 0.08, mat)
            for face in ('y', 'x'):
                span = 2 * (hx if face == 'y' else hy)
                n = max(2, int(span / 0.34))
                for i in range(1, n):
                    p = -span / 2 + i * span / n
                    if face == 'y':
                        box(p, -hy, z0, 0.035, 0.05, z1 - z0, mat)
                    else:
                        box(hx, p, z0, 0.05, 0.035, z1 - z0, mat)
        def ce_chimney(x, y, z, h=0.7, w=0.2):
            pass                                                    # smoke hole in the thatch instead
        def ce_cren(x, y, z, sx, sy, mat, n=6, hgt=0.12):          # rounded dry-stone coping stones
            hx, hy = sx / 2, sy / 2
            for i in range(n * 2):
                t = -hx + (i + 0.5) * sx / (n * 2)
                for sg in (-1, 1):
                    sphere(x + t, y + sg * (hy - 0.04), z + 0.03, 0.07, M['stone'], sz=0.6, seg=6, noise=0.2, seed=i)
                tt = -hy + (i + 0.5) * sy / (n * 2)
                for sg in (-1, 1):
                    sphere(x + sg * (hx - 0.04), y + tt, z + 0.03, 0.07, M['stone'], sz=0.6, seg=6, noise=0.2, seed=i + 30)
        props += [lambda px, py, r, k: standing_stone(px, py, r.uniform(0.4, 0.65), k), lambda px, py, r, k: high_cross(px, py),
                  lambda px, py, r, k: cauldron(px, py), lambda px, py, r, k: heather(px, py, r.uniform(0.9, 1.4), k),
                  lambda px, py, r, k: heather(px, py, 1.2, k + 3), lambda px, py, r, k: standing_stone(px, py, 0.5, k + 1)]
        prism_roof, hip_roof, cone, window, door, beams, chimney, crenellations = ce_prism, ce_hip, ce_cone, ce_window, ce_door, ce_beams, ce_chimney, ce_cren

        def m_house_civ(style):          # roundhouse: drystone drum, deep conical thatch, smoke hole
            r = 0.72
            lathe(0, 0, 0, [(r + 0.06, 0), (r + 0.05, 0.14), (r, 0.16), (r, 0.58), (r - 0.02, 0.6)], M['stone'], 16)
            lathe(0, 0, 0.14, [(r - 0.01, 0), (r - 0.01, 0.46)], M['plaster'], 16)
            for a in range(8):                                                                       # upright timbers
                aa = a * math.pi / 4 + 0.2
                box(math.cos(aa) * (r - 0.01), math.sin(aa) * (r - 0.01), 0.14, 0.07, 0.07, 0.48, M['woodDark'], rz=aa)
            lathe(0, 0, 0.58, scaled([(1.32, 0), (1.18, 0.1), (0.84, 0.42), (0.46, 0.76), (0.15, 0.96), (0.0, 1.0)], r, 0.95 + 0.06 * style), _roof(style), 16)
            cyl(0, 0, 0.56, r * 1.32, 0.05, M['woodDark'], verts=16)
            cyl(0, 0, 0.58 + (0.95 + 0.06 * style) * 0.9, 0.05, 0.16, M['woodDark'], verts=6)
            door(r - 0.02, 0.0, 'x', 0.3, 0.44)
            box(r + 0.2, 0, 0, 0.3, 0.5, 0.04, M['stoneDark'])
            if style >= 1:                                                                           # attached lean-to hut
                lathe(0.2, -0.95, 0, [(0.4, 0), (0.4, 0.34)], M['stone'], 12)
                lathe(0.2, -0.95, 0.34, scaled([(1.3, 0), (0.95, 0.4), (0.4, 0.8), (0, 1.0)], 0.4, 0.4), _roof(style), 12)
            standing_stone(-0.95, -0.2, 0.45, 2)
            heather(0.9, 0.85, 1.1, 1)
        m_house = m_house_civ
        C['pre'] = lambda name: None

    elif code == 'sp':
        # ===================================================== Spanish: whitewash, sandstone, low terracotta roofs, arches, domes
        mats(plaster=mat2('sp_plaster', stucco('#f6efdf', '#fffaf0', '#d6c8a4')),
             stone=mat2('sp_stone', masonry('#d4b886', '#ecd4a2', '#a88c5c', 0.5, 0.25)),
             stoneDark=mat2('sp_stoneDark', masonry('#b09462', '#cdb27c', '#7e6840', 0.5, 0.25)),
             thatch=mat2('sp_thatch', shingles('#b86438', '#d98250', '#7a3a1c', 0.17, 0.1)),
             tile=mat2('sp_tile', shingles('#c4562c', '#e87a46', '#8a3516', 0.17, 0.1)),
             slate=mat2('sp_slate', shingles('#a8401f', '#d0603a', '#6a2610', 0.17, 0.1)),
             imperial=mat2('sp_imp', shingles('#2b5aa8', '#4a82d6', '#163468', 0.17, 0.1)),
             marble=mat2('sp_marble', masonry('#efe7d4', '#fbf5e6', '#bfb497', 0.5, 0.25, False)),
             marblePlaster=mat2('sp_mplaster', stucco('#fffaf0', '#ffffff', '#e3d8bc')),
             awningBlue=mat2('sp_awn1', stripes('#c0392b', '#f1c40f')), awningRed=mat2('sp_awn2', stripes('#c0392b', '#f4ecd8')),
             dirt=mat2('sp_dirt', earth('#c7a972', '#e4cc9c')),
             azulejo=mat2('sp_azulejo', stripes('#1f4f9c', '#f4f0e6', 0.07)))
        def sp_prism(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
            hx, hy = sx / 2 + over + 0.12, sy / 2 + over + 0.12
            hh = h * 0.62
            o = gable_surf(x, y, z, hx, hy, hh, mat, along, expo=1.0, upturn=0.0, ext=ridge_ext, gmat=M['plaster'], name=name)
            L = (hx if along == 'x' else hy) + ridge_ext
            n = max(3, int(2 * L / 0.22))
            for i in range(n):                                                                       # ridge tiles
                t = -L + (i + 0.5) * 2 * L / n
                px, py = (x + t, y) if along == 'x' else (x, y + t)
                cyl(px, py, z + hh - 0.02, 0.04, 0.12, mat, verts=6, r2=0.04).rotation_euler = (0, math.pi / 2, 0) if along == 'x' else (math.pi / 2, 0, 0)
            fascia(x, y, z, hx - 0.02, hy - 0.02, M['plaster'], 0.06)
            return o
        def sp_hip(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
            hx, hy = sx / 2 + over + 0.1, sy / 2 + over + 0.1
            o = hip_surf(x, y, z, hx, hy, h * 0.62, mat, tx=top / 2, ty=top / 2, expo=1.0, lift=0.0, name=name)
            fascia(x, y, z, hx - 0.02, hy - 0.02, M['plaster'], 0.06)
            if top < 0.05:
                sphere(x, y, z + h * 0.62, 0.05, M['gold'], seg=6)
            return o
        def sp_cone(x, y, z, r, h, mat, verts=12, name='cone'):
            if r > 0.2 and h > 0.3 and roofmat(mat):
                lathe(x, y, z, scaled([(1.06, 0), (1.04, 0.1), (0.96, 0.3), (0.8, 0.52), (0.58, 0.72), (0.3, 0.9), (0.12, 0.97), (0, 1.0)], r, h * 0.7), mat, 14, name=name)
                cyl(x, y, z + h * 0.68, 0.045, 0.1, M['plaster'], verts=8)
                sphere(x, y, z + h * 0.7 + 0.12, 0.045, M['gold'], seg=6)
                cyl(x, y, z - 0.01, r * 1.08, 0.05, M['plaster'], verts=14)
                return None
            return _cone_orig(x, y, z, r, h, mat, verts, name)
        def sp_window(x, y, face='y', w=0.22, h=0.22):
            hh = max(h, 0.26)
            arch_opening(x, y, face, w * 0.8, hh, M['black'], 'round', 0.05, M['stone'])
            if face == 'y':                                                                          # iron grille + flower pot
                for i in (-1, 0, 1):
                    box(x + i * w * 0.25, y - 0.035, 0.02, 0.015, 0.015, hh - 0.08, M['iron'])
                pot(x - w * 0.5, y - 0.1, 0.8)
            else:
                for i in (-1, 0, 1):
                    box(x + 0.035, y + i * w * 0.25, 0.02, 0.015, 0.015, hh - 0.08, M['iron'])
                pot(x + 0.1, y + w * 0.5, 0.8)
        def sp_door(x, y, face='x', w=0.26, h=0.5, mat=None):
            arch_opening(x, y, face, w + 0.04, h + 0.1, mat or M['woodDark'], 'round', 0.05, M['stone'])
            if face == 'x':
                for z in (h * 0.3, h * 0.62):
                    box(x + 0.03, y, z, 0.02, w * 0.85, 0.022, M['iron'])
                box(x + 0.12, y, 0, 0.18, w + 0.22, 0.05, M['stoneDark'])
            else:
                for z in (h * 0.3, h * 0.62):
                    box(x, y - 0.03, z, w * 0.85, 0.02, 0.022, M['iron'])
                box(x, y - 0.12, 0, w + 0.22, 0.18, 0.05, M['stoneDark'])
        def sp_beams(hx, hy, z0, z1, mat=None):                    # plain whitewash with a sandstone corner quoin and a painted band
            for sx in (-hx, hx):
                for sy in (-hy, hy):
                    box(sx, sy, z0, 0.12, 0.12, z1 - z0, M['stone'])
            box(0, -hy, z1 - 0.05, 2 * hx + 0.1, 0.07, 0.06, M['stone']); box(hx, 0, z1 - 0.05, 0.07, 2 * hy + 0.1, 0.06, M['stone'])
            box(0, -hy - 0.005, z0 + 0.06, 2 * hx, 0.04, 0.08, M['azulejo']); box(hx + 0.005, 0, z0 + 0.06, 0.04, 2 * hy, 0.08, M['azulejo'])
        def sp_chimney(x, y, z, h=0.7, w=0.2):
            box(x, y, z, w, w, h, M['plaster']); box(x, y, z + h, w + 0.06, w + 0.06, 0.05, M['stone'])
            cyl(x, y, z + h + 0.05, 0.07, 0.1, M['tile'], verts=8)
        def sp_cren(x, y, z, sx, sy, mat, n=6, hgt=0.12):          # stepped Moorish merlons (pointed tops)
            hx, hy = sx / 2, sy / 2
            for i in range(n):
                t = -hx + (i + 0.5) * sx / n
                for sg in (-1, 1):
                    box(x + t, y + sg * (hy - 0.04), z, sx / n * 0.55, 0.08, hgt, mat); cone(x + t, y + sg * (hy - 0.04), z + hgt, sx / n * 0.3, hgt * 0.8, mat, verts=4)
            for i in range(n):
                t = -hy + (i + 0.5) * sy / n
                for sg in (-1, 1):
                    box(x + sg * (hx - 0.04), y + t, z, 0.08, sy / n * 0.55, hgt, mat); cone(x + sg * (hx - 0.04), y + t, z + hgt, sy / n * 0.3, hgt * 0.8, mat, verts=4)
        props += [lambda px, py, r, k: pot(px, py, r.uniform(1.0, 1.5)), lambda px, py, r, k: jar(px, py, r.uniform(0.8, 1.2)),
                  lambda px, py, r, k: tree_cypress(px, py, r.uniform(1.0, 1.4)), lambda px, py, r, k: pot(px, py, 1.2),
                  lambda px, py, r, k: jar(px, py, 1.0), lambda px, py, r, k: bush(px, py, 1.0, k, True)]
        prism_roof, hip_roof, cone, window, door, beams, chimney, crenellations = sp_prism, sp_hip, sp_cone, sp_window, sp_door, sp_beams, sp_chimney, sp_cren

        def m_house_civ(style):          # casa with arched loggia, balcony and a bell-gable
            sx, sy = 1.34, 1.1
            box(0, 0.05, 0, sx + 0.1, sy + 0.1, 0.2, M['stone'])
            box(0, 0.05, 0.2, sx, sy, 0.62, M['plaster'])
            beams(sx / 2 - 0.02, sy / 2 - 0.02, 0.2, 0.82, None)
            prism_roof(0, 0.05, 0.82, sx, sy, 0.6 + 0.06 * style, _roof(style), over=0.18, along='y')
            door(sx / 2 + 0.01, 0.05, 'x', 0.32, 0.5)
            window(sx / 2 + 0.01, -0.36, 'x', 0.2, 0.26)
            window(-0.2, -sy / 2 + 0.05 - 0.0, 'y', 0.24, 0.28)
            box(sx / 2 + 0.24, 0.05, 0.42, 0.34, 0.8, 0.05, M['woodDark'])                           # balcony with rail
            for t in (-0.34, -0.17, 0.0, 0.17, 0.34):
                box(sx / 2 + 0.4, 0.05 + t, 0.46, 0.02, 0.02, 0.16, M['iron'])
            box(sx / 2 + 0.4, 0.05, 0.62, 0.03, 0.8, 0.03, M['iron'])
            for t in (-0.38, 0.38):
                cyl(sx / 2 + 0.4, 0.05 + t, 0.0, 0.03, 0.42, M['woodDark'], verts=6)
            # bell gable (espadana) on the back wall
            box(-0.3, 0.05, 1.2 + 0.0, 0.42, 0.1, 0.0, M['plaster']) if False else None
            box(-0.28, 0.4, 0.82, 0.34, 0.1, 0.56, M['plaster'])
            arch_opening(-0.28, 0.35, 'y', 0.14, 0.26, M['black'], 'round', 0.05, M['stone']) if False else None
            cone(-0.28, 0.4, 1.38, 0.2, 0.2, M['plaster'], verts=4)
            sphere(-0.28, 0.35, 1.1, 0.04, M['gold'], seg=6)
            pot(-0.78, -0.62, 1.1); pot(0.95, -0.72, 1.0)
            if style >= 2:
                tree_cypress(-0.9, 0.55, 1.1)
        m_house = m_house_civ
        C['pre'] = lambda name: None

    elif code == 'mo':
        # ===================================================== Mongols: felt gers and pavilion tents, painted lattice, tug standards
        mats(plaster=mat2('mo_felt', cloth('#e6dcc2', '#f6eed9', 26, 0.95)),
             stone=mat2('mo_stone', masonry('#8e8168', '#aa9c81', '#5e5540', 0.5, 0.26)),
             stoneDark=mat2('mo_stoneDark', masonry('#6a604c', '#847964', '#453d2f', 0.5, 0.26)),
             thatch=mat2('mo_hide', cloth('#6f4a30', '#93694a', 24, 0.95)),
             tile=mat2('mo_felt2', cloth('#e2d8be', '#f3ebd6', 26, 0.95)),
             slate=mat2('mo_felt3', cloth('#d9cfb2', '#ece3cb', 26, 0.95)),
             imperial=mat2('mo_silk', cloth('#1f3f8c', '#3b66cc', 40, 0.6)),
             marble=mat2('mo_marble', masonry('#cfc3a6', '#e8dcc0', '#9a8e72', 0.5, 0.26, False)),
             marblePlaster=mat2('mo_mfelt', cloth('#f2eada', '#fffaec', 26, 0.9)),
             awningBlue=mat2('mo_awn1', stripes('#2f5aa8', '#f0e8d4')), awningRed=mat2('mo_awn2', stripes('#a4261a', '#f0e8d4')),
             dirt=mat2('mo_dirt', earth('#9c8c52', '#bdab6c')),
             lacq=mat2('mo_lacq', solid('#a4261a', 0.5)), blueP=mat2('mo_bluep', solid('#2a58b0', 0.5)))
        def mo_trim(x, y, z, hx, hy, th=0.07):                                   # painted band around the eaves
            for s in (-1, 1):
                box(x, y + s * hy, z - 0.04, 2 * hx, 0.04, th, M['lacq']); box(x + s * hx, y, z - 0.04, 0.04, 2 * hy, th, M['lacq'])
        def mo_prism(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
            hx, hy = sx / 2 + over + 0.06, sy / 2 + over + 0.06
            hh = h * 1.12
            o = gable_surf(x, y, z, hx, hy, hh, mat, along, expo=0.55, upturn=-0.02, ext=ridge_ext + 0.02, gmat=M['plaster'], name=name)
            mo_trim(x, y, z, hx, hy)
            L = (hx if along == 'x' else hy) + ridge_ext
            if along == 'x':
                box(x, y, z + hh - 0.03, 2 * L, 0.06, 0.04, M['lacq'])
                for s in (-1, 1):
                    sphere(x + s * L, y, z + hh - 0.02, 0.04, M['gold'], seg=6)
            else:
                box(x, y, z + hh - 0.03, 0.06, 2 * L, 0.04, M['lacq'])
                for s in (-1, 1):
                    sphere(x, y + s * L, z + hh - 0.02, 0.04, M['gold'], seg=6)
            return o
        def mo_hip(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
            hx, hy = sx / 2 + over + 0.04, sy / 2 + over + 0.04
            o = hip_surf(x, y, z, hx, hy, h * 1.05, mat, tx=top / 2, ty=top / 2, expo=0.62, lift=0.0, name=name)
            mo_trim(x, y, z, hx, hy)
            if top < 0.05:
                cyl(x, y, z + h * 1.05 - 0.03, 0.025, 0.2, M['gold'], verts=6)
                sphere(x, y, z + h * 1.05 + 0.12, 0.05, M['gold'], seg=6)
            return o
        def mo_cone(x, y, z, r, h, mat, verts=12, name='cone'):
            if r > 0.2 and h > 0.3 and roofmat(mat):
                lathe(x, y, z, scaled([(1.06, 0), (1.0, 0.22), (0.84, 0.5), (0.54, 0.78), (0.22, 0.95), (0, 1.0)], r, h * 0.9), mat, 14, name=name)
                cyl(x, y, z - 0.01, r * 1.07, 0.06, M['lacq'], verts=14)
                cyl(x, y, z + h * 0.86, 0.02, 0.2, M['gold'], verts=5)
                sphere(x, y, z + h * 0.86 + 0.2, 0.04, M['gold'], seg=6)
                return None
            return _cone_orig(x, y, z, r, h, mat, verts, name)
        def mo_window(x, y, face='y', w=0.22, h=0.22):            # no windows in a ger: a painted lattice panel instead
            if face == 'y':
                box(x, y, 0.0, w, 0.04, h, M['lacq'])
                for i in range(3):
                    box(x - w / 3 + i * w / 3, y - 0.02, 0.02, 0.025, 0.02, h - 0.04, M['gold'])
            else:
                box(x, y, 0.0, 0.04, w, h, M['lacq'])
                for i in range(3):
                    box(x + 0.02, y - w / 3 + i * w / 3, 0.02, 0.02, 0.025, h - 0.04, M['gold'])
        def mo_door(x, y, face='x', w=0.26, h=0.5, mat=None):     # low painted red door with a blue frame and gold studs
            if face == 'x':
                box(x, y, 0, 0.05, w + 0.1, h + 0.06, M['blueP'])
                box(x + 0.02, y, 0, 0.05, w, h, M['lacq'])
                for i in (-1, 0, 1):
                    sphere(x + 0.05, y + i * w * 0.28, h * 0.55, 0.018, M['gold'], seg=5)
                box(x + 0.1, y, 0, 0.16, w + 0.2, 0.05, M['stoneDark'])
            else:
                box(x, y, 0, w + 0.1, 0.05, h + 0.06, M['blueP'])
                box(x, y - 0.02, 0, w, 0.05, h, M['lacq'])
                for i in (-1, 0, 1):
                    sphere(x + i * w * 0.28, y - 0.05, h * 0.55, 0.018, M['gold'], seg=5)
                box(x, y - 0.1, 0, w + 0.2, 0.16, 0.05, M['stoneDark'])
        def mo_beams(hx, hy, z0, z1, mat=None):                    # khana lattice: crossed slats on the walls, painted posts
            for sx in (-hx, hx):
                for sy in (-hy, hy):
                    box(sx, sy, z0, 0.1, 0.1, z1 - z0, M['lacq'])
            box(0, -hy, z1 - 0.06, 2 * hx + 0.1, 0.08, 0.08, M['lacq']); box(hx, 0, z1 - 0.06, 0.08, 2 * hy + 0.1, 0.08, M['lacq'])
            for face in ('y', 'x'):
                span = 2 * (hx if face == 'y' else hy)
                n = max(2, int(span / 0.3))
                for i in range(n):
                    p = -span / 2 + (i + 0.5) * span / n
                    ang = math.atan2(z1 - z0, span / n * 1.6)
                    ln = math.hypot(z1 - z0, span / n * 1.6) * 0.9
                    for sg in (-1, 1):
                        if face == 'y':
                            o = box(p, -hy - 0.01, (z0 + z1) / 2 - ln * 0.02, 0.025, 0.025, ln * 0.0 + 0.0, M['woodDark']) if False else box(p, -hy - 0.01, (z0 + z1) / 2, ln, 0.025, 0.03, M['woodDark'])
                            o.rotation_euler = (0, sg * ang, 0)
                        else:
                            o = box(hx + 0.01, p, (z0 + z1) / 2, 0.025, ln, 0.03, M['woodDark'])
                            o.rotation_euler = (-sg * ang, 0, 0)
        def mo_chimney(x, y, z, h=0.7, w=0.2):                     # iron stovepipe with a cap
            cyl(x, y, z, 0.045, h * 1.1, M['iron'], verts=8)
            cyl(x, y, z + h * 1.1, 0.075, 0.04, M['iron'], verts=8)
        def mo_cren(x, y, z, sx, sy, mat, n=6, hgt=0.12):          # pole-and-skull palisade top: tall spikes
            hx, hy = sx / 2, sy / 2
            for i in range(n * 2):
                t = -hx + (i + 0.5) * sx / (n * 2)
                for sg in (-1, 1):
                    cone(x + t, y + sg * (hy - 0.04), z, 0.035, hgt * 2.2, M['woodDark'], verts=5)
                tt = -hy + (i + 0.5) * sy / (n * 2)
                for sg in (-1, 1):
                    cone(x + sg * (hx - 0.04), y + tt, z, 0.035, hgt * 2.2, M['woodDark'], verts=5)
        def mo_flag(x, y, z, h, team, w=0.42, ht=0.26, name='flag'):
            tug(x, y, h * 1.0, team)
        props += [lambda px, py, r, k: horse_post(px, py, r.choice((0, 1.57))), lambda px, py, r, k: tug(px, py, r.uniform(0.7, 1.0)),
                  lambda px, py, r, k: felt_rolls(px, py), lambda px, py, r, k: haystack(px, py, 0.9),
                  lambda px, py, r, k: horse(px, py, 0, r.uniform(0, 6.28), 0.55, M['hide'], 0.0) and None]
        prism_roof, hip_roof, cone, window, door, beams, chimney, crenellations, flag = mo_prism, mo_hip, mo_cone, mo_window, mo_door, mo_beams, mo_chimney, mo_cren, mo_flag

        def m_house_civ(style):          # ger: lattice drum, domed felt roof, crown ring, red door
            r = 0.74
            lathe(0, 0, 0, [(r + 0.06, 0), (r + 0.06, 0.1), (r, 0.12)], M['stoneDark'], 16)
            lathe(0, 0, 0.1, [(r, 0), (r, 0.5)], M['plaster'], 16)
            for a in range(12):                                                                       # lattice poles + painted band
                aa = a * math.pi / 6
                box(math.cos(aa) * r, math.sin(aa) * r, 0.1, 0.045, 0.045, 0.5, M['woodDark'], rz=aa)
            cyl(0, 0, 0.5, r + 0.02, 0.07, M['lacq'], verts=16)
            lathe(0, 0, 0.58, scaled([(1.12, 0), (1.04, 0.14), (0.86, 0.4), (0.55, 0.7), (0.2, 0.92), (0, 1.0)], r, 0.8 + 0.05 * style), _roof(style), 16)
            cyl(0, 0, 0.56, r * 1.12, 0.05, M['lacq'], verts=16)
            for k in range(8):                                                                        # rope bands over the roof
                aa = k * math.pi / 4
            cyl(0, 0, 0.58 + (0.8 + 0.05 * style) * 0.93, 0.18, 0.05, M['woodDark'], verts=10)
            cyl(0, 0, 0.58 + (0.8 + 0.05 * style) * 0.98, 0.025, 0.2, M['gold'], verts=5)
            sphere(0, 0, 0.58 + (0.8 + 0.05 * style) + 0.22, 0.04, M['gold'], seg=6)
            door(r - 0.01, 0, 'x', 0.28, 0.42)
            cyl(-0.3, 0.2, 0.9, 0.04, 0.5, M['iron'], verts=8)
            if style >= 1:                                                                            # smaller guest ger
                lathe(0.15, -1.0, 0, [(0.4, 0), (0.4, 0.3)], M['plaster'], 12)
                lathe(0.15, -1.0, 0.3, scaled([(1.12, 0), (0.86, 0.45), (0.4, 0.8), (0, 1.0)], 0.4, 0.38), _roof(style), 12)
                cyl(0.15, -1.0, 0.3, 0.45, 0.04, M['lacq'], verts=12)
            tug(1.0, 0.85, 1.0, 'blue')
            horse_post(0.9, -0.55)
        m_house = m_house_civ
        C['pre'] = lambda name: None

    elif code == 'sl':
        # ===================================================== Slavs: log timber, Kremlin red brick, tent roofs, onion domes
        logs = mat2('sl_logs', planks('#9a6a3a', '#c4905a', 2.6, 0.13, 0.5, '#33200f'))
        white = mat2('sl_white', stucco('#efe9dc', '#fbf7ee', '#cdc3aa'))
        mats(plaster=logs,
             stone=mat2('sl_brick', masonry('#a8412e', '#c4593f', '#e4dac4', 0.3, 0.15)),
             stoneDark=mat2('sl_brickD', masonry('#7a3022', '#98452f', '#bdb29a', 0.3, 0.15)),
             thatch=mat2('sl_shakes', shingles('#7b5b3a', '#a47f55', '#3d2a1a', 0.14, 0.2)),
             tile=mat2('sl_tile', shingles('#3b7a5c', '#58a27c', '#234a38', 0.13, 0.1)),
             slate=mat2('sl_slate', shingles('#2d6b78', '#4592a2', '#1a3f48', 0.13, 0.1)),
             imperial=mat2('sl_imp', shingles('#238c78', '#3cb89c', '#115446', 0.13, 0.1)),
             marble=mat2('sl_marble', masonry('#efe8d6', '#fbf6e8', '#c0b69b', 0.46, 0.23, False)),
             marblePlaster=white,
             awningBlue=mat2('sl_awn1', stripes('#2f7a5e', '#f2ede0')), awningRed=mat2('sl_awn2', stripes('#b03a2a', '#f2ede0')),
             dirt=mat2('sl_dirt', earth('#5c4630', '#7e6240')),
             trim=mat2('sl_trim', solid('#f1ece0', 0.7)), paint=mat2('sl_paint', solid('#2c78a8', 0.6)),
             onionA=mat2('sl_onionA', solid('#2a8f8c', 0.28, 0.6)), onionB=mat2('sl_onionB', solid('#c23a2a', 0.3, 0.5)))
        def sl_prism(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
            hx, hy = sx / 2 + over + 0.04, sy / 2 + over + 0.04
            hh = h * 1.22
            o = gable_surf(x, y, z, hx, hy, hh, mat, along, expo=1.0, upturn=0.0, ext=ridge_ext + 0.04, gmat=M['plaster'], name=name)
            L = (hx if along == 'x' else hy) + ridge_ext + 0.04
            span = hy if along == 'x' else hx
            slope_board(x, y, z, span, hh, M['trim'], 'x' if along == 'x' else 'y', 0.08, 0.035, L)    # carved bargeboards
            if along == 'x':
                box(x, y, z + hh - 0.02, 2 * L, 0.1, 0.07, M['woodDark'])
                for s in (-1, 1):
                    sphere(x + s * L, y, z + hh - 0.02, 0.05, M['trim'], seg=6)
            else:
                box(x, y, z + hh - 0.02, 0.1, 2 * L, 0.07, M['woodDark'])
                for s in (-1, 1):
                    sphere(x, y + s * L, z + hh - 0.02, 0.05, M['trim'], seg=6)
            fascia(x, y, z, hx - 0.02, hy - 0.02, M['woodDark'])
            return o
        def sl_hip(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):    # shatyor tent roof
            hx, hy = sx / 2 + over, sy / 2 + over
            o = hip_surf(x, y, z, hx, hy, h * 1.35, mat, tx=top / 2, ty=top / 2, expo=1.45, lift=0.0, name=name)
            fascia(x, y, z, hx - 0.02, hy - 0.02, M['woodDark'])
            if top < 0.05:
                onion(x, y, z + h * 1.35 - 0.02, 0.1, 0.2, M['onionB'] if (int(x * 10) + int(y * 10)) % 2 else M['gold'])
            return o
        def sl_cone(x, y, z, r, h, mat, verts=12, name='cone'):
            if r > 0.2 and h > 0.3 and roofmat(mat):
                if (int(x * 7) + int(y * 5)) % 3 == 0:                                                  # tent roof
                    lathe(x, y, z, scaled([(1.12, 0), (1.0, 0.1), (0.7, 0.4), (0.4, 0.7), (0.15, 0.92), (0, 1.0)], r, h * 1.25), mat, 8, name=name)
                    cyl(x, y, z + h * 1.2, 0.015, 0.2, M['gold'], verts=5)
                else:                                                                                   # onion dome on a drum
                    cyl(x, y, z - 0.01, r * 1.12, 0.1, M['trim'], verts=14)
                    onion(x, y, z + 0.08, r * 1.2, h * 1.25, M['onionA'] if (int(x * 7) + int(y * 5)) % 3 == 1 else M['gold'])
                return None
            return _cone_orig(x, y, z, r, h, mat, verts, name)
        def sl_window(x, y, face='y', w=0.22, h=0.22):             # painted platbands, carved header, shutters
            if face == 'y':
                box(x, y, 0.0, w, 0.05, h, M['black'])
                box(x, y - 0.015, h / 2 - 0.01, w, 0.045, 0.025, M['trim'])
                for s in (-1, 1):
                    box(x + s * (w / 2 + 0.05), y - 0.025, -0.02, 0.07, 0.05, h + 0.06, M['trim'])
                    box(x + s * (w / 2 + 0.12), y - 0.05, 0.0, 0.07, 0.03, h, M['paint']).rotation_euler = (0, 0, s * 0.3)
                box(x, y - 0.03, h + 0.0, w + 0.18, 0.06, 0.07, M['trim'])
                box(x, y - 0.03, h + 0.07, w * 0.5, 0.05, 0.06, M['trim'])
            else:
                box(x, y, 0.0, 0.05, w, h, M['black'])
                box(x + 0.015, y, h / 2 - 0.01, 0.045, w, 0.025, M['trim'])
                for s in (-1, 1):
                    box(x + 0.025, y + s * (w / 2 + 0.05), -0.02, 0.05, 0.07, h + 0.06, M['trim'])
                box(x + 0.03, y, h + 0.0, 0.06, w + 0.18, 0.07, M['trim'])
                box(x + 0.03, y, h + 0.07, 0.05, w * 0.5, 0.06, M['trim'])
        def sl_door(x, y, face='x', w=0.26, h=0.5, mat=None):
            _door_orig(x, y, face, w, h, mat)
            if face == 'x':
                box(x + 0.03, y, h + 0.04, 0.07, w + 0.2, 0.06, M['trim']); box(x + 0.03, y, h + 0.1, 0.06, w * 0.5, 0.05, M['trim'])
            else:
                box(x, y - 0.03, h + 0.04, w + 0.2, 0.07, 0.06, M['trim']); box(x, y - 0.03, h + 0.1, w * 0.5, 0.06, 0.05, M['trim'])
        def sl_beams(hx, hy, z0, z1, mat=None):                    # log-cabin: stacked logs with protruding saddle-notched ends
            n = max(3, int((z1 - z0) / 0.1))
            for i in range(n):
                zz = z0 + 0.05 + i * (z1 - z0 - 0.06) / n
                off = 0.0 if i % 2 == 0 else 0.0
                for sy in (-hy, hy):
                    cyl(-hx - 0.05, sy, zz, 0.055, 2 * hx + 0.1, M['bark'] if False else M['wood'], verts=6, ry=math.pi / 2)
                for sx in (-hx, hx):
                    cyl(sx, -hy - 0.05, zz + 0.05, 0.055, 2 * hy + 0.1, M['wood'], verts=6, rx=-math.pi / 2).rotation_euler = (-math.pi / 2, 0, 0)
            for sx in (-hx, hx):
                for sy in (-hy, hy):
                    box(sx, sy, z0, 0.04, 0.04, 0.0, M['woodDark']) if False else None
        def sl_cren(x, y, z, sx, sy, mat, n=6, hgt=0.12):          # swallowtail (M-shaped) Kremlin merlons
            hx, hy = sx / 2, sy / 2
            for i in range(n):
                t = -hx + (i + 0.5) * sx / n
                if i % 2 == 0:
                    for sg in (-1, 1):
                        box(x + t, y + sg * (hy - 0.04), z, sx / n * 0.8, 0.08, hgt, mat); hip_surf(x + t, y + sg * (hy - 0.04), z + hgt, sx / n * 0.42, 0.06, hgt * 0.6, M['trim'], tx=0.0, ty=0.0, expo=1.0, K=1)
            for i in range(n):
                t = -hy + (i + 0.5) * sy / n
                if i % 2 == 0:
                    for sg in (-1, 1):
                        box(x + sg * (hx - 0.04), y + t, z, 0.08, sy / n * 0.8, hgt, mat); hip_surf(x + sg * (hx - 0.04), y + t, z + hgt, 0.06, sy / n * 0.42, hgt * 0.6, M['trim'], tx=0.0, ty=0.0, expo=1.0, K=1)
        props += [lambda px, py, r, k: haystack(px, py, r.uniform(0.9, 1.2)), lambda px, py, r, k: tree_birch(px, py, r.uniform(0.9, 1.3), k),
                  lambda px, py, r, k: well_sweep(px, py), lambda px, py, r, k: beehive(px, py), lambda px, py, r, k: firewood(px, py),
                  lambda px, py, r, k: tree_birch(px, py, 1.1, k + 2)]
        prism_roof, hip_roof, cone, window, door, beams, crenellations = sl_prism, sl_hip, sl_cone, sl_window, sl_door, sl_beams, sl_cren

        def m_house_civ(style):          # izba: log cabin, steep shingled roof, carved trim, porch and a tent-roofed terem
            sx, sy = 1.24, 1.06
            box(0, 0.05, 0, sx + 0.12, sy + 0.12, 0.26, M['stoneDark'])
            box(0, 0.05, 0.26, sx, sy, 0.56, M['plaster'])
            beams(sx / 2 - 0.02, sy / 2 - 0.02, 0.26, 0.82, None)
            prism_roof(0, 0.05, 0.82, sx, sy, 0.62 + 0.04 * style, _roof(style), over=0.2, along='y')
            door(sx / 2 + 0.01, 0.05, 'x', 0.3, 0.44)
            for t in (-0.2, 0.3):                                                                       # porch posts + small roof
                cyl(sx / 2 + 0.36, 0.05 + t, 0.0, 0.035, 0.6, M['woodDark'], verts=6)
            prism_roof(sx / 2 + 0.2, 0.05, 0.6, 0.46, 0.7, 0.2, _roof(style), over=0.04, along='y')
            window(sx / 2 + 0.01, -0.36, 'x', 0.2, 0.24)
            window(-0.2, -sy / 2 + 0.05, 'y', 0.22, 0.26)
            if style >= 2:                                                                              # terem turret
                box(-0.4, 0.45, 0.82, 0.34, 0.34, 0.5, M['plaster'])
                hip_roof(-0.4, 0.45, 1.32, 0.34, 0.34, 0.34, M['slate'], over=0.04, top=0.0)
            else:
                box(-0.42, 0.36, 1.0, 0.18, 0.18, 0.55, M['stoneDark'])
            barrel(0.96, -0.62); haystack(-0.85, -0.7, 0.9)
        m_house = m_house_civ
        def sl_pre(name):
            pre = name.rsplit('_', 1)[0]
            try:
                st = int(name.rsplit('_', 1)[1])
            except Exception:
                st = 0
            castle = (pre in ('castle', 'keep', 'monastery', 'university', 'siege', 'stable', 'bombard')) or (pre == 'tower' and st >= 2) or (pre == 'townhall' and st >= 4) or st >= 2 and pre in ('blacksmith', 'market', 'barracks', 'archery')
            M['plaster'] = white if (castle or st >= 3) else logs
        C['pre'] = sl_pre

    _prism_base = prism_roof
    round_tower_orig = round_tower
    civ_pre = C.get('pre', lambda name: None)
    # per-civ decorate: ground pad colours come from M['dirt']; props from the civ list
    def civ_decorate(name, fw, fh, seed):
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
        generic = [lambda px, py, r, k: barrel(px, py), lambda px, py, r, k: crate(px, py, 0, r.uniform(0.14, 0.2), r.uniform(0, 1.5)),
                   lambda px, py, r, k: sack(px, py), lambda px, py, r, k: basket(px, py), lambda px, py, r, k: bucket(px, py)]
        pool = props * 2 + generic
        for k, (px, py) in enumerate(spots):
            rnd.choice(pool)(px, py, rnd, k)
    decorate = civ_decorate
    return C
