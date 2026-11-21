# Walls, gates and traps (exec'd by gen_assets.py).  1x1 footprint, centred on the origin.
# mask bits (grid space): 1 = neighbour at +gx, 2 = -gx, 4 = +gy, 8 = -gy.   Blender: gx -> +X, gy -> -Y.
ARMS = ((1, 1, 0), (2, -1, 0), (4, 0, -1), (8, 0, 1))      # (bit, world dx, world dy)

def _arm_box(dx, dy, t0, t1, thick, z, h, mat):
    tc, ln = (t0 + t1) / 2, t1 - t0
    if dx:
        return box(dx * tc, 0, z, ln, thick, h, mat)
    return box(0, dy * tc, z, thick, ln, h, mat)

def _pt(dx, dy, t):
    return dx * t, dy * t

def _crenels(dx, dy, thick, z, mat, ts=(0.14, 0.36), size=0.1, h=0.1):
    for t in ts:
        px, py = _pt(dx, dy, t)
        box(px, py, z, size if dx else thick * 0.9, thick * 0.9 if dx else size, h, mat)

def m_wall(style, mask):
    if style == 0:                                   # palisade
        cyl(0, 0, 0, 0.085, 0.78, M['wood'], verts=7)
        cone(0, 0, 0.78, 0.085, 0.14, M['woodLight'], verts=7)
        for bit, dx, dy in ARMS:
            if not mask & bit:
                continue
            for k, t in enumerate((0.19, 0.36)):
                px, py = _pt(dx, dy, t)
                hh = 0.58 + 0.06 * ((k + (1 if dx else 0)) % 2)
                cyl(px, py, 0, 0.062, hh, M['wood'], verts=6)
                cone(px, py, hh, 0.062, 0.1, M['woodLight'], verts=6)
            _arm_box(dx, dy, 0.05, 0.5, 0.04, 0.2, 0.045, M['woodDark'])
            _arm_box(dx, dy, 0.05, 0.5, 0.04, 0.42, 0.045, M['woodDark'])
        if mask == 0:
            for sx, sy in ((-0.2, -0.2), (0.2, -0.2), (-0.2, 0.2), (0.2, 0.2)):
                cyl(sx, sy, 0, 0.06, 0.5, M['wood'], verts=6)
        return
    straight = mask in (3, 12)
    if style == 1:                                   # stone wall
        if not straight:
            box(0, 0, 0, 0.36, 0.36, 0.84, M['stone'])
            box(0, 0, 0.84, 0.42, 0.42, 0.06, M['stoneDark'])
        for bit, dx, dy in ARMS:
            if not mask & bit:
                continue
            _arm_box(dx, dy, 0.0 if straight else 0.15, 0.52, 0.28, 0, 0.68, M['stone'])
            _arm_box(dx, dy, 0.0 if straight else 0.15, 0.52, 0.33, 0.68, 0.05, M['stoneDark'])
            _crenels(dx, dy, 0.28, 0.73, M['stone'], ts=(0.1, 0.3) if straight else (0.14, 0.36))
        return
    # style 2 fortified / 3 imperial marble
    if not straight:
        box(0, 0, 0, 0.5, 0.5, 1.1, M['stone'])
        box(0, 0, 1.1, 0.58, 0.58, 0.08, M['stoneDark'])
        for sx in (-1, 1):
            for sy in (-1, 1):
                box(sx * 0.24, sy * 0.24, 1.18, 0.1, 0.1, 0.12, M['stone'])
        if style == 3:
            sphere(0, 0, 1.33, 0.08, M['gold'], seg=8)
        box(0.255, 0, 0.55, 0.03, 0.05, 0.22, M['black'])
        box(0, -0.255, 0.55, 0.05, 0.03, 0.22, M['black'])
    for bit, dx, dy in ARMS:
        if not mask & bit:
            continue
        _arm_box(dx, dy, 0.0 if straight else 0.2, 0.52, 0.38, 0, 0.9, M['stone'])
        _arm_box(dx, dy, 0.0 if straight else 0.2, 0.52, 0.46, 0.9, 0.07, M['stoneDark'])
        _crenels(dx, dy, 0.46, 0.97, M['stone'], ts=(0.1, 0.3, 0.46) if straight else (0.26, 0.44), size=0.1, h=0.13)
        px, py = _pt(dx, dy, 0.36)
        box(px + (0.2 if dy else 0), py + (0.2 if dx else 0), 0.0, 0.1, 0.1, 0.5, M['stoneDark'])      # buttress
        box(px, py, 0.5, 0.05 if dx else 0.07, 0.07 if dx else 0.05, 0.2, M['black'])

def m_gate(style, axis):
    dx, dy = (1, 0) if axis == 'x' else (0, 1)
    wood = style == 0
    pm = M['woodDark'] if wood else M['stone']
    h = 1.0 if wood else (1.15 if style == 1 else 1.3)
    for s in (-1, 1):
        px, py = dx * 0.32 * s, dy * 0.32 * s
        if wood:
            cyl(px, py, 0, 0.1, h, M['wood'], verts=8)
            cone(px, py, h, 0.1, 0.16, M['woodLight'], verts=8)
        else:
            box(px, py, 0, 0.34 if dx else 0.46, 0.46 if dx else 0.34, h, pm)
            box(px, py, h, 0.4 if dx else 0.52, 0.52 if dx else 0.4, 0.07, M['stoneDark'])
            for q in (-1, 1):
                box(px + (0.11 * q if dx else 0), py + (0.11 * q if dy else 0), h + 0.07, 0.09, 0.09, 0.12, M['stone'])
    # lintel over the door + the closed double door
    ln = 0.72
    if wood:
        box(0, 0, 0.78, ln if dx else 0.14, 0.14 if dx else ln, 0.12, M['wood'])
        box(0, 0, 0.0, 0.52 if dx else 0.09, 0.09 if dx else 0.52, 0.76, M['wood'])
    else:
        box(0, 0, 0.72, ln if dx else 0.34, 0.34 if dx else ln, 0.3, pm)
        _crenels(dx, dy, 0.34, 1.02, M['stone'], ts=(-0.25, 0.0, 0.25), size=0.1, h=0.12)
        box(0, 0, 0.0, 0.5 if dx else 0.09, 0.09 if dx else 0.5, 0.72, M['woodDark'])
    for z in (0.18, 0.5):
        box(0, 0, z, 0.52 if dx else 0.11, 0.11 if dx else 0.52, 0.045, M['iron'])
    cyl(0.0, 0.0, 0.38, 0.03, 0.05, M['gold'] if style == 3 else M['metal'], verts=8, rx=math.pi / 2 if dx else 0, ry=0 if dx else math.pi / 2)
    if style >= 2:
        flag(dx * 0.32, dy * 0.32, h + 0.13, 0.3, 'blue')

def m_spike():
    box(0, 0, 0, 0.84, 0.84, 0.05, M['dirt'])
    box(0, 0, 0.05, 0.7, 0.7, 0.03, M['woodDark'])
    rnd = random.Random(21)
    for i in range(3):
        for j in range(3):
            c = cone(-0.22 + i * 0.22, -0.22 + j * 0.22, 0.07, 0.045, 0.24, M['iron'], verts=6)
            c.rotation_euler = (rnd.uniform(-0.18, 0.18), rnd.uniform(-0.18, 0.18), 0)
    for q in (-1, 1):
        box(q * 0.38, 0, 0.04, 0.04, 0.78, 0.07, M['wood'])
        box(0, q * 0.38, 0.04, 0.78, 0.04, 0.07, M['wood'])

def m_oil():
    cyl(0, 0, 0, 0.4, 0.07, M['stoneDark'], verts=16)
    cyl(0, 0, 0.07, 0.34, 0.07, M['iron'], verts=16)
    cyl(0, 0, 0.12, 0.27, 0.03, M['black'], verts=16)
    cyl(0, 0, 0.14, 0.2, 0.02, M['fire'], verts=14)
    for i in range(3):
        a = i * 2.1
        cone(math.cos(a) * 0.1, math.sin(a) * 0.1, 0.15, 0.05, 0.16, M['fire'], verts=6)
    for i in range(4):
        a = i * math.pi / 2 + 0.4
        box(math.cos(a) * 0.3, math.sin(a) * 0.3, 0.07, 0.03, 0.1, 0.14, M['iron'], rz=a)

def m_wallrubble():
    rnd = random.Random(5)
    box(0, 0, 0, 0.8, 0.8, 0.03, M['char'])
    for i in range(7):
        box(rnd.uniform(-0.3, 0.3), rnd.uniform(-0.3, 0.3), 0.02, rnd.uniform(0.1, 0.28), rnd.uniform(0.07, 0.16), rnd.uniform(0.04, 0.16), M['stoneDark'] if i % 2 else M['char'], rz=rnd.uniform(0, 3.14))
    for i in range(2):
        cyl(rnd.uniform(-0.25, 0.25), rnd.uniform(-0.25, 0.25), 0.04, 0.025, 0.3, M['woodDark'], verts=5, ry=rnd.uniform(-1.2, 1.2), rz=rnd.uniform(0, 6.28))
