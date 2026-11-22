# Resource camps (exec'd by gen_assets.py, overrides the older m_lumber / m_mine).
# Modelled on the classic RTS look: an open timber shed with a plank roof and blue trim, cut stone / ingot stacks, a cart,
# a log crane.  Each camp has its own colour theme so they read at a glance:
#   lumber = warm brown logs + tall crane + sawdust,  stone = cool grey blocks on gravel,  gold = bright yellow ingots on sand.

def _croof_mat(style):
    return M['woodLight'] if style < 2 else M['slate']

def _cgroup(x, y, z, rot=(0, 0, 0)):
    root = bpy.data.objects.new('camp_part', None)
    scene.collection.objects.link(root); _created.append(root)
    root.location = (x, y, z); root.rotation_euler = rot
    return root

def _kid(o, parent):
    o.parent = parent
    return o

def _shed(cx, cy, w, d, style, hgt=0.8):
    """open-sided lean-to: four posts, a sloping plank roof with blue trim, a back wall."""
    for sx in (-1, 1):
        for sy in (-1, 1):
            cyl(cx + sx * w / 2, cy + sy * d / 2, 0, 0.055, hgt + (0.04 if sx < 0 else -0.06), M['woodDark'], verts=6)
    box(cx - w / 2, cy, 0.06, 0.05, d, hgt * 0.78, M['wood'])                       # back wall planks
    for i in range(4):
        box(cx - w / 2 + 0.03, cy, 0.1 + i * hgt * 0.19, 0.02, d * 0.98, 0.012, M['woodDark'])
    box(cx + w / 2, cy, hgt - 0.07, 0.06, d + 0.04, 0.06, M['woodDark'])           # front beam
    box(cx - w / 2, cy, hgt - 0.01, 0.06, d + 0.04, 0.06, M['woodDark'])
    roof = _cgroup(cx, cy, hgt + 0.03, (0, 0.13, 0))
    sx, sy = w + 0.2, d + 0.2
    _kid(box(0, 0, 0, sx, sy, 0.07, _croof_mat(style)), roof)
    for k in range(5):                                                               # plank lines
        _kid(box(-sx / 2 + (k + 1) * sx / 6, 0, 0.065, 0.014, sy * 0.98, 0.012, M['woodDark']), roof)
    for ey in (-1, 1):
        _kid(box(0, ey * (sy / 2 - 0.02), 0.0, sx + 0.02, 0.06, 0.12, M['blue']), roof)       # blue side trim
    _kid(box(sx / 2 - 0.02, 0, 0.0, 0.06, sy + 0.02, 0.12, M['blue']), roof)
    _kid(box(-sx / 2 + 0.02, 0, 0.0, 0.06, sy + 0.02, 0.12, M['blue']), roof)

def _blocks(x, y, rows, z=0.04, bw=0.34, bd=0.22, bh=0.15, mat=None, rz=0.0, seed=0):
    rnd = random.Random(seed)
    for r, n in enumerate(rows):
        for i in range(n):
            ox = (i - (n - 1) / 2) * (bw + 0.015)
            m = mat or (M['stone'] if (i + r) % 2 == 0 else M['stoneDark'])
            box(x + ox * math.cos(rz), y + ox * math.sin(rz), z + r * bh, bw, bd, bh, m, rz=rz + rnd.uniform(-0.06, 0.06))

def _wheelbarrow(x, y, rz, load):
    root = _cgroup(x, y, 0, (0, 0, rz))
    _kid(box(0, 0, 0.17, 0.42, 0.28, 0.09, M['wood']), root)
    for s in (-1, 1):
        _kid(box(0.0, s * 0.15, 0.2, 0.42, 0.025, 0.14, M['woodLight']), root)
        c = _kid(cyl(-0.34, s * 0.11, 0.2, 0.014, 0.4, M['wood'], verts=5, ry=math.radians(80)), root)
    _kid(cyl(0.3, 0, 0.12, 0.11, 0.05, M['woodDark'], verts=10, rx=math.pi / 2), root)
    _kid(cyl(0.3, 0, 0.12, 0.03, 0.08, M['iron'], verts=6, rx=math.pi / 2), root)
    for s in (-1, 1):
        _kid(box(0.05, s * 0.12, 0.0, 0.03, 0.03, 0.17, M['wood']), root)
    for k, (lx, ly, r, mt) in enumerate(load):
        _kid(sphere(lx, ly, 0.3 + (k % 2) * 0.03, r, mt, seg=6, noise=0.3, seed=k + 20, sz=0.8), root)

def _cart(x, y, rz, load, mat_load):
    root = _cgroup(x, y, 0, (0, 0, rz))
    _kid(box(0, 0, 0.12, 0.62, 0.42, 0.22, M['wood']), root)
    for s in (-1, 1):
        _kid(box(0, s * 0.215, 0.12, 0.62, 0.025, 0.24, M['woodLight']), root)
        _kid(cyl(0.0, s * 0.26, 0.14, 0.14, 0.035, M['woodDark'], verts=12, rx=math.pi / 2), root)
        _kid(cyl(0.0, s * 0.275, 0.14, 0.04, 0.04, M['iron'], verts=6, rx=math.pi / 2), root)
    _kid(cyl(0.5, 0, 0.1, 0.02, 0.5, M['wood'], verts=6, ry=math.radians(84)), root)
    rnd = random.Random(7)
    for k in range(load):
        _kid(sphere(rnd.uniform(-0.2, 0.2), rnd.uniform(-0.12, 0.12), 0.34 + rnd.uniform(0, 0.07), rnd.uniform(0.06, 0.1), mat_load, seg=6, noise=0.3, seed=k + 5, sz=0.85), root)

def _camp_border(style):
    if style >= 2:                                                                    # stone edging from the Castle Age on
        for s in (-1, 1):
            box(0, s * 0.96, 0.0, 1.96, 0.1, 0.09, M['stone'])
            box(s * 0.96, 0, 0.0, 0.1, 1.96, 0.09, M['stone'])

# ------------------------------------------------------------------ stone camp: grey
def _stonecamp(style):
    box(0, 0, 0, 1.95, 1.95, 0.04, M['pebble'])
    _camp_border(style)
    _shed(-0.58, -0.58, 0.85, 0.85, style, 0.72)
    _blocks(-0.58, -0.58, (2, 2), z=0.04, rz=0.0, seed=1)                       # cut blocks stored under the shed
    box(-0.5, -0.98, 0.04, 0.5, 0.16, 0.1, M['stone'])
    _blocks(0.5, -0.45, (4, 3, 2, 1), z=0.04, rz=0.0, seed=2)                 # big stack of cut stone
    _blocks(0.62, 0.45, (3, 2), z=0.04, bw=0.4, bd=0.28, bh=0.12, rz=math.radians(20), seed=3)
    _wheelbarrow(0.45, 0.05, math.radians(25), [(0.05, 0.0, 0.1, M['rock']), (-0.08, 0.05, 0.08, M['rock']), (0.0, -0.06, 0.09, M['stoneDark'])])
    rnd = random.Random(11)
    for i in range(10):                                                          # grey boulders and rubble
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0.0, 0.5)
        sphere(0.1 + math.cos(a) * d, 0.75 + math.sin(a) * d * 0.5, 0.06, rnd.uniform(0.09, 0.2), M['rock'], seg=7, noise=0.3, seed=i + 60, sz=0.8)
    for i in range(5):
        sphere(-0.9 + rnd.uniform(0, 0.4), 0.7 + rnd.uniform(-0.2, 0.2), 0.05, rnd.uniform(0.07, 0.13), M['rock'], seg=6, noise=0.3, seed=i + 80, sz=0.8)
    cyl(0.85, 0.85, 0.0, 0.016, 0.6, M['wood'], verts=6, ry=math.radians(20))   # pickaxe
    box(0.95, 0.85, 0.55, 0.22, 0.03, 0.03, M['iron'])

# ------------------------------------------------------------------ gold camp: yellow
def _goldcamp(style):
    box(0, 0, 0, 1.95, 1.95, 0.04, M['sand'])
    _camp_border(style)
    _shed(-0.58, -0.58, 0.85, 0.85, style, 0.72)
    gm = M['gold']
    for r, n in enumerate((3, 2, 1)):                                          # ingot pyramid under the shed
        for i in range(n):
            box(-0.58 + (i - (n - 1) / 2) * 0.22, -0.58, 0.04 + r * 0.075, 0.2, 0.12, 0.07, gm)
    for r, n in enumerate((5, 4, 3, 2, 1)):                                       # bigger ingot stack in front
        for i in range(n):
            box(0.55, -0.55 + (i - (n - 1) / 2) * 0.17, 0.04 + r * 0.08, 0.3, 0.15, 0.075, gm, rz=0.0)
    _cart(0.45, 0.35, math.radians(-15), 16, gm)
    rnd = random.Random(21)
    for i in range(34):                                                           # nugget heap
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0.0, 0.38)
        sphere(0.55 + math.cos(a) * d, -0.05 + math.sin(a) * d * 0.8 + 0.85 * 0, 0.05 + max(0, 0.25 - d) * 0.5, rnd.uniform(0.05, 0.09), gm, seg=6, noise=0.35, seed=i + 90, sz=0.8)
    for i in range(14):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0.0, 0.35)
        sphere(-0.2 + math.cos(a) * d, 0.75 + math.sin(a) * d * 0.6, 0.05, rnd.uniform(0.05, 0.1), gm, seg=6, noise=0.35, seed=i + 120, sz=0.8)
    sphere(-0.9, 0.62, 0.07, 0.11, M['rock'], seg=6, noise=0.3, seed=3, sz=0.8)
    cyl(0.9, 0.9, 0.0, 0.016, 0.6, M['wood'], verts=6, ry=math.radians(20))
    box(1.0, 0.9, 0.55, 0.22, 0.03, 0.03, M['iron'])

# ------------------------------------------------------------------ lumber camp: brown, tall crane, big log piles
def _logs(x, y, rows, along='y', r=0.075, length=0.95, z=0.04):
    for ri, n in enumerate(rows):
        for i in range(n):
            off = (i - (n - 1) / 2) * r * 2.05; zz = z + r + ri * r * 1.75
            if along == 'y':
                cyl(x + off, y - length / 2, zz, r, length, M['bark'], verts=9, rx=-math.pi / 2)
                cyl(x + off, y + length / 2 - 0.001, zz, r * 0.93, 0.012, M['woodLight'], verts=9, rx=-math.pi / 2)
                cyl(x + off, y - length / 2 + 0.001, zz, r * 0.93, 0.012, M['woodLight'], verts=9, rx=-math.pi / 2)
            else:
                cyl(x - length / 2, y + off, zz, r, length, M['bark'], verts=9, ry=math.pi / 2)
                cyl(x + length / 2 - 0.012, y + off, zz, r * 0.93, 0.012, M['woodLight'], verts=9, ry=math.pi / 2)
                cyl(x - length / 2 + 0.001, y + off, zz, r * 0.93, 0.012, M['woodLight'], verts=9, ry=math.pi / 2)

def _lumbercamp(style):
    box(0, 0, 0, 1.95, 1.95, 0.04, M['dirt'])
    box(-0.1, 0.0, 0.045, 1.2, 1.1, 0.012, M['hay'])                              # sawdust
    _camp_border(style)
    # log crane (two leaning posts, cross beam, blue cloth, hanging log)
    for s in (-1, 1):
        cyl(-0.55, -0.55 + s * 0.28, 0.04, 0.05, 1.45, M['woodLight'], verts=7, rx=s * 0.12, ry=0.1)
    box(-0.5, -0.55, 1.35, 0.1, 0.9, 0.08, M['wood'])
    for s in (-1, 1):
        box(-0.5, -0.55 + s * 0.38, 1.0, 0.04, 0.05, 0.4, M['woodDark'])
    box(-0.43, -0.55, 0.78, 0.03, 0.62, 0.5, M['blue'])                               # blue cloth
    cyl(-0.4, -0.55, 0.6, 0.01, 0.78, M['black'], verts=4)
    cyl(-0.4, -0.55 - 0.25, 0.32, 0.07, 0.5, M['bark'], verts=9, rx=math.pi / 2)    # log on the rope
    cyl(-0.4, -0.3, 0.3, 0.007, 1.0, M['black'], verts=4)
    # big log piles (end caps pale) in front, a plank stack, saw horse with a half-cut log
    _logs(0.62, 0.15, (5, 4, 3, 2), 'y', 0.078, 1.05)
    _logs(0.0, 0.78, (4, 3, 2), 'x', 0.07, 0.95)
    for i in range(7):
        box(0.35, -0.7, 0.04 + i * 0.035, 0.7, 0.3, 0.03, M['woodLight'] if i % 2 == 0 else M['wood'])
    for s in (-1, 1):
        box(-0.2 + s * 0.22, -0.05, 0.0, 0.05, 0.3, 0.32, M['wood']).rotation_euler = (0, s * 0.22, 0)
    cyl(-0.2, -0.05 - 0.4, 0.3, 0.08, 0.8, M['bark'], verts=9, rx=-math.pi / 2)
    box(-0.2, -0.05, 0.42, 0.5, 0.012, 0.1, M['metal'])                                 # bow saw
    cyl(-0.95, 0.35, 0.04, 0.17, 0.26, M['bark'], verts=10)                              # stump with an axe
    cyl(-0.95, 0.35, 0.3, 0.15, 0.012, M['woodLight'], verts=10)
    cyl(-0.9, 0.38, 0.32, 0.014, 0.42, M['wood'], verts=6, rx=math.radians(65))
    box(-0.9, 0.58, 0.55, 0.05, 0.14, 0.1, M['metal'])
    rnd = random.Random(31)
    for i in range(6):                                                                  # leafy branches
        sphere(0.95 + rnd.uniform(-0.2, 0.1), -0.75 + rnd.uniform(-0.15, 0.15), 0.08, rnd.uniform(0.1, 0.17), M['leaf2'], seg=7, noise=0.3, seed=i + 40, sz=0.6)

def m_lumber(style):
    _lumbercamp(style)

def m_mine(style, kind):
    if kind == 'gold':
        _goldcamp(style)
    else:
        _stonecamp(style)
