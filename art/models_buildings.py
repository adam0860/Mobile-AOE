# Building models (exec'd by gen_assets.py; shares its globals).  Footprint centred at origin, +X / -Y faces face the camera.

def door(x, y, face='x', w=0.26, h=0.5, mat=None):
    mat = mat or M['woodDark']
    if face == 'x':
        box(x, y, 0, 0.05, w, h, mat)
        box(x + 0.01, y, h - 0.02, 0.06, w + 0.1, 0.06, M['wood'])
    else:
        box(x, y, 0, w, 0.05, h, mat)
        box(x, y - 0.01, h - 0.02, w + 0.1, 0.06, 0.06, M['wood'])

def window(x, y, face='y', w=0.22, h=0.22):
    if face == 'y':
        box(x, y, 0, w, 0.05, h, M['black'])
        box(x - w / 2 - 0.05, y - 0.01, 0, 0.06, 0.07, h, M['wood'])
        box(x + w / 2 + 0.05, y - 0.01, 0, 0.06, 0.07, h, M['wood'])
    else:
        box(x, y, 0, 0.05, w, h, M['black'])

def beams(hx, hy, z0, z1, mat=None):
    mat = mat or M['wood']
    for sx in (-hx, hx):
        for sy in (-hy, hy):
            box(sx, sy, z0, 0.09, 0.09, z1 - z0, mat)
    box(0, -hy, z1 - 0.07, hx * 2 + 0.09, 0.07, 0.07, mat)
    box(hx, 0, z1 - 0.07, 0.07, hy * 2 + 0.09, 0.07, mat)
    box(0, -hy, (z0 + z1) / 2, hx * 2, 0.06, 0.05, mat)
    box(hx, 0, (z0 + z1) / 2, 0.06, hy * 2, 0.05, mat)

def barrel(x, y, z=0, r=0.1, h=0.2):
    cyl(x, y, z, r, h, M['wood'], verts=10, r2=r * 0.9)
    cyl(x, y, z + h * 0.25, r * 1.02, 0.025, M['iron'], verts=10)
    cyl(x, y, z + h * 0.7, r * 1.02, 0.025, M['iron'], verts=10)

def crate(x, y, z=0, s=0.18, rz=0.0):
    box(x, y, z, s, s, s, M['woodLight'], rz=rz)

def sack(x, y, z=0, r=0.11):
    sphere(x, y, z + r * 0.8, r, M['canvas'], sz=0.9, seg=8)

def log_pile(x, y, z=0, length=0.8, rows=(4, 3, 2), r=0.065, along='x'):
    for ri, n in enumerate(rows):
        for i in range(n):
            off = (i - (n - 1) / 2) * r * 2.05
            zz = z + r + ri * r * 1.8
            if along == 'x':
                cyl(x - length / 2, y + off, zz, r, length, M['bark'], verts=8, ry=math.pi / 2)
            else:
                cyl(x + off, y + length / 2, zz, r, length, M['bark'], verts=8, rx=math.pi / 2)

def banner_pole(x, y, h, team='blue'):
    flag(x, y, 0, h, team)

# ------------------------------------------------------------------ house (2x2)
def m_house(style):
    wall_h = 0.52
    if style:
        box(0, 0, 0, 1.5, 1.4, 0.34, M['stone'])
        box(0, 0, 0.34, 1.44, 1.34, wall_h, M['plaster'])
        beams(0.72, 0.67, 0.34, 0.34 + wall_h, M['woodDark'])
        prism_roof(0, 0, 0.34 + wall_h, 1.44, 1.34, 0.78, roof_for(style), over=0.2, along='y')
        box(-0.42, 0.3, 0.85, 0.2, 0.2, 0.75, M['stone'])
        box(-0.42, 0.3, 1.58, 0.26, 0.26, 0.06, M['stoneDark'])
        window(0.2, -0.675, 'y', 0.25, 0.25)
        door(0.72, 0.1, 'x', 0.3, 0.55)
        barrel(0.95, -0.4)
    else:
        box(0, 0, 0, 1.4, 1.3, 0.2, M['woodDark'])
        box(0, 0, 0.2, 1.34, 1.24, 0.5, M['plaster'])
        beams(0.66, 0.61, 0.0, 0.7)
        prism_roof(0, 0, 0.7, 1.34, 1.24, 0.72, M['thatch'], over=0.24, along='y')
        window(-0.2, -0.62, 'y', 0.22, 0.22)
        door(0.66, 0.0, 'x', 0.28, 0.5)
        sack(0.9, 0.35)
    # smoke-stack (dark age) / eave overhang
    if not style:
        cyl(-0.35, 0.25, 1.05, 0.07, 0.25, M['stoneDark'], verts=8)

# ------------------------------------------------------------------ town centre (3x3)
def m_townhall(level):
    if level >= 4:
        return m_townhall_big(level)
    if level == 1:
        palisade(0, 0, 1.38, 1.38, gap=('x', 1, 0.33))
        box(0, 0, 0, 1.9, 1.9, 0.25, M['woodDark'])
        box(0, 0, 0.25, 1.82, 1.82, 0.62, M['plaster'])
        beams(0.89, 0.89, 0.0, 0.87)
        hip_roof(0, 0, 0.87, 1.82, 1.82, 1.15, M['thatch'], over=0.28, top=0.35)
        cyl(0, 0, 2.0, 0.07, 0.3, M['woodDark'], verts=6)
        door(0.91, 0, 'x', 0.42, 0.62)
        window(-0.4, -0.91, 'y', 0.26, 0.26)
        window(0.4, -0.91, 'y', 0.26, 0.26)
        banner_pole(1.05, -1.05, 1.7, 'blue')
        barrel(1.15, 0.6)
        barrel(1.3, 0.45)
        crate(1.2, -0.5, 0, 0.22, 0.3)
    elif level == 2:
        palisade(0, 0, 1.4, 1.4, gap=('x', 1, 0.36))
        box(0, 0, 0, 1.95, 1.95, 0.3, M['stoneDark'])
        box(0, 0, 0.3, 1.86, 1.86, 0.68, M['plaster'])
        beams(0.91, 0.91, 0.3, 0.98, M['woodDark'])
        hip_roof(0, 0, 0.98, 1.86, 1.86, 1.2, M['thatch'], over=0.3, top=0.3)
        # side annex
        box(-0.45, 0.95, 0, 0.9, 0.5, 0.55, M['plaster'])
        prism_roof(-0.45, 0.95, 0.55, 0.9, 0.5, 0.4, M['thatch'], over=0.15, along='x')
        cyl(0, 0, 2.18, 0.07, 0.4, M['woodDark'], verts=6)
        door(0.93, 0, 'x', 0.46, 0.7)
        window(-0.45, -0.93, 'y', 0.28, 0.3)
        window(0.45, -0.93, 'y', 0.28, 0.3)
        banner_pole(1.1, -1.1, 1.9, 'blue')
        banner_pole(1.1, 1.1, 1.5, 'blue')
        barrel(1.2, 0.7)
        sack(1.25, -0.5)
        crate(1.3, 0.45, 0, 0.22, 0.3)
        log_pile(-1.1, -0.9, 0, 0.7, (3, 2), 0.06, 'y')
    else:
        # Feudal: stone keep with four round towers
        box(0, 0, 0, 1.9, 1.9, 0.4, M['stone'])
        box(0, 0, 0.4, 1.7, 1.7, 0.72, M['plaster'])
        beams(0.82, 0.82, 0.4, 1.12, M['woodDark'])
        hip_roof(0, 0, 1.12, 1.7, 1.7, 0.95, M['tile'], over=0.22, top=0.3)
        cyl(0, 0, 2.05, 0.05, 0.4, M['woodDark'], verts=6)
        flag(0, 0, 2.05, 0.4, 'blue')
        for sx in (-1, 1):
            for sy in (-1, 1):
                cyl(sx * 1.08, sy * 1.08, 0, 0.3, 1.25, M['stone'], verts=14, r2=0.28)
                cyl(sx * 1.08, sy * 1.08, 1.25, 0.34, 0.1, M['stoneDark'], verts=14)
                cone(sx * 1.08, sy * 1.08, 1.35, 0.42, 0.62, M['tile'], verts=14)
                box(sx * 1.08 + (-0.3 if sx > 0 else 0.3) * 0, sy * 1.08, 0.7, 0.05, 0.05, 0.14, M['black'])
        for sx, sy in ((-1, -1), (1, -1)):
            flag(sx * 1.08, sy * 1.08, 1.95, 0.3, 'blue')
        # curtain walls with a gate on +X
        for sy in (-1, 1):
            box(0, sy * 1.08, 0, 1.4, 0.16, 0.5, M['stone'])
        box(-1.08, 0, 0, 0.16, 1.4, 0.5, M['stone'])
        for sy in (-1, 1):
            box(1.08, sy * 0.58, 0, 0.16, 0.7, 0.5, M['stone'])
        box(1.08, 0, 0.5, 0.16, 0.5, 0.16, M['stone'])
        box(1.1, 0, 0, 0.06, 0.34, 0.5, M['woodDark'])
        door(0.86, 0, 'x', 0.4, 0.6)
        window(-0.4, -0.86, 'y', 0.26, 0.3)
        window(0.4, -0.86, 'y', 0.26, 0.3)

# ------------------------------------------------------------------ lumber camp / mining camps (2x2)
def shed(style, roof_h=0.42, hx=0.72, hy=0.62, post_h=0.7):
    roof = roof_for(style)
    for sx in (-hx, hx):
        for sy in (-hy, hy):
            cyl(sx, sy, 0, 0.06, post_h, M['woodDark'], verts=7)
    box(0, 0, post_h - 0.05, hx * 2 + 0.1, 0.07, 0.07, M['wood'])
    prism_roof(0, 0, post_h, hx * 2, hy * 2, roof_h, roof, over=0.18, along='x')
    if style:
        box(0, 0, 0, hx * 2 + 0.08, hy * 2 + 0.08, 0.1, M['stone'])
    else:
        box(0, 0, 0, hx * 2 + 0.05, hy * 2 + 0.05, 0.05, M['dirt'])

def m_lumber(style):
    shed(style)
    log_pile(0.0, 0.0, 0.05, 0.9, (4, 3, 2), 0.055, 'x')
    log_pile(1.0, -0.2, 0.0, 0.6, (3, 2), 0.06, 'y')
    cyl(-0.95, -0.55, 0, 0.14, 0.24, M['bark'], verts=10)
    box(-0.95, -0.55, 0.24, 0.02, 0.02, 0.02, M['wood'])
    cyl(-0.95, -0.45, 0.24, 0.016, 0.42, M['wood'], verts=6, rx=math.radians(70))
    box(-0.95, -0.2, 0.55, 0.05, 0.14, 0.09, M['metal'])
    box(1.1, 0.5, 0.0, 0.5, 0.18, 0.04, M['woodLight'])
    box(1.1, 0.5, 0.04, 0.46, 0.16, 0.04, M['woodLight'])
    box(1.1, 0.5, 0.08, 0.42, 0.14, 0.04, M['woodLight'])

def m_mine(style, kind):
    shed(style, roof_h=0.38)
    if kind == 'gold':
        rnd = random.Random(4)
        for i in range(9):
            sphere(0.95 + rnd.uniform(-0.18, 0.18), 0.1 + rnd.uniform(-0.3, 0.3), 0.04, 0.09 + rnd.uniform(0, 0.05), M['gold'], seg=6, noise=0.25, seed=i)
        sphere(0.1, 0.0, 0.03, 0.2, M['rock'], seg=8, noise=0.2, seed=3)
        for i in range(4):
            sphere(0.1 + rnd.uniform(-0.1, 0.1), rnd.uniform(-0.1, 0.1), 0.12, 0.06, M['gold'], seg=6, noise=0.3, seed=i + 20)
    else:
        rnd = random.Random(9)
        for i in range(7):
            sphere(0.95 + rnd.uniform(-0.2, 0.2), 0.05 + rnd.uniform(-0.3, 0.3), 0.04, 0.1 + rnd.uniform(0, 0.08), M['rock'], seg=6, noise=0.3, seed=i)
        sphere(0.05, 0.05, 0.05, 0.22, M['stoneDark'], seg=8, noise=0.25, seed=5)
    # cart
    box(-0.1, -0.85, 0.12, 0.5, 0.3, 0.16, M['wood'])
    for sx in (-0.2, 0.0):
        cyl(sx, -1.02, 0.1, 0.1, 0.03, M['woodDark'], verts=10, rx=math.pi / 2)
        cyl(sx, -0.68, 0.1, 0.1, 0.03, M['woodDark'], verts=10, rx=math.pi / 2)
    # pickaxe
    cyl(-0.85, -0.45, 0.0, 0.015, 0.55, M['wood'], verts=6, ry=math.radians(18))
    box(-0.75, -0.45, 0.5, 0.2, 0.03, 0.03, M['metal'], rz=0)

# ------------------------------------------------------------------ farm (2x2)
def m_farm(style):
    box(0, 0, 0, 1.9, 1.9, 0.07, M['soil'])
    mat = M['wheat'] if style else M['crop']
    for i in range(8):
        y = -0.78 + i * 0.22
        h = 0.13 + (0.04 if i % 2 else 0)
        box(0, y, 0.06, 1.7, 0.11, h, mat)
    for sx in (-0.95, 0.95):
        for sy in (-0.95, 0.95):
            cyl(sx, sy, 0, 0.04, 0.3, M['wood'], verts=6)
    box(0, -0.95, 0.15, 1.9, 0.03, 0.03, M['wood'])
    box(0.95, 0, 0.15, 0.03, 1.9, 0.03, M['wood'])
    # scarecrow
    cyl(-0.55, 0.3, 0.05, 0.02, 0.6, M['wood'], verts=6)
    box(-0.55, 0.3, 0.45, 0.02, 0.4, 0.03, M['wood'])
    sphere(-0.55, 0.3, 0.7, 0.07, M['hay'], seg=6)
    cone(-0.55, 0.3, 0.74, 0.1, 0.1, M['thatch'], verts=8)

# ------------------------------------------------------------------ barracks (3x3)
def m_barracks(style):
    wall = M['plaster']
    roof = roof_for(style)
    box(0, 0, 0, 2.3, 1.7, 0.35, M['stone'] if style else M['woodDark'])
    box(0, 0, 0.35, 2.2, 1.6, 0.62, wall)
    beams(1.08, 0.78, 0.35, 0.97, M['woodDark'] if style else M['wood'])
    prism_roof(0, 0, 0.97, 2.2, 1.6, 0.95, roof, over=0.22, along='x', ridge_ext=0.05)
    door(1.1, 0.0, 'x', 0.4, 0.62)
    window(-0.5, -0.8, 'y', 0.25, 0.28)
    window(0.5, -0.8, 'y', 0.25, 0.28)
    # shields on the front wall
    for i, t in enumerate(('blue', 'wood', 'blue')):
        cyl(-0.65 + i * 0.5, -0.83, 0.45, 0.14, 0.04, team_mat('blue') if t == 'blue' else M['woodLight'], verts=12, rx=math.pi / 2)
    # weapon rack + dummy
    box(1.15, -1.0, 0, 0.75, 0.06, 0.06, M['woodDark'])
    cyl(0.85, -1.0, 0, 0.03, 0.45, M['woodDark'], verts=6)
    cyl(1.45, -1.0, 0, 0.03, 0.45, M['woodDark'], verts=6)
    for i in range(4):
        cyl(0.95 + i * 0.15, -1.0, 0.05, 0.012, 0.7, M['wood'], verts=5, ry=math.radians(-6))
        cone(0.95 + i * 0.15 - 0.04, -1.0, 0.75, 0.025, 0.07, M['metal'], verts=5)
    cyl(1.35, 0.95, 0, 0.04, 0.6, M['wood'], verts=6)
    box(1.35, 0.95, 0.4, 0.3, 0.04, 0.04, M['wood'])
    sphere(1.35, 0.95, 0.62, 0.09, M['canvas'], seg=6)
    banner_pole(-1.05, 0.95, 1.5, 'blue')
    barrel(-1.15, -0.95)

# ------------------------------------------------------------------ army camp (3x3)
def tent(x, y, size=0.5, h=0.55, rz=0.0, mat=None):
    mat = mat or M['canvas']
    box(x, y, 0, size * 2, size * 1.6, 0.18, mat, rz=rz)
    o = hip_roof(x, y, 0.18, size * 2, size * 1.6, h, mat, over=0.04, top=0.0)
    o.rotation_euler = (0, 0, rz)
    cyl(x, y, 0.18 + h, 0.012, 0.15, M['woodDark'], verts=5)

def m_armycamp(style):
    tent(-0.75, -0.55, 0.5, 0.6, 0.1)
    tent(0.35, -0.9, 0.5, 0.55, -0.1, M['hide'])
    tent(-0.5, 0.75, 0.55, 0.62, 0.0)
    tent(0.9, 0.55, 0.45, 0.5, 0.2, M['hide'])
    # fire
    for i in range(7):
        a = i / 7 * math.pi * 2
        sphere(0.2 + math.cos(a) * 0.17, 0.05 + math.sin(a) * 0.17, 0, 0.05, M['rock'], seg=6)
    cone(0.2, 0.05, 0.02, 0.1, 0.25, M['fire'], verts=7)
    log_pile(0.2, -0.25, 0, 0.35, (2,), 0.04, 'x')
    banner_pole(1.2, -1.2, 1.4, 'blue')
    box(-1.1, -1.1, 0, 0.5, 0.06, 0.06, M['woodDark'])
    for i in range(3):
        cyl(-1.25 + i * 0.15, -1.1, 0.03, 0.012, 0.6, M['wood'], verts=5)
    crate(0.95, -0.15, 0, 0.2, 0.3)
    barrel(-1.2, 0.2)

# ------------------------------------------------------------------ archery range (3x3)
def target(x, y, rz=0.0):
    cyl(x, y, 0, 0.02, 0.5, M['wood'], verts=6)
    cyl(x - 0.18, y, 0.02, 0.02, 0.5, M['wood'], verts=6, ry=math.radians(18))
    for k, (r, mat) in enumerate(((0.3, 'white'), (0.23, 'black'), (0.17, 'blue'), (0.11, 'red'), (0.05, 'gold'))):
        cyl(x + 0.0, y, 0.5, r, 0.02 + k * 0.004, M[mat], verts=16, ry=math.radians(90))
    return

def m_archery(style):
    roof = roof_for(style)
    for sx in (-0.9, 0.9):
        for sy in (-0.7, 0.7):
            cyl(sx, sy, 0, 0.07, 0.8, M['woodDark'], verts=8)
    box(0, -0.7, 0.75, 1.9, 0.08, 0.08, M['wood'])
    box(0, 0.7, 0.75, 1.9, 0.08, 0.08, M['wood'])
    prism_roof(0, 0, 0.8, 1.8, 1.4, 0.55, roof, over=0.22, along='x')
    box(0, 0, 0, 2.0, 1.6, 0.06, M['dirt'])
    # bows rack & hay bales
    box(0.0, 0.62, 0, 1.2, 0.08, 0.5, M['woodLight'])
    for i in range(3):
        cyl(-0.4 + i * 0.4, 0.58, 0.1, 0.02, 0.4, M['wood'], verts=5, ry=math.radians(8))
    for x in (-0.3, 0.1):
        box(x, -0.4, 0.06, 0.34, 0.2, 0.18, M['hay'])
    for k, (px, py) in enumerate(((1.35, -0.9), (1.4, 0.0), (1.35, 0.9))):
        target(px, py)
    banner_pole(-1.2, -1.15, 1.2, 'blue')

# ------------------------------------------------------------------ towers (2x2)
def m_tower(level):
    if level == 1:
        for sx in (-0.4, 0.4):
            for sy in (-0.4, 0.4):
                cyl(sx * 1.0, sy * 1.0, 0, 0.07, 1.45, M['woodDark'], verts=7, r2=0.06)
        for z in (0.5, 0.95):
            for sx in (-0.4, 0.4):
                box(sx, 0, z, 0.05, 0.85, 0.05, M['wood'])
            for sy in (-0.4, 0.4):
                box(0, sy, z + 0.2, 0.85, 0.05, 0.05, M['wood'])
        box(0, 0, 1.25, 1.0, 1.0, 0.08, M['woodLight'])
        for sx in (-0.45, 0.45):
            box(sx, 0, 1.33, 0.05, 1.0, 0.28, M['wood'])
        for sy in (-0.45, 0.45):
            box(0, sy, 1.33, 1.0, 0.05, 0.28, M['wood'])
        hip_roof(0, 0, 1.62, 0.92, 0.92, 0.45, M['thatch'], over=0.14, top=0.0)
        for sy in (-0.4, 0.4):
            cyl(0.0, sy, 1.33, 0.03, 0.3, M['woodDark'], verts=5)
        box(0.5, 0, 0, 0.04, 0.05, 1.3, M['wood'])
        banner_pole(0.45, -0.45, 2.1, 'blue') if False else None
    elif level == 2:
        cyl(0, 0, 0, 0.5, 1.45, M['stone'], verts=16, r2=0.44)
        cyl(0, 0, 1.45, 0.56, 0.14, M['stoneDark'], verts=16)
        for i in range(8):
            a = i / 8 * math.pi * 2
            box(math.cos(a) * 0.52, math.sin(a) * 0.52, 1.59, 0.16, 0.16, 0.16, M['stone'], rz=a)
        cone(0, 0, 1.59, 0.55, 0.62, M['slate'], verts=16)
        for a in (0.2, 1.2, 4.4):
            box(math.cos(a) * 0.46, math.sin(a) * 0.46, 0.85, 0.06, 0.06, 0.2, M['black'], rz=a)
        door(0.48, 0, 'x', 0.28, 0.4, M['woodDark'])
        flag(0, 0, 2.2, 0.35, 'blue')
    else:
        cyl(0, 0, 0, 0.58, 1.9, M['stone'], verts=18, r2=0.5)
        cyl(0, 0, 1.9, 0.7, 0.18, M['stoneDark'], verts=18)
        for i in range(10):
            a = i / 10 * math.pi * 2
            box(math.cos(a) * 0.66, math.sin(a) * 0.66, 2.08, 0.18, 0.2, 0.2, M['stone'], rz=a)
        cone(0, 0, 2.08, 0.62, 0.8, M['tile'], verts=18)
        for a in (0.1, 1.0, 2.2, 4.2, 5.3):
            box(math.cos(a) * 0.55, math.sin(a) * 0.55, 1.2, 0.07, 0.07, 0.26, M['black'], rz=a)
            box(math.cos(a) * 0.55, math.sin(a) * 0.55, 0.55, 0.06, 0.06, 0.2, M['black'], rz=a)
        door(0.56, 0, 'x', 0.3, 0.45, M['woodDark'])
        flag(0, 0, 2.88, 0.45, 'blue')

# ------------------------------------------------------------------ enemy-only buildings
def m_storehouse():
    box(0, 0, 0, 1.5, 1.3, 0.55, M['wood'])
    prism_roof(0, 0, 0.55, 1.5, 1.3, 0.65, M['thatch'], over=0.2, along='y')
    door(0.75, 0, 'x', 0.4, 0.5)
    sack(0.95, -0.5)
    sack(0.95, -0.7)
    barrel(0.95, 0.55)
    crate(-0.2, -0.78, 0, 0.22, 0.2)

def m_tent():
    cone(0, 0, 0, 0.85, 1.05, M['hide'], verts=10)
    cyl(0, 0, 0.9, 0.018, 0.4, M['woodDark'], verts=5, ry=0.5)
    cyl(0, 0, 0.9, 0.018, 0.4, M['woodDark'], verts=5, ry=-0.5)
    box(0.8, 0, 0, 0.05, 0.28, 0.5, M['black'])
    log_pile(0.0, -0.9, 0, 0.5, (2,), 0.04, 'x')

def m_chief():
    palisade(0, 0, 1.4, 1.4, gap=('x', 1, 0.3), mat=M['woodDark'], hgt=0.7)
    box(0, 0, 0, 1.9, 1.9, 0.5, M['woodDark'])
    hip_roof(0, 0, 0.5, 1.9, 1.9, 1.0, M['hide'], over=0.25, top=0.2)
    door(0.95, 0, 'x', 0.45, 0.55, M['black'])
    cyl(1.1, -1.0, 0, 0.04, 1.6, M['bark'], verts=6)
    for k in range(3):
        sphere(1.1, -1.0, 1.0 + k * 0.2, 0.09, M['white'], seg=7)
    flag(-1.05, -1.0, 0, 1.4, 'red')
    box(1.1, 0.7, 0, 0.3, 0.3, 0.15, M['rock'])
    cone(1.1, 0.7, 0.1, 0.12, 0.3, M['fire'], verts=7)

# ------------------------------------------------------------------ construction scaffold / rubble
def m_scaffold(n):
    h = 0.5 + 0.12 * n
    ext = n * 0.5 - 0.1
    for sx in (-ext, ext):
        for sy in (-ext, ext):
            cyl(sx, sy, 0, 0.05, h, M['woodLight'], verts=6)
    for z in (h * 0.4, h * 0.85):
        box(0, -ext, z, ext * 2, 0.045, 0.045, M['wood'])
        box(ext, 0, z, 0.045, ext * 2, 0.045, M['wood'])
        box(0, ext, z, ext * 2, 0.045, 0.045, M['wood'])
        box(-ext, 0, z, 0.045, ext * 2, 0.045, M['wood'])
    box(0, 0, 0, ext * 1.4, ext * 1.4, 0.08, M['dirt'])
    box(0.1, 0, 0.08, ext * 1.2, ext * 0.4, 0.12, M['plaster'])
    for i in range(4):
        box(-ext * 0.3, -ext * 0.4 + i * 0.03, 0.08 + i * 0.04, ext * 0.9, 0.22, 0.04, M['woodLight'])
    cyl(ext * 0.7, ext * 0.6, 0, 0.1, 0.18, M['wood'], verts=8)

def m_rubble(n):
    rnd = random.Random(n)
    ext = n * 0.45
    box(0, 0, 0, ext * 2, ext * 2, 0.04, M['char'])
    for i in range(10 * n):
        box(rnd.uniform(-ext, ext), rnd.uniform(-ext, ext), 0.02, rnd.uniform(0.1, 0.4), rnd.uniform(0.06, 0.2), rnd.uniform(0.05, 0.22), M['char'], rz=rnd.uniform(0, 3.14))
    for i in range(4 * n):
        cyl(rnd.uniform(-ext, ext), rnd.uniform(-ext, ext), 0.05, 0.03, rnd.uniform(0.3, 0.7), M['woodDark'], verts=5, ry=rnd.uniform(-1.2, 1.2), rz=rnd.uniform(0, 6.28))
    for i in range(3 * n):
        sphere(rnd.uniform(-ext, ext) * 0.8, rnd.uniform(-ext, ext) * 0.8, 0.06, 0.05, M['fire'], seg=5)
