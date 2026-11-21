# Remodelled hero buildings (exec'd last by gen_assets.py; overrides the first-generation versions).

def _roof(style):
    return M['slate'] if style >= 2 else (M['tile'] if style == 1 else M['thatch'])

def chimney(x, y, z, h=0.7, w=0.2):
    box(x, y, z, w, w, h, M['stone'])
    box(x, y, z + h, w + 0.07, w + 0.07, 0.06, M['stoneDark'])
    for s in (-1, 1):
        cyl(x + s * 0.045, y, z + h + 0.06, 0.035, 0.1, M['stoneDark'], verts=6)

def dormer(x, y, z, w=0.34, face='x', style=1):
    """small gabled dormer window sticking out of a roof slope"""
    if face == 'x':
        box(x, y, z, 0.26, w, 0.3, M['plaster'])
        prism_roof(x, y, z + 0.3, 0.26, w, 0.2, _roof(style), over=0.05, along='y')
        window(x + 0.135, y, 'x', 0.14, 0.17)
    else:
        box(x, y, z, w, 0.26, 0.3, M['plaster'])
        prism_roof(x, y, z + 0.3, w, 0.26, 0.2, _roof(style), over=0.05, along='x')
        window(x, y - 0.135, 'y', 0.14, 0.17)

# ------------------------------------------------------------------ house (2x2): main body, lean-to, awning, dormer, chimney
def m_house(style):
    sx, sy = 1.28, 1.12
    base_h = 0.32
    box(0, 0.05, 0, sx + 0.1, sy + 0.1, base_h, M['stone'])
    box(0, 0.05, base_h, sx, sy, 0.54, M['plaster'])
    beams(sx / 2 - 0.02, sy / 2 - 0.02, base_h, base_h + 0.54, M['woodDark'])
    for dy in (0.05,):
        pass
    prism_roof(0, 0.05, base_h + 0.54, sx, sy, 0.62 if style else 0.7, _roof(style), over=0.2, along='y')
    chimney(-0.42, 0.45, base_h + 0.45, 0.62 + (0.1 if style else 0))
    # lean-to storeroom on the front (-Y) with its own low roof and a stone foundation
    box(0.18, -0.72, 0, 0.7, 0.42, 0.22, M['stone'])
    box(0.18, -0.72, 0.22, 0.64, 0.38, 0.4, M['woodLight'])
    r = _prism_base(0.18, -0.72, 0.62, 0.64, 0.38, 0.16, _roof(style), 0.1, 'x', 'leanto', 0.0)
    window(0.0, -0.93, 'y', 0.2, 0.2)
    door(0.51, -0.72, 'x', 0.2, 0.34)
    # front door on the +X gable wall with a striped awning
    door(sx / 2 + 0.01, 0.1, 'x', 0.3, 0.46)
    awning(sx / 2 + 0.02, 0.1, 0.62, 0.5, 0.34, 'x', M['awningBlue'] if style != 1 else M['awningRed'])
    window(sx / 2 + 0.01, -0.35, 'x', 0.2, 0.24)
    window(-0.25, -sy / 2 - 0.0, 'y', 0.22, 0.24)
    dormer(0.2, 0.05, base_h + 0.54 + 0.12, 0.34, 'x', style) if False else None
    cyl(-0.62, -0.55, 0, 0.02, 0.5, M['woodDark'], verts=5)                          # clothes-line post
    cyl(-0.62, 0.0, 0, 0.02, 0.5, M['woodDark'], verts=5)
    box(-0.62, -0.28, 0.44, 0.012, 0.58, 0.012, M['white'])
    for k, c in enumerate(('blue', 'white', 'red')):
        box(-0.62, -0.5 + k * 0.2, 0.28, 0.01, 0.1, 0.16, M[c])

# ------------------------------------------------------------------ farm (2x2): furrows, crop clumps, fence, scarecrow
def m_farm(style):
    box(0, 0, 0, 1.94, 1.94, 0.05, M['soil'])
    rnd = random.Random(7 + style)
    cm1, cm2 = (M['crop'], M['leaf2']) if style == 0 else (M['wheat'], M['hay'])
    rows = 8
    for r in range(rows):
        y = -0.8 + r * 0.228
        box(0, y, 0.04, 1.76, 0.14, 0.05, M['dirt'])                                   # raised furrow
        for c in range(10):
            x = -0.84 + c * 0.187 + (0.04 if r % 2 else 0)
            if style == 0:
                for k in range(3):
                    sphere(x + rnd.uniform(-0.03, 0.03), y + rnd.uniform(-0.03, 0.03), 0.1 + rnd.uniform(0, 0.05), rnd.uniform(0.05, 0.08), cm1 if k % 2 else cm2, seg=6, noise=0.25, seed=r * 10 + c, sz=1.1)
            else:
                for k in range(5):
                    a = rnd.uniform(0, 6.28)
                    st = cyl(x + math.cos(a) * 0.04, y + math.sin(a) * 0.04, 0.06, 0.009, rnd.uniform(0.2, 0.3), M['wheat'], verts=4)
                    st.rotation_euler = (rnd.uniform(-0.2, 0.2), rnd.uniform(-0.2, 0.2), 0)
                    sphere(x + math.cos(a) * 0.04, y + math.sin(a) * 0.04, 0.3, 0.022, M['hay'], seg=5, sz=2.0)
    # fence on the two visible sides + corner posts
    for s in (-1, 1):
        for sy in (-1, 1):
            cyl(s * 0.97, sy * 0.97, 0, 0.035, 0.34, M['wood'], verts=6)
    fence_run(-0.97, -0.97, 0.97, -0.97, 0.3)
    fence_run(0.97, -0.97, 0.97, 0.97, 0.3)
    # scarecrow + hay bales + water barrel
    cyl(-0.45, 0.55, 0.04, 0.02, 0.66, M['wood'], verts=6)
    box(-0.45, 0.55, 0.52, 0.02, 0.44, 0.03, M['wood'])
    sphere(-0.45, 0.55, 0.76, 0.075, M['hay'], seg=7)
    cone(-0.45, 0.55, 0.8, 0.12, 0.12, M['thatch'], verts=8)
    box(-0.45, 0.55, 0.34, 0.07, 0.3, 0.2, M['cloth1'])
    haybale(0.7, 0.85, 0.04, 0.4)
    haybale(0.7, 0.55, 0.04, 0.2)
    barrel(-0.8, -0.85, 0.04)

# ------------------------------------------------------------------ town center 1-3
def m_townhall(level):
    if level >= 4:
        return m_townhall_big(level)
    if level == 1:
        palisade(0, 0, 1.4, 1.4, gap=('x', 1, 0.34))
        box(0, 0, 0, 2.0, 2.0, 0.28, M['stoneDark'])
        box(0, 0, 0.28, 1.84, 1.84, 0.62, M['plaster'])
        beams(0.9, 0.9, 0.28, 0.9, M['woodDark'])
        hip_roof(0, 0, 0.9, 1.84, 1.84, 1.1, M['thatch'], over=0.3, top=0.34)
        # entrance gable on the +X face with a striped awning porch
        box(1.02, 0, 0.0, 0.34, 0.9, 0.28, M['stone'])
        box(1.0, 0, 0.28, 0.2, 0.7, 0.6, M['plaster'])
        _prism_base(1.0, 0, 0.88, 0.22, 0.7, 0.3, M['thatch'], 0.08, 'y', 'gable', 0.0)
        door(1.11, 0, 'x', 0.44, 0.6)
        awning(1.12, 0, 0.74, 0.74, 0.4, 'x', M['awningBlue'])
        for t in (-0.55, 0.55):
            cyl(1.34, t, 0.0, 0.05, 0.7, M['woodDark'], verts=6)
        window(-0.45, -0.93, 'y', 0.28, 0.28)
        window(0.4, -0.93, 'y', 0.28, 0.28)
        cyl(0, 0, 1.9, 0.07, 0.34, M['woodDark'], verts=6)
        banner_pole(1.15, -1.1, 1.8, 'blue')
        banner_pole(-1.15, 1.1, 1.5, 'blue')
        lantern(1.25, 0.7, 0.8)
        lantern(1.25, -0.7, 0.8)
        chimney(-0.5, 0.5, 1.1, 0.6)
        barrel(1.2, 0.95); barrel(1.35, 0.8); crate(-1.1, -0.9, 0, 0.22, 0.3)
        haybale(-1.15, 0.2, 0, 0.5)
    elif level == 2:
        palisade(0, 0, 1.42, 1.42, gap=('x', 1, 0.36))
        box(0, 0, 0, 2.05, 2.05, 0.32, M['stone'])
        box(0, 0, 0.32, 1.88, 1.88, 0.7, M['plaster'])
        beams(0.92, 0.92, 0.32, 1.02, M['woodDark'])
        hip_roof(0, 0, 1.02, 1.88, 1.88, 1.2, M['thatch'], over=0.32, top=0.3)
        box(-0.4, 1.0, 0, 1.0, 0.56, 0.5, M['plaster'])
        beams(0.48, 0.26, 0.0, 0.5, M['woodDark']) if False else None
        _prism_base(-0.4, 1.0, 0.5, 1.0, 0.56, 0.42, M['thatch'], 0.14, 'x', 'annex', 0.0)
        box(1.04, 0, 0.0, 0.36, 0.96, 0.3, M['stone'])
        box(1.0, 0, 0.3, 0.22, 0.76, 0.66, M['plaster'])
        _prism_base(1.0, 0, 0.96, 0.24, 0.76, 0.34, M['thatch'], 0.08, 'y', 'gable', 0.0)
        door(1.12, 0, 'x', 0.48, 0.68)
        awning(1.13, 0, 0.82, 0.8, 0.44, 'x', M['awningBlue'])
        window(-0.45, -0.95, 'y', 0.3, 0.32); window(0.45, -0.95, 'y', 0.3, 0.32)
        cyl(0, 0, 2.18, 0.07, 0.42, M['woodDark'], verts=6)
        banner_pole(1.2, -1.15, 1.9, 'blue'); banner_pole(1.2, 1.15, 1.6, 'blue'); banner_pole(-1.2, -1.1, 1.5, 'blue')
        lantern(1.3, 0.75, 0.85); lantern(1.3, -0.75, 0.85)
        chimney(-0.6, 0.3, 1.2, 0.7)
        barrel(1.25, 0.9); sack(1.3, -0.95); crate(-1.15, -0.85, 0, 0.22, 0.3)
        log_pile(-1.2, 0.6, 0, 0.7, (3, 2), 0.06, 'y')
        well(1.3, -1.2) if False else None
    else:
        box(0, 0, 0, 1.96, 1.96, 0.42, M['stone'])
        box(0, 0, 0.42, 1.76, 1.76, 0.74, M['plaster'])
        beams(0.85, 0.85, 0.42, 1.16, M['woodDark'])
        hip_roof(0, 0, 1.16, 1.76, 1.76, 0.98, M['tile'], over=0.24, top=0.3)
        cyl(0, 0, 2.1, 0.05, 0.42, M['woodDark'], verts=6)
        flag(0, 0, 2.1, 0.42, 'blue')
        for sx in (-1, 1):
            for sy in (-1, 1):
                cyl(sx * 1.1, sy * 1.1, 0, 0.32, 1.3, M['stone'], verts=16, r2=0.29)
                cyl(sx * 1.1, sy * 1.1, 1.3, 0.36, 0.1, M['stoneDark'], verts=16)
                cone(sx * 1.1, sy * 1.1, 1.4, 0.44, 0.66, M['tile'], verts=16)
                box(sx * 1.1 + (-0.3 if sx > 0 else 0.3) * 0, sy * 1.1, 0.7, 0.05, 0.05, 0.14, M['black'])
        for sx, sy in ((-1, -1), (1, -1), (1, 1)):
            flag(sx * 1.1, sy * 1.1, 2.0, 0.3, 'blue')
        for sy in (-1, 1):
            box(0, sy * 1.1, 0, 1.46, 0.18, 0.52, M['stone'])
        box(-1.1, 0, 0, 0.18, 1.46, 0.52, M['stone'])
        for sy in (-1, 1):
            box(1.1, sy * 0.62, 0, 0.18, 0.72, 0.52, M['stone'])
        box(1.1, 0, 0.52, 0.18, 0.52, 0.18, M['stone'])
        box(1.12, 0, 0, 0.06, 0.36, 0.52, M['woodDark'])
        crenellations(0, 0, 0.52, 2.2, 2.2, M['stone'], n=7, hgt=0.12)
        door(0.88, 0, 'x', 0.42, 0.64)
        awning(0.89, 0, 0.78, 0.7, 0.4, 'x', M['awningBlue'])
        window(-0.42, -0.88, 'y', 0.28, 0.32); window(0.42, -0.88, 'y', 0.28, 0.32)
        lantern(1.35, 0.8, 0.9); lantern(1.35, -0.8, 0.9)

# ------------------------------------------------------------------ watch tower 1: timber tower with a striped roof (like the reference)
_tower_old = m_tower
def m_tower(level):
    if level != 1:
        return _tower_old(level)
    box(0, 0, 0, 0.98, 0.98, 0.32, M['stone'])
    for sx in (-0.4, 0.4):
        for sy in (-0.4, 0.4):
            cyl(sx, sy, 0.3, 0.07, 1.3, M['woodDark'], verts=7, r2=0.06)
    for z in (0.7, 1.15):
        for sx in (-0.4, 0.4):
            box(sx, 0, z, 0.05, 0.86, 0.05, M['wood'])
        for sy in (-0.4, 0.4):
            box(0, sy, z, 0.86, 0.05, 0.05, M['wood'])
    for i in range(2):
        for s in (-1, 1):
            d = box(0.4, 0, 0.55 + i * 0.45, 0.04, 1.0, 0.04, M['wood']); d.rotation_euler = (s * 0.7, 0, 0)
    box(0, 0, 1.5, 1.08, 1.08, 0.08, M['woodLight'])
    for sx in (-0.5, 0.5):
        box(sx, 0, 1.58, 0.05, 1.08, 0.3, M['wood'])
    for sy in (-0.5, 0.5):
        box(0, sy, 1.58, 1.08, 0.05, 0.3, M['wood'])
    for sx in (-0.45, 0.45):
        for sy in (-0.45, 0.45):
            cyl(sx, sy, 1.58, 0.04, 0.55, M['woodDark'], verts=6)
    hip_roof(0, 0, 2.05, 1.0, 1.0, 0.5, M['awningBlue'], over=0.16, top=0.0)
    ladder(0.5, 0.25, 1.5, 0.0, 0.0)
    flag(0, 0, 2.45, 0.4, 'blue')

# ------------------------------------------------------------------ barracks (3x3): main hall + forge lean-to + training yard
def m_barracks(style):
    roof = _roof(style)
    box(0, 0, 0, 2.3, 1.7, 0.36, M['stone'] if style else M['woodDark'])
    box(0, 0, 0.36, 2.2, 1.6, 0.64, M['plaster'])
    beams(1.08, 0.78, 0.36, 1.0, M['woodDark'])
    prism_roof(0, 0, 1.0, 2.2, 1.6, 0.98, roof, over=0.24, along='x', ridge_ext=0.05)
    door(1.1, 0.0, 'x', 0.42, 0.66)
    awning(1.11, 0, 0.82, 0.7, 0.4, 'x', M['awningRed'] if style == 1 else M['awningBlue'])
    for i, xx in enumerate((-0.6, 0.0, 0.6)):
        window(xx, -0.8, 'y', 0.24, 0.3)
    chimney(-0.8, 0.45, 1.2, 0.7)
    # forge lean-to at the back + shield wall + weapon racks + dummy
    box(-0.5, 1.12, 0, 1.2, 0.5, 0.5, M['stone'])
    _prism_base(-0.5, 1.12, 0.5, 1.2, 0.5, 0.3, roof, 0.1, 'x', 'forge', 0.0)
    for i, t in enumerate(('blue', 'wood', 'blue', 'wood')):
        cyl(-0.8 + i * 0.5, -0.83, 0.5, 0.15, 0.045, team_mat('blue') if t == 'blue' else M['woodLight'], verts=14, rx=math.pi / 2)
        cyl(-0.8 + i * 0.5, -0.86, 0.5, 0.05, 0.03, M['metal'], verts=8, rx=math.pi / 2)
    box(1.15, -1.0, 0, 0.8, 0.06, 0.06, M['woodDark'])
    cyl(0.78, -1.0, 0, 0.03, 0.5, M['woodDark'], verts=6); cyl(1.52, -1.0, 0, 0.03, 0.5, M['woodDark'], verts=6)
    for i in range(5):
        cyl(0.88 + i * 0.14, -1.0, 0.05, 0.012, 0.78, M['wood'], verts=5, ry=math.radians(-6))
        cone(0.88 + i * 0.14 - 0.04, -1.0, 0.82, 0.026, 0.08, M['metal'], verts=5)
    cyl(1.3, 0.95, 0, 0.045, 0.62, M['wood'], verts=6)
    box(1.3, 0.95, 0.42, 0.34, 0.045, 0.045, M['wood'])
    sphere(1.3, 0.95, 0.66, 0.1, M['canvas'], seg=7)
    banner_pole(-1.15, -1.0, 1.7, 'blue')
    banner_pole(1.25, 1.1, 1.5, 'blue')

# ------------------------------------------------------------------ lumber camp: sawpit, big log piles, planks
def m_lumber(style):
    roof = _roof(style)
    for sx in (-0.72, 0.72):
        for sy in (-0.62, 0.62):
            cyl(sx, sy, 0, 0.07, 0.72, M['woodDark'], verts=7)
    box(0, -0.62, 0.68, 1.5, 0.08, 0.08, M['wood']); box(0, 0.62, 0.68, 1.5, 0.08, 0.08, M['wood'])
    prism_roof(0, 0, 0.72, 1.44, 1.24, 0.46, roof, over=0.2, along='x')
    box(0, 0, 0, 1.6, 1.4, 0.05, M['dirt'])
    log_pile(0.0, 0.2, 0.05, 1.0, (4, 3, 2), 0.06, 'x')
    log_pile(1.0, -0.3, 0.0, 0.7, (4, 3, 2), 0.065, 'y')
    box(-0.3, -0.35, 0.05, 0.9, 0.34, 0.18, M['woodLight'])                         # sawn planks
    for i in range(4):
        box(-0.3, -0.35, 0.23 + i * 0.03, 0.9, 0.3, 0.025, M['woodLight'])
    cyl(-0.95, -0.6, 0, 0.15, 0.26, M['bark'], verts=10)                            # chopping block + axe
    cyl(-0.95, -0.5, 0.26, 0.016, 0.45, M['wood'], verts=6, rx=math.radians(70))
    box(-0.95, -0.22, 0.58, 0.05, 0.16, 0.1, M['metal'])
    # saw horse + big saw
    for s in (-1, 1):
        box(0.6 + s * 0.22, 0.6, 0, 0.04, 0.3, 0.3, M['wood']).rotation_euler = (0, s * 0.25, 0)
    box(0.6, 0.6, 0.28, 0.5, 0.05, 0.05, M['wood'])
    box(0.6, 0.55, 0.34, 0.6, 0.012, 0.09, M['metal'])
    banner_pole(-1.0, 0.9, 1.1, 'blue')

# ------------------------------------------------------------------ mining camps: shed, mine entrance in a rock hill, ore cart on rails
def m_mine(style, kind):
    roof = _roof(style)
    for sx in (-0.7, 0.7):
        for sy in (-0.6, 0.6):
            cyl(sx - 0.15, sy, 0, 0.07, 0.7, M['woodDark'], verts=7)
    box(-0.15, -0.6, 0.66, 1.5, 0.08, 0.08, M['wood']); box(-0.15, 0.6, 0.66, 1.5, 0.08, 0.08, M['wood'])
    prism_roof(-0.15, 0, 0.7, 1.4, 1.2, 0.42, roof, over=0.18, along='x')
    box(-0.15, 0, 0, 1.55, 1.35, 0.05, M['dirt'])
    # rock hill with a timbered mine entrance on the +X side
    for i, (x, y, r) in enumerate(((0.7, 0.55, 0.45), (0.9, 0.1, 0.38), (0.75, -0.4, 0.34), (0.45, 0.15, 0.4))):
        sphere(x, y, 0.12, r, M['rock'], seg=9, noise=0.22, seed=i + 3, sz=0.8)
    for s in (-1, 1):
        box(1.12, 0.05 + s * 0.22, 0.0, 0.08, 0.08, 0.5, M['woodDark'])
    box(1.12, 0.05, 0.48, 0.1, 0.56, 0.08, M['woodDark'])
    box(1.1, 0.05, 0.0, 0.04, 0.36, 0.46, M['black'])
    for i in range(5):                                                               # rails
        box(0.55 + i * 0.12, 0.05, 0.02, 0.03, 0.34, 0.015, M['woodDark'])
    for s in (-1, 1):
        box(0.85, 0.05 + s * 0.12, 0.035, 0.7, 0.02, 0.02, M['iron'])
    # ore cart
    box(0.45, 0.05, 0.1, 0.36, 0.28, 0.16, M['wood'])
    for sx in (-0.1, 0.1):
        for s in (-1, 1):
            cyl(0.45 + sx, 0.05 + s * 0.15, 0.06, 0.06, 0.025, M['iron'], verts=8, rx=math.pi / 2)
    rnd = random.Random(5)
    for i in range(7):
        if kind == 'gold':
            sphere(0.45 + rnd.uniform(-0.1, 0.1), 0.05 + rnd.uniform(-0.08, 0.08), 0.27 + rnd.uniform(0, 0.04), rnd.uniform(0.04, 0.065), M['gold'], seg=6, noise=0.3, seed=i)
        else:
            sphere(0.45 + rnd.uniform(-0.1, 0.1), 0.05 + rnd.uniform(-0.08, 0.08), 0.27 + rnd.uniform(0, 0.04), rnd.uniform(0.05, 0.08), M['rock'], seg=6, noise=0.3, seed=i)
    # ore heaps by the shed + tools
    for i in range(10):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0, 0.28)
        mt = M['gold'] if kind == 'gold' else M['rock']
        sphere(-0.85 + math.cos(a) * d, -0.55 + math.sin(a) * d, 0.05, rnd.uniform(0.05, 0.1), mt, seg=6, noise=0.3, seed=i + 40, sz=0.8)
    cyl(-0.4, -0.85, 0.0, 0.015, 0.6, M['wood'], verts=6, ry=math.radians(18))
    box(-0.3, -0.85, 0.55, 0.22, 0.03, 0.03, M['metal'])
    banner_pole(-0.95, 0.85, 1.1, 'blue')
