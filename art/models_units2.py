# Castle Age units (exec'd by gen_assets.py)

def _arm(side, hip, swing=0.0, mat=None):
    a = cyl(0, 0, 0, 0.04, 0.3, mat or M['skin'], verts=7)
    a.data.transform(Matrix.Translation((0, 0, -0.3)))
    a.location = (0, side * 0.17, hip + 0.36)
    a.rotation_euler = (0, swing, 0)
    return a

def _finish_unit(start, dir_i, scale):
    new = _created[start:]
    root = bpy.data.objects.new('unit_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.rotation_euler = (0, 0, math.radians(45 * dir_i))
    root.scale = (scale, scale, scale)
    for o in new:
        o.parent = root

def build_unit2(kind, team, dir_i, frame):
    start = len(_created)
    stride = (0, 0.55, -0.55)[frame]
    bob = 0.0 if frame == 0 else 0.015
    hip = 0.40 + bob
    tm = team_mat(team)
    scale = 0.85
    if kind == 'ram':
        ram_model(0, 0, 0, 0, team)
        _finish_unit(start, dir_i, 0.95)
        return
    if kind == 'mangonel':
        mangonel_model(0, 0, 0, 0, team, 0.0)
        _finish_unit(start, dir_i, 0.95)
        return
    if kind == 'knight':
        horse(0, 0, 0, 0, 1.0, M['hide'] if team == 'blue' else M['woodDark'], stride * 0.8)
        # caparison in team colours
        box(0, 0, 0.78, 0.5, 0.58, 0.05, tm)
        box(0.0, -0.3, 0.55, 0.5, 0.03, 0.26, tm)
        box(0.0, 0.3, 0.55, 0.5, 0.03, 0.26, tm)
        # rider
        rz = 0.95
        cyl(0, 0, rz, 0.14, 0.34, tm, verts=10, r2=0.11)
        cyl(0, 0, rz + 0.08, 0.145, 0.05, M['iron'], verts=10)
        sphere(0, 0, rz + 0.42, 0.1, M['metal'], seg=10, sz=1.05)
        cone(0, 0, rz + 0.5, 0.035, 0.12, M['red'] if team != 'blue' else M['blue'], verts=6)
        _arm(-1, rz - 0.04, 0.0, M['metal']).location = (0.0, -0.17, rz + 0.3)
        _arm(1, rz - 0.04, 0.0, M['metal']).location = (0.0, 0.17, rz + 0.3)
        # lance with pennant + kite shield
        cyl(-0.2, -0.2, rz + 0.05, 0.016, 1.5, M['wood'], verts=6, ry=math.radians(90 - 8))
        cone(1.25, -0.2, rz + 0.0, 0.03, 0.12, M['metal'], verts=5).rotation_euler = (0, math.radians(82), 0)
        box(0.1, 0.24, rz + 0.0, 0.03, 0.2, 0.36, tm)
        box(0.12, 0.24, rz + 0.05, 0.02, 0.04, 0.26, M['white'])
        _finish_unit(start, dir_i, 0.9)
        return
    # ---- infantry-style units share the same skeleton
    pants = M['woodDark'] if kind != 'longsword' else M['iron']
    def leg(y, sw):
        o = cyl(0, 0, 0, 0.055, 0.40, pants, verts=8)
        o.data.transform(Matrix.Translation((0, 0, -0.40)))
        o.location = (0, y, hip)
        o.rotation_euler = (0, sw, 0)
    if kind == 'monk':
        # long robe hides the legs; walking = subtle sway
        cyl(0, 0, 0.02 + bob, 0.2, 0.62, M['hide'] if team == 'blue' else M['red'], verts=12, r2=0.13)
        cyl(0, 0, 0.5 + bob, 0.15, 0.3, M['hide'] if team == 'blue' else M['red'], verts=12, r2=0.12)
        cyl(0, 0, 0.55 + bob, 0.155, 0.045, M['white'], verts=12)      # sash
        sphere(0, 0, 0.9 + bob, 0.1, M['skin'], seg=10)
        cone(0, 0, 0.84 + bob, 0.13, 0.2, M['hide'] if team == 'blue' else M['red'], verts=10)   # hood
        _arm(-1, 0.55, 0.3 if frame == 1 else 0.0, M['skin'])
        _arm(1, 0.55, -0.3 if frame == 2 else 0.0, M['skin'])
        cyl(0.12, 0.2, 0.0, 0.018, 1.1, M['wood'], verts=6)            # staff with a cross
        box(0.12, 0.2, 0.95, 0.02, 0.14, 0.02, M['gold'])
        box(0.12, 0.2, 1.02, 0.02, 0.02, 0.1, M['gold'])
        _finish_unit(start, dir_i, scale)
        return
    leg(-0.07, stride)
    leg(0.07, -stride)
    tunic = tm
    if kind == 'crossbow':
        tunic = M['cloth2'] if team == 'blue' else M['red']
    cyl(0, 0, hip, 0.14, 0.40, tunic if kind != 'longsword' else M['iron'], verts=10, r2=0.115)
    if kind == 'longsword':
        cyl(0, 0, hip + 0.1, 0.152, 0.3, tm, verts=10, r2=0.13)        # surcoat
    cyl(0, 0, hip + 0.02, 0.145, 0.04, M['black'], verts=10)
    sphere(0, 0, hip + 0.47, 0.1, M['skin'], seg=10)
    sw = 0.45 * (stride / 0.55 if stride else 0)
    _arm(-1, hip, sw)
    _arm(1, hip, -sw)
    if kind == 'longsword':
        sphere(0, 0, hip + 0.5, 0.115, M['metal'], seg=10, sz=0.95)
        box(0.0, 0.0, hip + 0.47, 0.012, 0.2, 0.1, M['metal'])
        # long sword held two-handed in front
        box(0.16, -0.12, hip - 0.1, 0.025, 0.02, 0.75, M['metal'])
        box(0.16, -0.12, hip + 0.05, 0.025, 0.14, 0.03, M['iron'])
        box(0.12, 0.22, hip + 0.0, 0.03, 0.22, 0.4, tm)
        box(0.14, 0.22, hip + 0.05, 0.02, 0.04, 0.3, M['white'])
    elif kind == 'crossbow':
        cone(0, 0, hip + 0.45, 0.13, 0.2, M['cloth2'] if team == 'blue' else M['red'], verts=10)
        box(0.18, -0.15, hip + 0.18, 0.4, 0.04, 0.05, M['woodDark'])             # stock
        box(0.3, -0.15, hip + 0.2, 0.03, 0.3, 0.025, M['wood'])                  # limbs
        box(0.2, -0.15, hip + 0.22, 0.2, 0.012, 0.012, M['white'])               # bolt
        box(-0.12, 0.05, hip + 0.1, 0.07, 0.08, 0.28, M['hide'])
    # frame-dependent small weapon bob is omitted on purpose: silhouette stays readable
    _finish_unit(start, dir_i, scale)
