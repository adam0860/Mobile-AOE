M['goldIcon'] = mat2('v2goldIcon', solid('#f0b422', 0.38, 0.15, None, True))
M['goldDeep'] = mat2('v2goldDeep', solid('#b07a10', 0.4, 0.1, None, False))
# UI icons (exec'd by gen_assets.py).  Rendered from a low front/top camera at 128x128, background transparent.

def _icon_cam(target_z=0.35, scale=1.5):
    rot = Euler((math.radians(72), 0, math.radians(22)), 'XYZ')
    cam.rotation_euler = rot
    d = rot.to_matrix() @ Vector((0, 0, 1))
    cam.location = Vector((0, 0, target_z)) + d * 60
    cam_data.ortho_scale = scale
    cam_data.shift_x = 0
    cam_data.shift_y = 0
    scene.render.resolution_x = scene.render.resolution_y = 128

def render_icon(name, fn, z=0.35, scale=1.5):
    if not should(name):
        return
    clear_models()
    fn()
    _icon_cam(z, scale)
    scene.render.filepath = os.path.join(OUT, name + '.png')
    bpy.ops.render.render(write_still=True)
    MANIFEST[name] = {'w': 128, 'h': 128, 'ax': 64.0, 'ay': 64.0, 'pxm': 45.2548 * 2, 'icon': True}
    print('rendered', name)
    # restore the isometric camera
    cam.rotation_euler = Euler((math.radians(90) - ELEV, 0, math.radians(45)), 'XYZ')

def i_wood():
    for i, (x, z) in enumerate(((-0.2, 0.1), (0.2, 0.1), (0.0, 0.42))):
        c = cyl(x, -0.35, z, 0.17, 0.75, M['bark'], verts=14, rx=-math.pi / 2)
        cyl(x, 0.4, z, 0.16, 0.01, M['woodLight'], verts=14, rx=-math.pi / 2)
        cyl(x, 0.405, z, 0.11, 0.01, M['wood'], verts=14, rx=-math.pi / 2)
    cone(0.0, -0.3, 0.72, 0.06, 0.2, M['leaf2'], verts=6)

def i_food():
    for i in range(9):
        a = (i - 4) * 0.13
        c = cyl(math.sin(a) * 0.04, 0, 0.0, 0.012, 0.8, M['wheat'], verts=4)
        c.rotation_euler = (0, a, 0)
        for j in range(5):
            sphere(math.sin(a) * 0.04 + math.sin(a) * (0.5 + j * 0.06), 0.0, 0.5 + j * 0.06, 0.03, M['hay'], seg=5, sz=1.7)
    cyl(0, 0, 0.3, 0.11, 0.07, M['hide'], verts=10)
    sphere(0.2, -0.2, 0.08, 0.18, M['hay'], sx=1.3, sy=0.9, sz=0.7, seg=10)
    sphere(0.2, -0.2, 0.14, 0.15, M['woodLight'], sx=1.2, sy=0.85, sz=0.6, seg=10)

def i_gold():
    gm = M['goldIcon']
    for i in range(6):
        cyl(0.02 * (i % 2), 0.0, i * 0.07, 0.25, 0.065, gm, verts=22)
        cyl(0.02 * (i % 2), 0.0, i * 0.07 + 0.06, 0.2, 0.008, M['goldDeep'], verts=22)
    c = cyl(0.36, -0.1, 0.12, 0.2, 0.06, gm, verts=22); c.rotation_euler = (1.15, 0.3, 0)
    cyl(-0.34, -0.1, 0.0, 0.19, 0.06, gm, verts=22)
    cyl(-0.34, -0.1, 0.055, 0.15, 0.008, M['goldDeep'], verts=22)

def i_stone():
    for i, (x, y, r, sz) in enumerate(((0, 0, 0.36, 0.8), (0.3, -0.12, 0.22, 0.8), (-0.3, -0.1, 0.2, 0.8), (0.0, -0.3, 0.17, 0.8))):
        sphere(x, y, 0.1, r, M['rock'], seg=8, noise=0.25, seed=i + 5, sz=sz)

def i_pop():
    sphere(0, 0, 0.62, 0.22, M['skin'], seg=12)
    cyl(0, 0, 0.78, 0.4, 0.03, M['hay'], verts=16)
    cone(0, 0, 0.8, 0.24, 0.2, M['hay'], verts=14)
    cyl(0, 0, 0.0, 0.3, 0.42, M['blue'], verts=14, r2=0.22)
    for s in (-1, 1):
        sphere(0.18, s * 0.08, 0.66, 0.025, M['black'], seg=5)

def i_trophy():
    cyl(0, 0, 0.0, 0.26, 0.08, M['woodDark'], verts=14)
    cyl(0, 0, 0.08, 0.06, 0.28, M['goldIcon'], verts=10)
    cyl(0, 0, 0.36, 0.12, 0.05, M['goldIcon'], verts=12)
    cyl(0, 0, 0.4, 0.07, 0.5, M['goldIcon'], verts=14, r2=0.3)
    for s in (-1, 1):
        sphere(s * 0.31, 0, 0.62, 0.11, M['goldIcon'], sx=0.5, sy=0.3, sz=1.0, seg=8)
    sphere(0, -0.28, 0.65, 0.07, M['red'], seg=8)

def i_swords():
    for s in (-1, 1):
        j = _jt(None, 0, 0, -0.42)
        j.rotation_euler = (0, s * math.radians(34), 0)
        _bx(j, 0, 0, 0.0, 0.055, 0.045, 0.3, M['woodDark'])
        _sp(j, 0, 0, -0.01, 0.055, M['goldIcon'], seg=8)
        _bx(j, 0, 0, 0.3, 0.4, 0.07, 0.065, M['goldIcon'])
        _bx(j, 0, 0, 0.36, 0.085, 0.035, 1.0, M['metal'])
        _bx(j, 0, 0, 0.36, 0.02, 0.04, 1.0, M['white'])
        _cn(j, 0, 0, 1.36, 0.042, 0.16, M['metal'], 4)

def i_hammer():
    j = _jt(None, 0, 0, -0.35)
    j.rotation_euler = (0, math.radians(32), 0)
    _cy(j, 0, 0, 0.0, 0.05, 0.95, M['woodLight'], verts=8)
    _bx(j, 0, 0, 0.88, 0.3, 0.2, 0.2, M['metal'])
    _bx(j, 0, 0, 0.88, 0.32, 0.22, 0.04, M['iron'])
    _bx(j, 0, 0, 1.04, 0.32, 0.22, 0.04, M['iron'])
    _sp(j, 0, 0, 0.0, 0.06, M['goldIcon'], seg=6)

def i_gear():
    cyl(0, 0, 0.0, 0.34, 0.14, M['iron'], verts=24, rx=-math.pi / 2)
    for i in range(10):
        a = i * math.pi / 5
        b = box(math.cos(a) * 0.4, 0, 0.0 + math.sin(a) * 0.4 + 0.0, 0.16, 0.14, 0.14, M['iron'])
        b.rotation_euler = (0, -a, 0)
    cyl(0, -0.03, 0.0, 0.14, 0.2, M['black'], verts=14, rx=-math.pi / 2)

def i_shield():
    pts = [(0.62, 0.0), (0.6, 0.1), (0.56, 0.2), (0.5, 0.3), (0.4, 0.4), (0.28, 0.5), (0.14, 0.6), (0.03, 0.68)]
    for i, (w, z) in enumerate(pts):
        box(0, 0, 0.74 - z - 0.1, w, 0.09, 0.1, M['blue'])
    box(0, -0.05, 0.3, 0.1, 0.03, 0.8, M['white'])
    box(0, -0.05, 0.55, 0.5, 0.03, 0.1, M['white'])
    box(0, 0, 0.72, 0.66, 0.1, 0.07, M['goldIcon'])
    for s in (-1, 1):
        box(s * 0.31, 0, 0.2, 0.05, 0.1, 0.55, M['goldIcon'])

def i_scroll():
    cyl(0, 0, 0.0, 0.1, 0.8, M['canvas'], verts=14, ry=math.pi / 2)
    box(0, 0, 0.12, 0.9, 0.02, 0.5, M['canvas'])
    cyl(-0.46, 0, 0.0, 0.11, 0.1, M['woodLight'], verts=12, ry=math.pi / 2)
    cyl(0.46, 0, 0.0, 0.11, 0.1, M['woodLight'], verts=12, ry=math.pi / 2)
    for i in range(4):
        box(0, -0.02, 0.2 + i * 0.1, 0.6 - (i % 2) * 0.2, 0.01, 0.02, M['woodDark'])

def i_clock():
    cyl(0, 0, 0, 0.36, 0.1, M['gold'], verts=24, rx=-math.pi / 2)
    cyl(0, -0.06, 0, 0.3, 0.02, M['white'], verts=24, rx=-math.pi / 2)
    box(0, -0.09, 0.1, 0.03, 0.02, 0.2, M['black'])
    box(0.08, -0.09, 0.0, 0.18, 0.02, 0.03, M['black'])
    cyl(0, 0, 0.36, 0.1, 0.1, M['gold'], verts=12, rx=-math.pi / 2)

def i_star():
    pts = []
    for i in range(10):
        a = math.pi / 2 + i * math.pi / 5
        r = 0.42 if i % 2 == 0 else 0.18
        pts.append((math.cos(a) * r, math.sin(a) * r + 0.05))
    verts = [(0, 0.06, 0.05)] + [(p[0], 0.0, p[1]) for p in pts] + [(0, -0.06, 0.05)]
    faces = []
    for i in range(10):
        faces.append((0, 1 + i, 1 + (i + 1) % 10))
        faces.append((11, 1 + (i + 1) % 10, 1 + i))
    mesh_obj('star', verts, faces, M['goldIcon'])

def i_sword():
    blade = box(0, 0, 0.15, 0.08, 0.03, 0.9, M['metal'])
    box(0, 0, 0.15, 0.34, 0.06, 0.06, M['gold'])
    box(0, 0, -0.2, 0.06, 0.05, 0.3, M['woodDark'])
    sphere(0, 0, -0.38, 0.06, M['gold'], seg=6)
    cone(0, 0, 1.05, 0.04, 0.12, M['metal'], verts=4)

ICON_JOBS = (('icon_wood', i_wood, 0.3, 1.5), ('icon_food', i_food, 0.35, 1.5), ('icon_gold', i_gold, 0.25, 1.4), ('icon_stone', i_stone, 0.25, 1.3),
             ('icon_pop', i_pop, 0.4, 1.6), ('icon_trophy', i_trophy, 0.42, 1.5), ('icon_swords', i_swords, 0.35, 1.9), ('icon_hammer', i_hammer, 0.3, 1.7),
             ('icon_gear', i_gear, 0.0, 1.2), ('icon_shield', i_shield, 0.3, 1.6), ('icon_scroll', i_scroll, 0.2, 1.5), ('icon_clock', i_clock, 0.1, 1.2),
             ('icon_star', i_star, 0.05, 1.1), ('icon_sword', i_sword, 0.4, 1.7))
