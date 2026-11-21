# Nature v2: trees, palms, bushes, rocks, ore nodes and the chunked ground (exec'd after models_v2.py)

M['bark'] = mat2('v2bark2', planks('#4e3524', '#745238', 0.16, 1.4, 0.7, '#2c1c10'))
M['log'] = M['bark']

def _branch(x, y, z, length, rx, ry, r=0.04):
    c = cyl(x, y, z, r, length, M['bark'], verts=6, r2=r * 0.6)
    c.rotation_euler = (rx, ry, 0)
    return c

def m_oak(k):
    rnd = random.Random(100 + k)
    th = 0.55 + rnd.uniform(0, 0.15)
    cone(0, 0, 0, 0.2, 0.2, M['bark'], verts=9)
    cyl(0, 0, 0.0, 0.12, th, M['bark'], verts=9, r2=0.08)
    for i in range(3):
        a = i * 2.1 + rnd.uniform(0, 1)
        _branch(0, 0, th * 0.75, 0.5, math.sin(a) * 0.7, -math.cos(a) * 0.7, 0.045)
    leaf = (M['leaf1'], M['leaf2'])
    cz = th + 0.62
    for i in range(15):
        a = rnd.uniform(0, 6.283); d = rnd.uniform(0.0, 0.52) ** 0.9
        z = cz + rnd.uniform(-0.38, 0.5) * (1.0 - d * 0.55)
        sphere(math.cos(a) * d, math.sin(a) * d, z, rnd.uniform(0.25, 0.4), leaf[i % 2], seg=11, noise=0.2, seed=k * 20 + i, sz=0.86)
    for i in range(6):                                   # bright sun-lit clumps on top
        a = rnd.uniform(0, 6.283); d = rnd.uniform(0.0, 0.28)
        sphere(math.cos(a) * d, math.sin(a) * d, cz + 0.55 + rnd.uniform(0, 0.15), rnd.uniform(0.17, 0.26), M['leaf2'], seg=9, noise=0.2, seed=k * 33 + i)

def m_pine(k):
    rnd = random.Random(200 + k)
    cone(0, 0, 0, 0.14, 0.14, M['bark'], verts=8)
    cyl(0, 0, 0, 0.09, 0.5, M['bark'], verts=8, r2=0.06)
    tiers = 6
    for i in range(tiers):
        r = (0.62 - i * 0.085) * rnd.uniform(0.92, 1.08)
        z = 0.32 + i * 0.3
        c = cone(rnd.uniform(-0.03, 0.03), rnd.uniform(-0.03, 0.03), z, r, 0.55, M['leafPine'], verts=11)
        c.rotation_euler = (rnd.uniform(-0.05, 0.05), rnd.uniform(-0.05, 0.05), rnd.uniform(0, 6))
        # drooping skirt of needles
        for j in range(7):
            a = j * 0.9 + rnd.uniform(0, 0.4)
            s = sphere(math.cos(a) * r * 0.72, math.sin(a) * r * 0.72, z + 0.06, r * 0.22, M['leafPine'], seg=7, noise=0.25, seed=i * 9 + j, sz=0.55)
    cone(0, 0, 0.32 + tiers * 0.3 - 0.08, 0.1, 0.3, M['leafPine'], verts=7)

def _cyl_between(p0, p1, r0, r1, mat, verts=8):
    d = Vector(p1) - Vector(p0); L = d.length
    o = cyl(0, 0, 0, r0, L, mat, verts=verts, r2=r1)
    o.location = p0
    o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return o

def m_palm(k):
    rnd = random.Random(300 + k)
    pts = [(0.0, 0.0, 0.0)]
    ang = rnd.uniform(0, 6.28); lean = 0.0
    for i in range(8):
        lean += 0.035
        x, y, z = pts[-1]
        pts.append((x + math.cos(ang) * math.sin(lean) * 0.26, y + math.sin(ang) * math.sin(lean) * 0.26, z + 0.26 * math.cos(lean)))
    for i in range(8):
        _cyl_between(pts[i], pts[i + 1], 0.095 - i * 0.008, 0.095 - (i + 1) * 0.008, M['bark'], 8)
        if i % 2 == 0:
            cyl(pts[i][0], pts[i][1], pts[i][2], 0.1 - i * 0.008, 0.035, M['woodDark'], verts=8)       # growth rings
    x, y, z = pts[-1]
    for i in range(12):
        a = i * 0.5236 + rnd.uniform(-0.1, 0.1)
        L = rnd.uniform(0.75, 0.95)
        droop = rnd.uniform(0.25, 0.6)
        end = (x + math.cos(a) * L, y + math.sin(a) * L, z + 0.12 - droop * L * 0.55)
        mid = (x + math.cos(a) * L * 0.5, y + math.sin(a) * L * 0.5, z + 0.2 - droop * L * 0.12)
        _cyl_between((x, y, z), mid, 0.02, 0.016, M['leaf1'], 4)
        _cyl_between(mid, end, 0.016, 0.004, M['leaf1'], 4)
        for j in range(6):                                # leaflets along the frond
            t = 0.3 + j * 0.12
            px = x + (end[0] - x) * t; py = y + (end[1] - y) * t; pz = z + (end[2] - z) * t + 0.03 * math.sin(t * 3)
            for sd in (-1, 1):
                lf = sphere(px - math.sin(a) * sd * 0.07, py + math.cos(a) * sd * 0.07, pz - 0.03, 0.12, M['leaf2'] if (i + j) % 2 else M['leaf1'], sx=0.3, sy=1.5, sz=0.1, seg=6)
                lf.rotation_euler = (0, 0, a + sd * 0.15)
    for i in range(4):
        sphere(x + math.cos(i * 1.6) * 0.07, y + math.sin(i * 1.6) * 0.07, z - 0.06, 0.055, M['hide'], seg=6)

def m_goldnode():
    rnd = random.Random(7)
    for i, (x, y, r, sz) in enumerate(((0, 0, 0.36, 0.75), (-0.3, 0.12, 0.24, 0.8), (0.28, -0.1, 0.22, 0.85), (-0.05, 0.3, 0.2, 0.8))):
        sphere(x, y, 0.06, r, M['rock'], seg=9, noise=0.28, seed=i + 1, sz=sz)
    for i in range(16):
        a = rnd.uniform(0, 6.28); d = rnd.uniform(0.0, 0.38)
        sphere(math.cos(a) * d, math.sin(a) * d, 0.18 + rnd.uniform(0, 0.2), rnd.uniform(0.06, 0.12), M['gold'], seg=6, noise=0.32, seed=i)
    for i in range(6):
        sphere(rnd.uniform(-0.4, 0.4), rnd.uniform(-0.4, 0.4), 0.03, rnd.uniform(0.04, 0.07), M['gold'], seg=6, noise=0.3, seed=i + 90, sz=0.7)

def m_stonenode():
    rnd = random.Random(11)
    for i, (x, y, r, sz, mt) in enumerate(((0, 0, 0.36, 0.8, 'rock'), (0.32, 0.14, 0.24, 0.75, 'stoneDark'), (-0.3, -0.12, 0.22, 0.8, 'rock'), (0.05, -0.32, 0.16, 0.8, 'stoneDark'), (-0.2, 0.28, 0.17, 0.8, 'rock'))):
        sphere(x, y, 0.06, r, M[mt], seg=9, noise=0.3, seed=i + 11, sz=sz)
    for i in range(6):
        sphere(rnd.uniform(-0.45, 0.45), rnd.uniform(-0.45, 0.45), 0.02, rnd.uniform(0.04, 0.08), M['pebble'], seg=6, noise=0.3, seed=i, sz=0.6)
    for i in range(3):
        sphere(rnd.uniform(-0.3, 0.3), rnd.uniform(-0.3, 0.3), 0.3, 0.07, M['leaf2'], seg=6, noise=0.3, sz=0.4)       # moss

# decor sprites -------------------------------------------------------------
def m_bush(k):
    bush(0, 0, 1.6 + k * 0.3, k, k == 1)
def m_tuft(k):
    for i in range(3 + k):
        tuft(random.Random(k * 5 + i).uniform(-0.2, 0.2), random.Random(k * 7 + i).uniform(-0.2, 0.2), 2.0, k * 10 + i)
def m_flowers(k):
    rnd = random.Random(k + 70)
    for i in range(10 + k * 3):
        x, y = rnd.uniform(-0.25, 0.25), rnd.uniform(-0.25, 0.25)
        cyl(x, y, 0, 0.008, rnd.uniform(0.1, 0.2), M['grassBlade'], verts=4)
        sphere(x, y, rnd.uniform(0.12, 0.22), 0.035, M[('flower1', 'flower2', 'flower3')[(i + k) % 3]], seg=6, sz=0.7)
    tuft(0, 0, 1.5, k)
def m_rocks(k):
    rnd = random.Random(k + 20)
    for i in range(3 + k):
        sphere(rnd.uniform(-0.2, 0.2), rnd.uniform(-0.2, 0.2), 0.04, rnd.uniform(0.07, 0.17), M['rock'], seg=7, noise=0.3, seed=i + k, sz=0.7)
    tuft(0.15, 0.1, 1.2, k)
def m_stump():
    cyl(0, 0, 0, 0.17, 0.26, M['bark'], verts=10, r2=0.15)
    cyl(0, 0, 0.255, 0.15, 0.015, M['woodLight'], verts=10)
    for i in range(3):
        a = i * 2.1
        cone(math.cos(a) * 0.17, math.sin(a) * 0.17, 0, 0.06, 0.14, M['bark'], verts=5)

# ------------------------------------------------------------------ ground: 2x2 chunks of the whole 30x30 map
def m_ground2():
    m, nt, bsdf, uv, obj = _mk('ground2')
    big = _noise(nt, obj, 0.16, 4, 0.6)
    mid = _noise(nt, obj, 0.9, 5, 0.62)
    fine = _noise(nt, obj, 8, 5, 0.6)
    micro = _noise(nt, obj, 40, 4, 0.55)
    base = _ramp(nt, mid, [(0.25, '#3f7028'), (0.5, '#5a9138'), (0.78, '#86b84a')])
    fr = _ramp(nt, fine, [(0.3, '#c8c8c8'), (0.7, '#ffffff')])
    base = _mulc(nt, base, fr, 0.55)
    mr = _ramp(nt, micro, [(0.3, '#b8b8b8'), (0.7, '#ffffff')])
    base = _mulc(nt, base, mr, 0.4)
    dry = _ramp(nt, big, [(0.35, '#6d9a3a'), (0.65, '#a6a652')])
    base = _mixc(nt, base, dry, _ramp(nt, _noise(nt, obj, 0.35, 3, 0.5), [(0.5, '#000000'), (0.72, '#ffffff')]))
    patch = _ramp(nt, big, [(0.6, '#000000'), (0.7, '#ffffff')])
    dirt = _ramp(nt, fine, [(0.3, '#7c6038'), (0.7, '#a4845a')])
    base = _mixc(nt, base, dirt, patch)
    n = _bump(nt, micro, None, 0.45, 0.04)
    _finish(nt, bsdf, base, n, 0.97, True, 0.2)
    return m

def build_ground2():
    gm = m_ground2()
    thick = 0.9
    v = [(0, -30, -thick), (30, -30, -thick), (30, 0, -thick), (0, 0, -thick), (0, -30, 0), (30, -30, 0), (30, 0, 0), (0, 0, 0)]
    f = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    me = bpy.data.meshes.new('ground')
    me.from_pydata(v, [], f)
    me.update()
    assign_uv(me)
    me.materials.append(gm)
    me.materials.append(M['dirt'])
    for i, p in enumerate(me.polygons):
        p.material_index = 0 if i == 1 else 1
    o = bpy.data.objects.new('ground', me)
    scene.collection.objects.link(o)
    _created.append(o)
    rnd = random.Random(2024)
    # grass tufts, flowers and pebbles scattered over the whole map (one joined mesh per kind)
    def scatter(count, build, mat, name):
        bm = bmesh.new()
        for i in range(count):
            x, y = rnd.uniform(0.3, 29.7), -rnd.uniform(0.3, 29.7)
            build(bm, x, y, rnd)
        me2 = bpy.data.meshes.new(name)
        bm.to_mesh(me2)
        bm.free()
        assign_uv(me2)
        ob = _link(bpy.data.objects.new(name, me2), mat)
        return ob
    def blade_cluster(bm, x, y, r):
        for _ in range(5):
            a = r.uniform(0, 6.28); d = r.uniform(0.0, 0.07)
            h = r.uniform(0.1, 0.24)
            res = bmesh.ops.create_cone(bm, cap_ends=False, segments=4, radius1=0.018, radius2=0.002, depth=h)
            tilt = r.uniform(-0.4, 0.4)
            bmesh.ops.rotate(bm, verts=res['verts'], cent=(0, 0, 0), matrix=Matrix.Rotation(tilt, 3, 'X') @ Matrix.Rotation(r.uniform(0, 6.28), 3, 'Z'))
            bmesh.ops.translate(bm, verts=res['verts'], vec=(x + math.cos(a) * d, y + math.sin(a) * d, h * 0.5))
    def flower(bm, x, y, r):
        h = r.uniform(0.08, 0.16)
        res = bmesh.ops.create_icosphere(bm, subdivisions=1, radius=r.uniform(0.022, 0.034))
        bmesh.ops.translate(bm, verts=res['verts'], vec=(x, y, h))
    def pebble(bm, x, y, r):
        res = bmesh.ops.create_icosphere(bm, subdivisions=1, radius=r.uniform(0.03, 0.08))
        bmesh.ops.scale(bm, verts=res['verts'], vec=(1, 1, 0.55))
        bmesh.ops.translate(bm, verts=res['verts'], vec=(x, y, 0.01))
    scatter(2600, blade_cluster, M['grassBlade'], 'tufts')
    scatter(260, flower, M['flower1'], 'fl1')
    scatter(260, flower, M['flower2'], 'fl2')
    scatter(240, flower, M['flower3'], 'fl3')
    scatter(420, pebble, M['pebble'], 'pebbles')
    for i in range(34):                                   # a few bushes and boulders for scale
        x, y = rnd.uniform(1, 29), -rnd.uniform(1, 29)
        if i % 3 == 0:
            for j in range(3):
                sphere(x + rnd.uniform(-0.15, 0.15), y + rnd.uniform(-0.15, 0.15), 0.08, rnd.uniform(0.14, 0.26), M['rock'], seg=8, noise=0.3, seed=i * 3 + j, sz=0.65)
        else:
            bush(x, y, rnd.uniform(1.0, 1.7), i, rnd.random() < 0.5)
    return o
