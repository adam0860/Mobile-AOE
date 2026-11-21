# Imperial Age models (exec'd by gen_assets.py after models_castle.py / models_units2.py)


# ------------------------------------------------------------------ Town Center levels 8-10 (Imperial palace)
_townhall_big_castle = m_townhall_big
def m_townhall_big(level):
    if level < 8:
        return _townhall_big_castle(level)
    saved = {k: M[k] for k in ('plaster', 'stone', 'slate', 'tile')}
    M['plaster'], M['stone'], M['slate'], M['tile'] = M['marblePlaster'], M['marble'], M['imperial'], M['imperial']
    try:
        _townhall_big_castle(level)
        top = 0.6 + 0.8
        zc = top + 1.4 + 0.1 * (level - 5)
        cyl(0, 0, zc, 0.46, 0.05, M['gold'], verts=14)              # gold band on the central tower
        sphere(0, 0, zc + 0.62, 0.13, M['gold'], seg=8)
        if level >= 9:
            for sx in (-1, 1):
                for sy in (-1, 1):
                    round_tower(sx * 0.84, sy * 0.84, 0.16, 1.15, M['stone'], M['imperial'], 0.4)
                    sphere(sx * 0.84, sy * 0.84, 1.15 + 0.08 + 0.45, 0.045, M['gold'], seg=6)
        if level >= 10:
            for t in (-0.6, 0.6):
                flag(1.32, t, 0.9, 0.7, 'blue')
                flag(-1.32, t, 0.9, 0.7, 'blue')
            cyl(0, 0, zc + 0.05, 0.5, 0.03, M['gold'], verts=14)
    finally:
        for k, v in saved.items():
            M[k] = v

# ------------------------------------------------------------------ Bombard Tower (2x2)
def m_bombard():
    cyl(0, 0, 0, 0.7, 0.2, M['stoneDark'], verts=14)
    cyl(0, 0, 0.2, 0.6, 1.5, M['marble'], verts=16, r2=0.55)
    cyl(0, 0, 1.7, 0.74, 0.14, M['stoneDark'], verts=16)
    cyl(0, 0, 1.84, 0.62, 0.1, M['marble'], verts=16)
    for i in range(10):
        a = i / 10 * math.pi * 2
        box(math.cos(a) * 0.68, math.sin(a) * 0.68, 1.94, 0.17, 0.2, 0.2, M['marble'], rz=a)
    cyl(0, 0, 1.2, 0.62, 0.05, M['gold'], verts=16)
    # cannon on the roof, pointing +X
    box(0.1, 0, 1.94, 0.5, 0.3, 0.12, M['woodDark'])
    cyl(-0.1, 0, 2.15, 0.12, 1.0, M['iron'], verts=12, r2=0.1, ry=math.radians(84))
    cyl(0.5, 0, 2.17, 0.135, 0.07, M['gold'], verts=12, ry=math.radians(84))
    for i in range(4):
        sphere(-0.3 + (i % 2) * 0.16, -0.35 + (i // 2) * 0.16, 2.0, 0.07, M['black'], seg=6)
    for a in (0.1, 1.1, 2.3, 4.3, 5.4):
        box(math.cos(a) * 0.57, math.sin(a) * 0.57, 0.9, 0.07, 0.07, 0.26, M['black'], rz=a)
    door(0.6, 0, 'x', 0.32, 0.5, M['woodDark'])
    barrel(0.85, 0.55)
    barrel(0.95, 0.38)
    flag(-0.3, 0.3, 2.06, 0.5, 'blue')

# ------------------------------------------------------------------ siege units
def trebuchet_model(x=0.0, y=0.0, z=0.0, rz=0.0, team='blue', name='treb'):
    start = len(_created)
    for sy in (-0.45, 0.45):
        box(0, sy, 0.0, 1.5, 0.1, 0.1, M['woodDark'])
        # A-frame legs
        for sx, lean in ((-0.15, 12), (-0.15, -12)):
            c = cyl(sx, sy, 0.05, 0.05, 0.95, M['wood'], verts=6)
            c.rotation_euler = (math.radians(lean * (1 if sy > 0 else -1) * 0.0), math.radians(lean * 1.0), 0)
    box(-0.15, 0, 0.9, 0.08, 1.0, 0.08, M['wood'])
    # long arm with counterweight + sling
    arm = cyl(0, 0, 0, 0.05, 1.7, M['wood'], verts=7)
    arm.location = (-0.15, 0, 0.95)
    arm.rotation_euler = (0, math.radians(-125), 0)
    box(-0.15 - 0.55, 0, 0.55, 0.3, 0.3, 0.3, M['stoneDark'])
    sphere(0.65, 0, 0.35, 0.09, M['rock'], seg=6, noise=0.2)
    cyl(0.45, 0, 0.35, 0.01, 0.6, M['canvas'], verts=4, ry=math.radians(-40))
    box(0.7, 0.45, 0.1, 0.5, 0.05, 0.05, M['wood'])
    for sy in (-0.45, 0.45):
        cyl(-0.6, sy, 0.08, 0.08, 0.05, M['iron'], verts=8, rx=math.pi / 2)
        cyl(0.6, sy, 0.08, 0.08, 0.05, M['iron'], verts=8, rx=math.pi / 2)
    flag(-0.7, -0.45, 0.1, 0.5, team)
    objs = _created[start:]
    root = bpy.data.objects.new(name + '_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.location = (x, y, z)
    root.rotation_euler = (0, 0, rz)
    for o in objs:
        o.parent = root
    return root

def bombard_model(x=0.0, y=0.0, z=0.0, rz=0.0, team='blue', name='bombard'):
    start = len(_created)
    box(0, 0, 0.16, 1.1, 0.5, 0.1, M['woodDark'])
    for sx in (-0.3, 0.3):
        for sy in (-0.3, 0.3):
            cyl(sx, sy, 0.15, 0.15, 0.06, M['wood'], verts=12, rx=math.pi / 2)
            cyl(sx, sy, 0.15, 0.05, 0.08, M['iron'], verts=8, rx=math.pi / 2)
    box(0, 0, 0.26, 0.7, 0.4, 0.14, M['wood'])
    cyl(-0.45, 0, 0.55, 0.16, 1.15, M['iron'], verts=14, r2=0.12, ry=math.radians(84))
    for t in (0.1, 0.35, 0.6):
        cyl(-0.45 + t, 0, 0.55, 0.17 - t * 0.06, 0.05, M['gold'], verts=14, ry=math.radians(84))
    box(-0.5, 0, 0.3, 0.1, 0.1, 0.3, M['woodDark'])
    for i in range(3):
        sphere(-0.35 + i * 0.13, -0.45, 0.07, 0.07, M['black'], seg=6)
    box(-0.05, 0.4, 0.0, 0.14, 0.14, 0.16, M['wood'])
    flag(-0.55, -0.22, 0.4, 0.45, team)
    objs = _created[start:]
    root = bpy.data.objects.new(name + '_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.location = (x, y, z)
    root.rotation_euler = (0, 0, rz)
    for o in objs:
        o.parent = root
    return root
_old_siege_building = m_siege

# ------------------------------------------------------------------ infantry / cavalry units
def build_unit3(kind, team, dir_i, frame):
    start = len(_created)
    stride = (0, 0.55, -0.55)[frame]
    bob = 0.0 if frame == 0 else 0.015
    hip = 0.40 + bob
    tm = team_mat(team)
    scale = 0.85
    if kind == 'trebuchet':
        trebuchet_model(0, 0, 0, 0, team)
        _finish_unit(start, dir_i, 0.85)
        return
    if kind == 'bombard':
        bombard_model(0, 0, 0, 0, team)
        _finish_unit(start, dir_i, 0.95)
        return
    if kind == 'paladin':
        horse(0, 0, 0, 0, 1.05, M['woodLight'] if team == 'blue' else M['woodDark'], stride * 0.8)
        # barding: plate on chest/head, caparison with white trim
        box(0.4, 0, 0.62, 0.22, 0.3, 0.3, M['metal'])
        box(0.7, 0, 1.08, 0.2, 0.14, 0.14, M['metal'])
        box(0, 0, 0.82, 0.62, 0.64, 0.06, tm)
        box(0.0, -0.33, 0.5, 0.62, 0.03, 0.34, tm)
        box(0.0, 0.33, 0.5, 0.62, 0.03, 0.34, tm)
        box(0.0, -0.345, 0.5, 0.64, 0.02, 0.05, M['white'])
        box(0.0, 0.345, 0.5, 0.64, 0.02, 0.05, M['white'])
        rz = 1.0
        cyl(0, 0, rz, 0.15, 0.36, M['metal'], verts=10, r2=0.12)
        cyl(0, 0, rz + 0.1, 0.16, 0.2, tm, verts=10, r2=0.13)
        sphere(0, 0, rz + 0.45, 0.11, M['metal'], seg=10, sz=1.05)
        cone(0, 0, rz + 0.53, 0.05, 0.28, tm, verts=6)
        for s in (-1, 1):
            sphere(0, s * 0.18, rz + 0.3, 0.07, M['metal'], seg=6)
            a = _arm(s, rz - 0.04, 0.0, M['metal']); a.location = (0.0, s * 0.17, rz + 0.3)
        cyl(-0.2, -0.2, rz + 0.05, 0.018, 1.6, M['wood'], verts=6, ry=math.radians(90 - 8))
        cone(1.35, -0.2, rz - 0.01, 0.035, 0.14, M['metal'], verts=5).rotation_euler = (0, math.radians(82), 0)
        box(0.1, 0.25, rz + 0.0, 0.03, 0.22, 0.4, tm)
        box(0.12, 0.25, rz + 0.05, 0.02, 0.04, 0.3, M['gold'])
        _finish_unit(start, dir_i, 0.92)
        return
    # ---- infantry skeleton
    def leg(y, sw, mat):
        o = cyl(0, 0, 0, 0.055, 0.40, mat, verts=8)
        o.data.transform(Matrix.Translation((0, 0, -0.40)))
        o.location = (0, y, hip)
        o.rotation_euler = (0, sw, 0)
    plate = kind in ('champion', 'arbalester')
    leg(-0.07, stride, M['metal'] if kind == 'champion' else M['iron'])
    leg(0.07, -stride, M['metal'] if kind == 'champion' else M['iron'])
    if kind == 'handcannon':
        body = M['hide']
    elif kind == 'arbalester':
        body = M['cloth2'] if team == 'blue' else M['red']
    else:
        body = M['metal']
    cyl(0, 0, hip, 0.145, 0.40, body, verts=10, r2=0.12)
    if kind in ('champion', 'arbalester'):
        cyl(0, 0, hip + 0.06, 0.16, 0.3, tm, verts=10, r2=0.135)        # surcoat
    cyl(0, 0, hip + 0.02, 0.15, 0.04, M['black'], verts=10)
    sphere(0, 0, hip + 0.47, 0.1, M['skin'], seg=10)
    sw = 0.45 * (stride / 0.55 if stride else 0)
    _arm(-1, hip, sw, M['metal'] if plate else M['skin'])
    _arm(1, hip, -sw, M['metal'] if plate else M['skin'])
    if kind == 'champion':
        for s in (-1, 1):
            sphere(0, s * 0.17, hip + 0.38, 0.075, M['metal'], seg=8)                # pauldrons
        sphere(0, 0, hip + 0.5, 0.125, M['metal'], seg=10, sz=1.0)
        box(0.1, 0, hip + 0.5, 0.02, 0.15, 0.05, M['black'])                          # visor slit
        cone(0, 0, hip + 0.6, 0.05, 0.3, tm, verts=6)                                 # plume
        # greatsword resting on the shoulder (two-handed)
        box(0.12, -0.16, hip - 0.12, 0.03, 0.025, 0.95, M['metal'])
        box(0.12, -0.16, hip + 0.1, 0.03, 0.2, 0.035, M['gold'])
        box(0.12, -0.16, hip - 0.18, 0.04, 0.04, 0.12, M['woodDark'])
    elif kind == 'arbalester':
        cyl(0, 0, hip + 0.53, 0.17, 0.02, M['metal'], verts=14)
        sphere(0, 0, hip + 0.52, 0.1, M['metal'], seg=10, sz=0.7)
        box(0.2, -0.15, hip + 0.18, 0.46, 0.045, 0.055, M['woodDark'])
        box(0.34, -0.15, hip + 0.2, 0.03, 0.38, 0.03, M['metal'])
        box(0.22, -0.15, hip + 0.235, 0.24, 0.012, 0.012, M['white'])
        box(-0.12, 0.05, hip + 0.1, 0.07, 0.08, 0.3, M['hide'])
    elif kind == 'handcannon':
        cyl(0, 0, hip + 0.52, 0.19, 0.02, tm, verts=14)                               # wide-brim hat
        sphere(0, 0, hip + 0.55, 0.11, tm, seg=10, sz=0.65)
        cyl(0.12, -0.12, hip + 0.12, 0.04, 0.6, M['iron'], verts=8, ry=math.radians(84))
        cyl(0.62, -0.12, hip + 0.12, 0.05, 0.04, M['gold'], verts=8, ry=math.radians(84))
        box(0.0, -0.12, hip + 0.1, 0.22, 0.05, 0.07, M['woodDark'])
        box(-0.14, 0.1, hip + 0.02, 0.05, 0.07, 0.12, M['white'])
    _finish_unit(start, dir_i, scale)
