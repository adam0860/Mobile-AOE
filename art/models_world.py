# Nature, units and ground (exec'd by gen_assets.py)

# ------------------------------------------------------------------ trees / resource nodes
def m_oak(k):
    rnd = random.Random(100 + k)
    h = 0.32 + rnd.uniform(0, 0.1)
    cyl(0, 0, 0, 0.12, h, M['bark'], verts=8, r2=0.08)
    leaf = M['leaf1'] if k % 2 == 0 else M['leaf2']
    for i in range(5):
        a = rnd.uniform(0, 6.28)
        d = rnd.uniform(0, 0.28)
        sphere(math.cos(a) * d * 1.4, math.sin(a) * d * 1.4, h + 0.3 + rnd.uniform(0, 0.5), rnd.uniform(0.38, 0.52), leaf, seg=10, noise=0.16, seed=k * 10 + i, sz=0.85)
    sphere(0, 0, h + 0.6, 0.5, leaf, seg=10, noise=0.14, seed=k)

def m_pine(k):
    rnd = random.Random(200 + k)
    cyl(0, 0, 0, 0.08, 0.4, M['bark'], verts=7, r2=0.06)
    for i in range(4):
        cone(0, 0, 0.3 + i * 0.36, 0.5 - i * 0.1 + rnd.uniform(-0.03, 0.03), 0.62, M['leafPine'], verts=9)

def m_goldnode():
    rnd = random.Random(7)
    sphere(0, 0, 0.05, 0.33, M['rock'], seg=9, noise=0.22, seed=1, sz=0.7)
    sphere(-0.28, 0.1, 0.04, 0.2, M['rock'], seg=8, noise=0.25, seed=2, sz=0.7)
    for i in range(10):
        a = rnd.uniform(0, 6.28)
        d = rnd.uniform(0.0, 0.35)
        sphere(math.cos(a) * d, math.sin(a) * d, 0.16 + rnd.uniform(0, 0.15), rnd.uniform(0.07, 0.12), M['gold'], seg=6, noise=0.3, seed=i)

def m_stonenode():
    sphere(0, 0, 0.06, 0.34, M['rock'], seg=9, noise=0.25, seed=11, sz=0.8)
    sphere(0.3, 0.14, 0.04, 0.22, M['stoneDark'], seg=8, noise=0.3, seed=12, sz=0.75)
    sphere(-0.28, -0.12, 0.04, 0.2, M['rock'], seg=8, noise=0.3, seed=13, sz=0.8)
    sphere(0.05, -0.3, 0.03, 0.14, M['stoneDark'], seg=7, noise=0.3, seed=14, sz=0.8)

# ------------------------------------------------------------------ units
_unit_root = None
def _leg(x, y, hipz, length, r, mat, swing):
    o = cyl(0, 0, 0, r, length, mat, verts=8)
    o.data.transform(Matrix.Translation((0, 0, -length)))
    o.location = (x, y, hipz)
    o.rotation_euler = (0, swing, 0)
    return o

def build_unit(kind, team, dir_i, frame):
    objs_before = len(_created)
    tunic = {'villager': M['cloth1'], 'militia': team_mat(team), 'manatarms': team_mat(team), 'archer': M['cloth2'] if team == 'blue' else M['red']}[kind]
    stride = (0, 0.55, -0.55)[frame]
    bob = 0.0 if frame == 0 else 0.015
    hip = 0.40 + bob
    pants = M['woodDark'] if kind != 'manatarms' else M['iron']
    _leg(0, -0.07, hip, 0.40, 0.055, pants, stride)
    _leg(0, 0.07, hip, 0.40, 0.055, pants, -stride)
    cyl(0, 0, hip, 0.14, 0.40, tunic, verts=10, r2=0.115)
    cyl(0, 0, hip + 0.02, 0.145, 0.04, M['black'], verts=10)
    sphere(0, 0, hip + 0.47, 0.1, M['skin'], seg=10)
    # arms
    arm_sw = 0.45 * (stride / 0.55 if stride else 0)
    for side in (-1, 1):
        a = cyl(0, 0, 0, 0.04, 0.3, M['skin'], verts=7)
        a.data.transform(Matrix.Translation((0, 0, -0.3)))
        a.location = (0, side * 0.17, hip + 0.36)
        a.rotation_euler = (0, -arm_sw * side * -1, 0)
    if kind == 'villager':
        cone(0, 0, hip + 0.52, 0.17, 0.09, M['hay'], verts=12)
        cyl(0, 0, hip + 0.52, 0.17, 0.012, M['hay'], verts=12)
        cyl(0.12, 0.2, hip - 0.1, 0.014, 0.4, M['wood'], verts=5, ry=math.radians(-25))
        box(0.2, 0.2, hip + 0.28, 0.08, 0.05, 0.05, M['iron'])
        box(-0.15, 0, hip + 0.1, 0.1, 0.2, 0.2, M['woodLight'])     # sack on back
    elif kind == 'militia':
        sphere(0, 0, hip + 0.52, 0.105, M['iron'], seg=10, sz=0.7)
        cyl(0.08, -0.2, hip + 0.1, 0.012, 0.28, M['metal'], verts=5, ry=math.radians(70))
        box(0.12, -0.2, hip + 0.1, 0.02, 0.012, 0.32, M['metal'])
        cyl(0.12, 0.22, hip + 0.13, 0.15, 0.03, M['woodLight'], verts=12, rx=0.0, ry=math.radians(90))
        cyl(0.145, 0.22, hip + 0.13, 0.04, 0.03, M['metal'], verts=8, ry=math.radians(90))
    elif kind == 'manatarms':
        sphere(0, 0, hip + 0.5, 0.115, M['metal'], seg=10, sz=0.9)
        cone(0, 0, hip + 0.58, 0.04, 0.09, M['red'] if team != 'blue' else M['blue'], verts=6)
        box(0.1, -0.2, hip - 0.12, 0.025, 0.02, 0.55, M['metal'])
        box(0.1, -0.2, hip + 0.07, 0.025, 0.12, 0.03, M['iron'])
        # kite shield
        box(0.14, 0.23, hip + 0.0, 0.03, 0.2, 0.36, team_mat(team))
        box(0.16, 0.23, hip + 0.05, 0.02, 0.04, 0.26, M['white'])
        box(0.16, 0.23, hip + 0.19, 0.02, 0.16, 0.04, M['white'])
        box(0, 0, hip + 0.32, 0.33, 0.33, 0.07, M['metal'])
    elif kind == 'archer':
        cone(0, 0, hip + 0.45, 0.13, 0.2, M['cloth2'], verts=10)
        # bow (arc of small segments) + quiver
        for i in range(7):
            t = i / 6
            ang = (t - 0.5) * 2.0
            bx = 0.14 + math.cos(ang) * 0.0 + (1 - abs(t - 0.5) * 2) * 0.08
            bz = hip + 0.12 + t * 0.5
            box(bx + 0.04, -0.2, bz, 0.02, 0.02, 0.09, M['wood'], rz=0)
        box(0.1, -0.2, hip + 0.12, 0.005, 0.005, 0.5, M['white'])
        box(-0.12, 0.05, hip + 0.1, 0.07, 0.07, 0.3, M['hide'], rz=0.0)
        for i in range(3):
            cyl(-0.12 + i * 0.012, 0.05 + i * 0.02, hip + 0.4, 0.008, 0.08, M['white'], verts=4)
    # rotate whole figure by dir
    new = _created[objs_before:]
    root = bpy.data.objects.new('unit_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    root.rotation_euler = (0, 0, math.radians(45 * dir_i))
    root.scale = (0.85, 0.85, 0.85)
    for o in new:
        o.parent = root

# ------------------------------------------------------------------ ground (whole 30x30 map in one image)
def m_ground():
    m = bpy.data.materials.new('ground')
    try:
        m.use_nodes = True
    except Exception:
        pass
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    bsdf.inputs['Roughness'].default_value = 0.95
    tc = nt.nodes.new('ShaderNodeTexCoord')
    def noise(scale, detail=6, rough=0.55):
        n = nt.nodes.new('ShaderNodeTexNoise')
        n.inputs['Scale'].default_value = scale
        n.inputs['Detail'].default_value = detail
        n.inputs['Roughness'].default_value = rough
        nt.links.new(tc.outputs['Object'], n.inputs['Vector'])
        return n
    big = noise(0.11, 3)
    mid = noise(0.9, 5)
    fine = noise(14, 6)
    def ramp(inp, c0, c1, p0=0.3, p1=0.7):
        r = nt.nodes.new('ShaderNodeValToRGB')
        r.color_ramp.elements[0].position = p0
        r.color_ramp.elements[1].position = p1
        r.color_ramp.elements[0].color = srgb(c0)
        r.color_ramp.elements[1].color = srgb(c1)
        nt.links.new(inp, r.inputs['Fac'])
        return r
    grass_a = ramp(mid.outputs['Fac'], '#4a7a31', '#6fa047', 0.35, 0.65)
    grass_b = ramp(fine.outputs['Fac'], '#3f6f2b', '#5f9340', 0.3, 0.7)
    mixg = nt.nodes.new('ShaderNodeMix')
    mixg.data_type = 'RGBA'
    mixg.inputs[0].default_value = 0.45
    nt.links.new(grass_a.outputs['Color'], mixg.inputs[6])
    nt.links.new(grass_b.outputs['Color'], mixg.inputs[7])
    dirt = ramp(mid.outputs['Fac'], '#7a5f3a', '#9a7d4e', 0.3, 0.7)
    mask = ramp(big.outputs['Fac'], '#000000', '#ffffff', 0.58, 0.68)
    mixd = nt.nodes.new('ShaderNodeMix')
    mixd.data_type = 'RGBA'
    nt.links.new(mask.outputs['Color'], mixd.inputs[0])
    nt.links.new(mixg.outputs[2], mixd.inputs[6])
    nt.links.new(dirt.outputs['Color'], mixd.inputs[7])
    nt.links.new(mixd.outputs[2], bsdf.inputs['Base Color'])
    bn = nt.nodes.new('ShaderNodeBump')
    bn.inputs['Strength'].default_value = 0.35
    bn.inputs['Distance'].default_value = 0.08
    nt.links.new(fine.outputs['Fac'], bn.inputs['Height'])
    nt.links.new(bn.outputs['Normal'], bsdf.inputs['Normal'])
    return m

def build_ground():
    gm = m_ground()
    thick = 0.9
    v = [(0, -30, -thick), (30, -30, -thick), (30, 0, -thick), (0, 0, -thick), (0, -30, 0), (30, -30, 0), (30, 0, 0), (0, 0, 0)]
    f = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    me = bpy.data.meshes.new('ground')
    me.from_pydata(v, [], f)
    me.update()
    me.materials.append(gm)
    me.materials.append(M['dirt'])
    for i, p in enumerate(me.polygons):
        p.material_index = 0 if i == 1 else 1
    o = bpy.data.objects.new('ground', me)
    scene.collection.objects.link(o)
    _created.append(o)
    return o
