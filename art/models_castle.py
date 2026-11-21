# Castle Age models (exec'd by gen_assets.py; shares its globals)

def roof_for(style):
    return M['slate'] if style >= 2 else (M['tile'] if style == 1 else M['thatch'])

def arch_window(x, y, face='y', w=0.2, h=0.34, mat=None):
    mat = mat or M['black']
    if face == 'y':
        box(x, y, 0, w, 0.05, h, mat)
        cyl(x, y - 0.01, h, w / 2, 0.05, mat, verts=10, rx=math.pi / 2)
    else:
        box(x, y, 0, 0.05, w, h, mat)
        cyl(x + 0.01, y, h, w / 2, 0.05, mat, verts=10, ry=math.pi / 2)

def round_tower(x, y, r, h, wall, roof, roof_h, flagteam=None, ring=True):
    cyl(x, y, 0, r, h, wall, verts=16, r2=r * 0.94)
    if ring:
        cyl(x, y, h, r * 1.12, 0.08, M['stoneDark'], verts=16)
    cone(x, y, h + 0.08, r * 1.18, roof_h, roof, verts=16)
    if flagteam:
        flag(x, y, h + 0.08 + roof_h, 0.3, flagteam)

# ------------------------------------------------------------------ townhall levels 4-7 (3x3)
def m_townhall_big(level):
    castle = level >= 5
    roofm = M['slate'] if castle else M['tile']
    wallm = M['stone']
    # curtain wall ring with a gate on +X
    ring = 1.32
    wh = 0.55 + 0.05 * (level - 4)
    for sy in (-1, 1):
        box(0, sy * ring, 0, ring * 2, 0.16, wh, wallm)
    box(-ring, 0, 0, 0.16, ring * 2, wh, wallm)
    for sy in (-1, 1):
        box(ring, sy * 0.78, 0, 0.16, 0.62, wh, wallm)
    box(ring, 0, wh - 0.1, 0.16, 0.5, 0.18, wallm)
    box(ring + 0.02, 0, 0, 0.05, 0.34, wh - 0.1, M['woodDark'])
    crenellations(0, 0, wh, ring * 2, ring * 2, M['stone'], n=9, hgt=0.12)
    # inner keep
    base_h = 0.5 if level < 6 else 0.6
    box(0, 0, 0, 1.75, 1.75, base_h, wallm)
    box(0, 0, base_h, 1.6, 1.6, 0.8, M['plaster'] if level < 6 else M['stone'])
    beams(0.78, 0.78, base_h, base_h + 0.8, M['woodDark'])
    top = base_h + 0.8
    hip_roof(0, 0, top, 1.6, 1.6, 0.95 + 0.05 * (level - 4), roofm, over=0.2, top=0.3)
    if castle:
        # tall central tower
        cyl(0, 0, top + 0.5, 0.36, 0.9 + 0.1 * (level - 5), M['stone'], verts=14, r2=0.32)
        cyl(0, 0, top + 1.4 + 0.1 * (level - 5), 0.44, 0.08, M['stoneDark'], verts=14)
        cone(0, 0, top + 1.48 + 0.1 * (level - 5), 0.44, 0.55, roofm, verts=14)
        flag(0, 0, top + 2.03 + 0.1 * (level - 5), 0.4, 'blue')
        if level >= 7:
            sphere(0, 0, top + 2.03 + 0.1 * (level - 5) - 0.02, 0.06, M['gold'], seg=8)
    else:
        cyl(0, 0, top + 0.9, 0.05, 0.4, M['woodDark'], verts=6)
        flag(0, 0, top + 0.9, 0.4, 'blue')
    # four corner towers
    for sx in (-1, 1):
        for sy in (-1, 1):
            tx, ty = sx * ring, sy * ring
            round_tower(tx, ty, 0.27 + 0.02 * (level - 4), 1.0 + 0.12 * (level - 4), wallm, roofm, 0.5, 'blue' if (sx, sy) != (1, 1) else None)
            if level >= 7:
                sphere(tx, ty, 1.0 + 0.12 * (level - 4) + 0.58, 0.05, M['gold'], seg=6)
    if level >= 6:
        for t in (-0.7, 0.7):                        # extra turrets mid-wall
            round_tower(t, -ring, 0.17, 0.75, wallm, roofm, 0.35)
            round_tower(-ring, t, 0.17, 0.75, wallm, roofm, 0.35)
    if level >= 7:
        for t in (-0.45, 0.45):
            flag(ring, t, wh, 0.5, 'blue')
    door(0.8, 0, 'x', 0.4, 0.6)
    window(-0.4, -0.8, 'y', 0.26, 0.3)
    window(0.4, -0.8, 'y', 0.26, 0.3)
    arch_window(-0.4, -0.8, 'y', 0.22, 0.36, M['blue']) if level >= 6 else None

# ------------------------------------------------------------------ tower 4 (Castle Age)
def m_tower4():
    cyl(0, 0, 0, 0.62, 2.0, M['stone'], verts=18, r2=0.52)
    cyl(0, 0, 2.0, 0.76, 0.14, M['stoneDark'], verts=18)
    # wooden hoarding gallery
    cyl(0, 0, 2.14, 0.72, 0.34, M['wood'], verts=18)
    cone(0, 0, 2.48, 0.85, 0.85, M['slate'], verts=18)
    for a in (0.3, 1.4, 2.5, 3.6, 4.7, 5.6):
        box(math.cos(a) * 0.72, math.sin(a) * 0.72, 2.22, 0.07, 0.07, 0.14, M['black'], rz=a)
    for a in (0.1, 1.0, 2.2, 4.2, 5.3):
        box(math.cos(a) * 0.58, math.sin(a) * 0.58, 1.3, 0.07, 0.07, 0.3, M['black'], rz=a)
        box(math.cos(a) * 0.6, math.sin(a) * 0.6, 0.6, 0.06, 0.06, 0.22, M['black'], rz=a)
    door(0.6, 0, 'x', 0.32, 0.5, M['woodDark'])
    flag(0, 0, 3.3, 0.5, 'blue')
    flag(0.5, -0.5, 2.5, 0.3, 'blue')

# ------------------------------------------------------------------ stable (3x3)
def horse(x, y, z=0.0, rz=0.0, sc=1.0, coat=None, stride=0.0, name='horse'):
    """horse standing on the ground, facing +X (before rz); returns list of objects"""
    coat = coat or M['hide']
    start = len(_created)
    s = sc
    sphere(0, 0, 0.62 * s, 0.34 * s, coat, sx=1.6, sy=0.62, sz=0.75, seg=10)                 # body
    sphere(0.38 * s, 0, 0.78 * s, 0.17 * s, coat, sx=1.2, sy=0.7, sz=1.0, seg=8)              # chest
    # neck + head
    neck = cyl(0.42 * s, 0, 0.78 * s, 0.09 * s, 0.34 * s, coat, verts=8, r2=0.075 * s, ry=math.radians(-35))
    sphere(0.66 * s, 0, 1.08 * s, 0.1 * s, coat, sx=1.5, sy=0.7, sz=0.8, seg=8)
    cone(0.58 * s, 0.045 * s, 1.17 * s, 0.025 * s, 0.07 * s, coat, verts=5)
    cone(0.58 * s, -0.045 * s, 1.17 * s, 0.025 * s, 0.07 * s, coat, verts=5)
    box(0.46 * s, 0, 0.84 * s, 0.06 * s, 0.03 * s, 0.26 * s, M['black'], rz=0)                # mane
    # tail
    cyl(-0.5 * s, 0, 0.7 * s, 0.03 * s, 0.4 * s, M['black'], verts=6, r2=0.012 * s, ry=math.radians(25))
    # legs (pivot at the top)
    for lx, ly, ph in ((0.3, -0.12, 1), (0.3, 0.12, -1), (-0.3, -0.12, -1), (-0.3, 0.12, 1)):
        leg = cyl(0, 0, 0, 0.045 * s, 0.5 * s, coat, verts=7, r2=0.035 * s)
        leg.data.transform(Matrix.Translation((0, 0, -0.5 * s)))
        leg.location = (lx * s, ly * s, 0.52 * s)
        leg.rotation_euler = (0, stride * ph, 0)
        cyl(lx * s + math.sin(stride * ph) * -0.5 * s * 0.9, ly * s, 0.0, 0.05 * s, 0.05 * s, M['black'], verts=6)
    objs = _created[start:]
    root = bpy.data.objects.new(name + '_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.location = (x, y, z)
    root.rotation_euler = (0, 0, rz)
    for o in objs:
        o.parent = root
    return root

def m_stable(style):
    # long stone-and-timber barn with open stalls on the +X side
    box(0, 0, 0, 2.3, 1.4, 0.35, M['stone'])
    box(-0.1, -0.05, 0.35, 2.1, 1.3, 0.6, M['plaster'])
    beams(1.0, 0.62, 0.35, 0.95, M['woodDark'])
    prism_roof(-0.1, -0.05, 0.95, 2.1, 1.3, 0.85, M['slate'], over=0.22, along='x', ridge_ext=0.05)
    door(0.94, 0.0, 'x', 0.55, 0.6)
    window(-0.5, -0.7, 'y', 0.22, 0.26)
    window(0.2, -0.7, 'y', 0.22, 0.26)
    # paddock fence in front
    for i in range(7):
        py = -1.2 + i * 0.4
        cyl(1.3, py, 0, 0.04, 0.4, M['wood'], verts=6)
    box(1.3, 0.0, 0.15, 0.04, 2.45, 0.04, M['wood'])
    box(1.3, 0.0, 0.3, 0.04, 2.45, 0.04, M['wood'])
    for i in range(4):
        px = 0.6 + i * 0.25
        cyl(px, -1.2, 0, 0.04, 0.4, M['wood'], verts=6)
    box(0.9, -1.2, 0.15, 0.7, 0.04, 0.04, M['wood'])
    horse(1.0, 0.5, 0, math.radians(200), 0.8, M['hide'])
    horse(1.05, -0.45, 0, math.radians(160), 0.8, M['woodLight'])
    box(0.6, 1.05, 0, 0.5, 0.18, 0.1, M['hay'])
    box(0.1, 1.05, 0, 0.3, 0.18, 0.16, M['hay'])
    banner_pole(-1.0, 1.2, 1.6, 'blue')

# ------------------------------------------------------------------ siege machines + workshop
def ram_model(x=0.0, y=0.0, z=0.0, rz=0.0, team='blue', name='ram'):
    start = len(_created)
    L = 1.1
    for sy in (-0.3, 0.3):
        box(0, sy, 0.16, L, 0.06, 0.06, M['woodDark'])
    for sx in (-0.35, 0.35):
        box(sx, 0, 0.16, 0.06, 0.66, 0.06, M['woodDark'])
    for sx in (-0.38, 0.38):
        for sy in (-0.36, 0.36):
            cyl(sx, sy, 0.14, 0.14, 0.05, M['wood'], verts=12, rx=math.pi / 2)
            cyl(sx, sy, 0.14, 0.04, 0.07, M['iron'], verts=8, rx=math.pi / 2)
    # covered frame: posts + gabled hide roof
    for sx in (-0.35, 0.35):
        for sy in (-0.28, 0.28):
            cyl(sx, sy, 0.18, 0.035, 0.4, M['wood'], verts=6)
    prism_roof(0, 0, 0.55, 0.95, 0.56, 0.25, M['wood'], over=0.05, along='x', ridge_ext=0.05)
    prism_roof(0, 0, 0.55, 0.95, 0.56, 0.01, team_mat(team), over=0.0, along='x')
    # swinging log with an iron head
    cyl(-0.45, 0, 0.36, 0.1, 1.15, M['bark'], verts=10, ry=math.pi / 2)
    cone(0.7, 0, 0.36, 0.11, 0.2, M['iron'], verts=8).rotation_euler = (0, math.pi / 2, 0)
    box(0, 0, 0.5, 0.03, 0.03, 0.12, M['black'])
    flag(-0.4, 0, 0.8, 0.3, team)
    objs = _created[start:]
    root = bpy.data.objects.new(name + '_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.location = (x, y, z)
    root.rotation_euler = (0, 0, rz)
    for o in objs:
        o.parent = root
    return root

def mangonel_model(x=0.0, y=0.0, z=0.0, rz=0.0, team='blue', arm=0.0, name='mangonel'):
    start = len(_created)
    box(0, 0, 0.14, 0.9, 0.55, 0.07, M['woodDark'])
    for sx in (-0.3, 0.3):
        for sy in (-0.3, 0.3):
            cyl(sx, sy, 0.13, 0.13, 0.05, M['wood'], verts=12, rx=math.pi / 2)
            cyl(sx, sy, 0.13, 0.035, 0.07, M['iron'], verts=8, rx=math.pi / 2)
    for sy in (-0.2, 0.2):
        box(-0.05, sy, 0.2, 0.07, 0.07, 0.4, M['wood'])
        box(0.0, sy, 0.2, 0.07, 0.07, 0.4, M['wood'], rz=0)
    box(0.0, 0, 0.55, 0.1, 0.5, 0.06, M['wood'])
    # throwing arm
    a = cyl(0, 0, 0, 0.035, 0.95, M['wood'], verts=6)
    a.location = (-0.05, 0, 0.5)
    a.rotation_euler = (0, math.radians(-110 + arm), 0)
    sphere(0.38 + arm * 0.004, 0, 0.9 - abs(arm) * 0.002, 0.09, M['rock'], seg=6, noise=0.25)
    box(-0.4, 0, 0.2, 0.14, 0.14, 0.12, M['stoneDark'])
    flag(-0.35, 0.26, 0.2, 0.4, team)
    objs = _created[start:]
    root = bpy.data.objects.new(name + '_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.location = (x, y, z)
    root.rotation_euler = (0, 0, rz)
    for o in objs:
        o.parent = root
    return root

def m_siege(style):
    # open timber workshop with a gantry; machines parked outside
    for sx in (-0.9, 0.9):
        for sy in (-0.7, 0.7):
            cyl(sx, sy, 0, 0.09, 1.0, M['woodDark'], verts=8)
    box(0, -0.7, 0.95, 2.0, 0.1, 0.1, M['wood'])
    box(0, 0.7, 0.95, 2.0, 0.1, 0.1, M['wood'])
    prism_roof(0, 0, 1.0, 1.9, 1.5, 0.6, M['slate'], over=0.25, along='x')
    box(0, 0, 0, 2.1, 1.7, 0.06, M['dirt'])
    log_pile(-0.3, 0.45, 0.06, 1.0, (4, 3, 2), 0.07, 'x')
    box(-0.5, -0.35, 0.06, 0.8, 0.5, 0.35, M['woodLight'])
    cyl(-0.5, -0.35, 0.42, 0.16, 0.1, M['iron'], verts=10)
    ram_model(1.25, -0.35, 0, math.radians(-20), 'blue', 'ram_parked')
    mangonel_model(1.2, 0.8, 0, math.radians(200), 'blue', 0.0, 'mang_parked')
    banner_pole(-1.2, -1.2, 1.5, 'blue')
    barrel(-1.2, 1.0)

# ------------------------------------------------------------------ monastery (3x3)
def m_monastery(style):
    nave = M['plaster']
    box(0, 0, 0, 2.3, 1.05, 0.3, M['stone'])
    box(0, 0, 0.3, 2.2, 0.95, 0.75, nave)
    prism_roof(0, 0, 1.05, 2.2, 0.95, 0.7, M['slate'], over=0.14, along='x')
    box(0.1, 0, 0, 1.0, 2.1, 0.3, M['stone'])
    box(0.1, 0, 0.3, 0.9, 2.0, 0.75, nave)
    prism_roof(0.1, 0, 1.05, 0.9, 2.0, 0.65, M['slate'], over=0.14, along='y')
    # bell tower with spire
    box(-0.95, -0.55, 0, 0.62, 0.62, 1.55, M['stone'])
    box(-0.95, -0.55, 1.55, 0.7, 0.7, 0.08, M['stoneDark'])
    hip_roof(-0.95, -0.55, 1.63, 0.62, 0.62, 0.85, M['slate'], over=0.04, top=0.0)
    box(-0.95, -0.55, 2.45, 0.025, 0.025, 0.28, M['gold'])
    box(-0.95, -0.55, 2.62, 0.14, 0.025, 0.025, M['gold'])
    arch_window(-0.95, -0.87, 'y', 0.16, 0.3, M['black'])
    arch_window(-0.64, -0.55, 'x', 0.16, 0.3, M['black'])
    box(-0.95, -0.55, 1.1, 0.2, 0.64, 0.28, M['black'])
    # entrance + stained windows
    door(1.12, 0, 'x', 0.42, 0.55)
    for i, mat in enumerate(('blue', 'red', 'blue')):
        arch_window(-0.55 + i * 0.5, -0.5, 'y', 0.2, 0.4, M[mat])
    box(0.1, 1.05, 0.35, 0.02, 0.02, 0.0, M['gold'])
    # garden hedge + cross
    for i in range(5):
        sphere(1.15, -0.9 + i * 0.4, 0.16, 0.17, M['leaf1'], sz=0.8, seg=8, noise=0.1, seed=i)
    box(1.2, 1.0, 0, 0.05, 0.05, 0.5, M['stoneDark'])
    box(1.2, 1.0, 0.33, 0.22, 0.05, 0.05, M['stoneDark'])

# ------------------------------------------------------------------ keep (2x2)
def m_keep(style):
    box(0, 0, 0, 1.5, 1.5, 0.15, M['stoneDark'])
    box(0, 0, 0.15, 1.36, 1.36, 1.7, M['stone'])
    box(0, 0, 1.85, 1.6, 1.6, 0.14, M['stoneDark'])
    crenellations(0, 0, 1.99, 1.6, 1.6, M['stone'], n=7, hgt=0.2)
    for sx in (-1, 1):
        for sy in (-1, 1):
            cyl(sx * 0.72, sy * 0.72, 1.5, 0.2, 0.7, M['stone'], verts=10)
            cone(sx * 0.72, sy * 0.72, 2.2, 0.26, 0.42, M['slate'], verts=10)
            if (sx, sy) in ((-1, -1), (1, -1)):
                flag(sx * 0.72, sy * 0.72, 2.62, 0.3, 'blue')
    hip_roof(0, 0, 1.99, 0.8, 0.8, 0.5, M['slate'], over=0.04, top=0.0)
    for z in (0.7, 1.35):
        box(0.69, 0, z, 0.05, 0.07, 0.24, M['black'])
        box(0, -0.69, z, 0.07, 0.05, 0.24, M['black'])
    door(0.69, 0, 'x', 0.34, 0.5, M['woodDark'])
    box(0.7, 0, 0.52, 0.04, 0.36, 0.06, M['iron'])

# ------------------------------------------------------------------ blacksmith (2x2)
def m_blacksmith(style):
    box(0, 0, 0, 1.5, 1.35, 0.3, M['stone'])
    box(0, 0, 0.3, 1.44, 1.29, 0.5, M['plaster'])
    beams(0.7, 0.62, 0.3, 0.8, M['woodDark'])
    prism_roof(0, 0, 0.8, 1.44, 1.29, 0.6, roof_for(max(1, style)), over=0.18, along='y')
    box(-0.45, 0.3, 0.8, 0.28, 0.28, 0.95, M['stoneDark'])
    sphere(-0.45, 0.3, 1.7, 0.1, M['fire'], seg=6)
    door(0.7, 0.1, 'x', 0.3, 0.5)
    box(0.95, -0.35, 0, 0.22, 0.14, 0.2, M['iron'])            # anvil
    box(0.95, -0.35, 0.2, 0.34, 0.1, 0.07, M['iron'])
    cyl(1.0, 0.4, 0, 0.1, 0.2, M['wood'], verts=10)            # quench barrel
    box(0.2, -0.68, 0.2, 0.5, 0.05, 0.35, M['woodDark'])
    for i in range(3):
        cyl(0.05 + i * 0.17, -0.74, 0.25, 0.012, 0.3, M['metal'], verts=5)
    cyl(-0.2, -0.7, 0.3, 0.13, 0.03, M['blue'], verts=12, rx=math.pi / 2)

# ------------------------------------------------------------------ market (3x3)
def stall(x, y, mat_a, rz=0.0):
    for sx in (-0.3, 0.3):
        for sy in (-0.22, 0.22):
            cyl(x + sx, y + sy, 0, 0.03, 0.55, M['woodDark'], verts=6)
    box(x, y, 0.55, 0.78, 0.6, 0.04, mat_a)
    box(x, y - 0.3, 0.45, 0.8, 0.05, 0.12, mat_a)
    box(x, y + 0.04, 0.0, 0.7, 0.4, 0.28, M['woodLight'])
    for i in range(4):
        sphere(x - 0.25 + i * 0.17, y + 0.04, 0.34, 0.07, M['red'] if i % 2 else M['gold'], seg=6)

def m_market(style):
    box(0, 0, 0, 2.7, 2.7, 0.05, M['stoneDark'])
    stall(-0.75, -0.8, M['red'])
    stall(0.3, -0.85, M['blue'])
    stall(-0.8, 0.45, M['blue'])
    stall(0.45, 0.6, M['red'])
    for p in ((1.0, -0.3), (1.05, -0.05), (-0.3, 1.1)):
        crate(p[0], p[1], 0.05, 0.2, 0.3)
    barrel(1.1, 0.95)
    barrel(1.0, 1.15)
    sack(-1.05, -0.2)
    sack(-1.1, -0.0)
    # signpost with a coin
    cyl(1.15, -1.05, 0, 0.03, 1.0, M['woodDark'], verts=6)
    cyl(1.15, -1.05, 0.95, 0.15, 0.03, M['gold'], verts=14, rx=math.pi / 2)
    banner_pole(-1.2, 1.15, 1.3, 'blue')

# ------------------------------------------------------------------ castle (4x4)
def m_castle(style):
    ring = 1.8
    wh = 0.85
    for sy in (-1, 1):
        box(0, sy * ring, 0, ring * 2, 0.24, wh, M['stone'])
    box(-ring, 0, 0, 0.24, ring * 2, wh, M['stone'])
    for sy in (-1, 1):
        box(ring, sy * 1.1, 0, 0.24, 1.4, wh, M['stone'])
    box(ring, 0, wh - 0.15, 0.24, 0.8, 0.3, M['stone'])
    box(ring + 0.03, 0, 0, 0.07, 0.8, wh - 0.15, M['woodDark'])
    for z in (0.15, 0.35, 0.55):
        box(ring + 0.04, 0, z, 0.03, 0.8, 0.03, M['iron'])
    crenellations(0, 0, wh, ring * 2, ring * 2, M['stone'], n=13, hgt=0.16)
    box(0, 0, 0, 3.4, 3.4, 0.04, M['dirt'])
    # central keep
    box(0, 0, 0, 2.1, 2.1, 0.5, M['stone'])
    box(0, 0, 0.5, 1.95, 1.95, 1.0, M['stone'])
    crenellations(0, 0, 1.5, 2.1, 2.1, M['stone'], n=8, hgt=0.22)
    box(0, 0, 1.5, 2.1, 2.1, 0.1, M['stoneDark'])
    box(0, 0, 1.6, 1.1, 1.1, 0.7, M['plaster'])
    hip_roof(0, 0, 2.3, 1.1, 1.1, 0.8, M['slate'], over=0.12, top=0.0)
    flag(0, 0, 3.1, 0.55, 'blue')
    for sx in (-1, 1):
        for sy in (-1, 1):
            round_tower(sx * ring, sy * ring, 0.4, 1.7, M['stone'], M['slate'], 0.8, 'blue')
    for t in (-0.9, 0.9):
        round_tower(t, -ring, 0.22, 1.1, M['stone'], M['slate'], 0.5)
        round_tower(-ring, t, 0.22, 1.1, M['stone'], M['slate'], 0.5)
    door(1.0, 0, 'x', 0.5, 0.7)
    for z in (0.3, 0.8, 1.2):
        arch_window(0.4 * (z % 2) - 0.2, -1.0, 'y', 0.18, 0.28)
    for t in (-0.5, 0.5):
        flag(ring, t, wh, 0.45, 'blue')

# ------------------------------------------------------------------ university (3x3)
def m_university(style):
    box(0, 0, 0, 2.4, 1.7, 0.4, M['stone'])
    box(0, 0, 0.4, 2.3, 1.6, 0.9, M['plaster'])
    beams(1.1, 0.75, 0.4, 1.3, M['woodDark'])
    prism_roof(0, 0, 1.3, 2.3, 1.6, 0.8, M['slate'], over=0.2, along='x')
    # clock tower
    box(-0.8, 0.5, 0, 0.7, 0.7, 1.9, M['stone'])
    cyl(-0.8, 0.5, 1.9, 0.5, 0.1, M['stoneDark'], verts=4, r2=0.5)
    hip_roof(-0.8, 0.5, 2.0, 0.7, 0.7, 0.9, M['slate'], over=0.06, top=0.0)
    sphere(-0.8, 0.5, 2.95, 0.07, M['gold'], seg=8)
    cyl(-0.45, 0.5, 1.4, 0.2, 0.04, M['white'], verts=16, ry=math.pi / 2)
    cyl(-0.42, 0.5, 1.4, 0.16, 0.02, M['gold'], verts=16, ry=math.pi / 2)
    # arched windows + steps + door
    for i in range(4):
        arch_window(-0.3 + i * 0.4, -0.81, 'y', 0.2, 0.42, M['blue'])
    door(1.15, 0, 'x', 0.45, 0.65)
    for i in range(3):
        box(1.22 + i * 0.1, 0, 0, 0.1, 0.7, 0.07 * (3 - i), M['stoneDark'])
    banner_pole(1.1, -1.1, 1.6, 'blue')
    banner_pole(1.1, 1.1, 1.6, 'blue')
