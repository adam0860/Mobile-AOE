# Exploration content (exec'd by gen_assets.py): windmill (Mill), wild animals, treasure chest.

def m_mill():
    # round stone tower with a thatched cap and four canvas sails facing +X (towards the camera)
    cyl(0, 0, 0, 0.62, 0.22, M['stoneDark'], verts=14)
    cyl(0, 0, 0.18, 0.55, 1.15, M['plaster'], verts=14, r2=0.4)
    for k in range(3):
        cyl(0, 0, 0.35 + k * 0.32, 0.5 - k * 0.045, 0.045, M['woodDark'], verts=14)
    cone(0, 0, 1.3, 0.5, 0.5, M['thatch'], verts=14)
    cyl(0, 0, 1.76, 0.06, 0.12, M['woodDark'], verts=6)
    door(0.53, 0, 'x', 0.3, 0.5)
    box(0.37, 0.0, 0.85, 0.05, 0.16, 0.22, M['woodDark'])
    hub = (0.5, 0.0, 1.4)
    cyl(hub[0] - 0.02, hub[1], hub[2], 0.07, 0.16, M['iron'], verts=8, ry=math.pi / 2)
    for a in (math.radians(35), math.radians(125), math.radians(215), math.radians(305)):
        d = (0.0, -math.sin(a), math.cos(a))
        b = box(hub[0] + 0.1, hub[1], hub[2], 0.035, 0.07, 1.25, M['woodLight'])
        b.rotation_euler = (a, 0, 0)
        s = box(hub[0] + 0.11, hub[1] + d[1] * 0.68 + (-d[2]) * 0.0, hub[2] + d[2] * 0.68, 0.02, 0.2, 0.55, M['canvas'])
        s.rotation_euler = (a, 0, 0)
        s.location = (hub[0] + 0.115, hub[1] + d[1] * 0.35, hub[2] + d[2] * 0.35)
    sack(0.75, 0.45); sack(0.8, 0.28); sack(0.72, -0.5)
    barrel(0.7, -0.35)
    box(0.9, 0.0, 0, 0.5, 0.4, 0.05, M['dirt'])


def _animal_root(parts_fn, name, yaw, up=1.0):
    clear_models()
    parts_fn()
    root = bpy.data.objects.new('animal_root', None)
    scene.collection.objects.link(root); _created.append(root)
    for o in list(_created):
        if o.parent is None and o is not root:
            o.parent = root
    root.rotation_euler = (0, 0, yaw)
    return root


def _legs(coat, hoof, x0, x1, ys, h, r):
    for lx in (x0, x1):
        for ly in ys:
            cyl(lx, ly, 0.0, r, h, coat, verts=7, r2=r * 0.7)
            cyl(lx, ly, 0.0, r * 0.8, 0.045, hoof, verts=7)


def p_deer():
    coat, belly = M['hide'], M['hay']
    sphere(0, 0, 0.38, 0.28, coat, sx=1.5, sy=0.62, sz=0.7, seg=12)
    sphere(0, 0, 0.3, 0.22, belly, sx=1.4, sy=0.6, sz=0.5, seg=10)
    _legs(coat, M['black'], 0.28, -0.28, (0.1, -0.1), 0.34, 0.035)
    cyl(0.34, 0, 0.42, 0.075, 0.38, coat, verts=8, r2=0.05, ry=math.radians(-28))
    sphere(0.52, 0, 0.78, 0.085, coat, sx=1.5, sy=0.85, sz=0.85, seg=9)
    sphere(0.6, 0, 0.77, 0.03, M['black'], seg=5)
    for s in (-1, 1):
        cone(0.45, s * 0.05, 0.84, 0.025, 0.1, coat, verts=5)
        cyl(0.47, s * 0.05, 0.86, 0.012, 0.22, M['woodLight'], verts=5, r2=0.006, rx=s * 0.3)
        cyl(0.5, s * 0.11, 1.0, 0.009, 0.14, M['woodLight'], verts=5, r2=0.004, rx=s * 0.5, ry=-0.5)
    sphere(-0.4, 0, 0.5, 0.07, M['white'], sx=1.0, sy=0.9, sz=1.0, seg=6)


def p_boar():
    coat, dark = M['woodDark'], M['black']
    sphere(0, 0, 0.3, 0.32, coat, sx=1.35, sy=0.85, sz=0.78, seg=12)
    sphere(0.25, 0, 0.36, 0.24, coat, sx=1.0, sy=0.95, sz=0.85, seg=10)
    _legs(dark, dark, 0.27, -0.25, (0.14, -0.14), 0.22, 0.045)
    sphere(0.52, 0, 0.3, 0.17, coat, sx=1.15, sy=0.9, sz=0.85, seg=9)
    sphere(0.68, 0, 0.26, 0.09, M['skin'], sx=1.0, sy=1.0, sz=0.8, seg=8)
    for s in (-1, 1):
        cone(0.64, s * 0.1, 0.18, 0.025, 0.12, M['white'], verts=5).rotation_euler = (s * 0.5, -0.6, 0)
        cone(0.5, s * 0.1, 0.42, 0.04, 0.1, coat, verts=5)
        sphere(0.6, s * 0.1, 0.35, 0.02, M['black'], seg=5)
    for i in range(6):
        cone(0.2 - i * 0.1, 0, 0.55 - abs(i - 2) * 0.03, 0.03, 0.12, dark, verts=4)
    cyl(-0.44, 0, 0.3, 0.015, 0.14, dark, verts=5, ry=math.radians(-60))


def p_wolf():
    coat, dark = M['pebble'] if 'pebble' in M else M['rock'], M['stoneDark']
    sphere(0, 0, 0.36, 0.26, coat, sx=1.55, sy=0.55, sz=0.62, seg=12)
    sphere(0.3, 0, 0.42, 0.2, dark, sx=1.0, sy=0.8, sz=0.8, seg=10)
    _legs(coat, M['black'], 0.28, -0.26, (0.1, -0.1), 0.33, 0.03)
    cyl(0.4, 0, 0.42, 0.07, 0.3, coat, verts=8, r2=0.05, ry=math.radians(-45))
    sphere(0.56, 0, 0.66, 0.09, coat, sx=1.2, sy=0.85, sz=0.85, seg=9)
    sphere(0.69, 0, 0.62, 0.05, dark, sx=1.4, sy=0.8, sz=0.7, seg=7)
    sphere(0.73, 0, 0.64, 0.016, M['black'], seg=5)
    for s in (-1, 1):
        cone(0.52, s * 0.055, 0.74, 0.03, 0.08, dark, verts=4)
        sphere(0.62, s * 0.055, 0.68, 0.013, M['fire'], seg=5)
    cyl(-0.4, 0, 0.42, 0.045, 0.42, coat, verts=7, r2=0.02, ry=math.radians(-110))


def m_animal(kind, k):
    fn = {'deer': p_deer, 'boar': p_boar, 'wolf': p_wolf}[kind]
    # k: 0 faces +x, 1 faces +y (grid axes; Blender Y = -grid y), 2 faces -x, 3 faces -y
    yaw = (0.0, -math.pi / 2, math.pi, math.pi / 2)[k]
    _animal_root(fn, kind, yaw)


def m_chest():
    box(0, 0, 0, 0.62, 0.42, 0.28, M['wood'])
    cyl(0, 0, 0.28, 0.21, 0.62, M['woodLight'], verts=12, r2=0.21, rx=0.0, ry=math.pi / 2).location = (-0.31, 0, 0.28)
    for sx in (-0.2, 0.2):
        box(sx, 0, 0, 0.06, 0.44, 0.3, M['gold'])
        cyl(sx - 0.03, 0, 0.28, 0.215, 0.06, M['gold'], verts=12, ry=math.pi / 2)
    box(0.0, -0.22, 0.12, 0.1, 0.03, 0.12, M['gold'])
    for i in range(7):
        a = i * 0.9
        sphere(0.38 + math.cos(a) * 0.12, math.sin(a) * 0.2, 0.03, 0.045, M['gold'], sz=0.35, seg=7)
    sphere(0.0, 0.0, 0.3, 0.2, M['gold'], sx=1.3, sy=0.8, sz=0.35, seg=8)


def m_relic():
    # a golden reliquary on a small stone plinth (the old-world relics monks fetch)
    box(0, 0, 0, 0.56, 0.56, 0.07, M['stoneDark'])
    box(0, 0, 0.07, 0.46, 0.46, 0.06, M['stone'])
    box(0, 0, 0.13, 0.34, 0.34, 0.2, M['marble'])
    box(0, 0, 0.33, 0.4, 0.4, 0.04, M['gold'])
    box(0, 0, 0.37, 0.3, 0.3, 0.22, M['gold'])
    for s in (-1, 1):
        box(0.16 * s, 0, 0.4, 0.02, 0.2, 0.14, M['white'])
        box(0, 0.16 * s, 0.4, 0.2, 0.02, 0.14, M['white'])
    prism_roof(0, 0, 0.59, 0.3, 0.3, 0.2, M['gold'], over=0.05, along='x')
    cone(0, 0, 0.8, 0.035, 0.12, M['gold'], verts=6)
    sphere(0.17, 0.0, 0.46, 0.035, M['blue'], seg=6)
    box(0.0, 0.0, 0.0, 0.7, 0.7, 0.02, M['dirt'])
