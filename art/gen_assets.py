# Dawn of Kings — procedural isometric sprite generator for Blender (run headless).
#   blender -b -P art/gen_assets.py -- --out art/renders [--only name1,name2] [--engine EEVEE|CYCLES] [--samples N]
# Camera: orthographic, 30deg elevation, 45deg azimuth => 2:1 dimetric, matching the game's TW=64/TH=32 grid.
# World mapping: grid (gx,gy) <-> Blender (X=gx, Y=-gy).  Footprints are centred on the world origin.
import bpy, bmesh, math, os, sys, json, random
from mathutils import Vector, Euler, Matrix

# ---------------------------------------------------------------- args
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default
OUT = os.path.abspath(arg('--out', 'art/renders'))
ONLY = set(arg('--only', '').split(',')) - {''}
ENGINE = arg('--engine', 'EEVEE')
SAMPLES = int(arg('--samples', '48'))
os.makedirs(OUT, exist_ok=True)

S = 2.0                         # supersample: sprite px per game px
PXM = 45.2548 * S               # px per metre along screen-x  (tile diagonal sqrt2 m == TW px)
ELEV = math.radians(30)

# ---------------------------------------------------------------- scene
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
if ENGINE.upper() == 'CYCLES':
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = SAMPLES
    scene.cycles.use_denoising = True
    try:
        scene.cycles.device = 'CPU'
    except Exception:
        pass
else:
    eev = [e for e in engines if 'EEVEE' in e]
    scene.render.engine = eev[0] if eev else 'CYCLES'
    try:
        scene.eevee.taa_render_samples = SAMPLES
    except Exception:
        pass
    for attr, val in (('use_shadows', True), ('use_raytracing', False), ('use_fast_gi', True), ('fast_gi_quality', 0.85), ('fast_gi_distance', 2.5),
                      ('shadow_ray_count', 4), ('shadow_step_count', 16), ('shadow_resolution_scale', 1.0), ('light_threshold', 0.002), ('taa_render_samples', SAMPLES)):
        try:
            setattr(scene.eevee, attr, val)
        except Exception:
            pass
scene.render.film_transparent = True
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.image_settings.compression = 15
try:
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
except Exception:
    pass
scene.render.filter_size = 1.0

cam_data = bpy.data.cameras.new('cam')
cam_data.type = 'ORTHO'
cam_data.sensor_fit = 'HORIZONTAL'
cam_data.clip_start = 0.1
cam_data.clip_end = 400
cam = bpy.data.objects.new('cam', cam_data)
scene.collection.objects.link(cam)
cam.rotation_euler = Euler((math.radians(90) - ELEV, 0, math.radians(45)), 'XYZ')
scene.camera = cam

sun_data = bpy.data.lights.new('sun', 'SUN')
sun_data.energy = 5.4
sun_data.angle = math.radians(3.5)
sun_data.color = (1.0, 0.93, 0.8)
sun = bpy.data.objects.new('sun', sun_data)
scene.collection.objects.link(sun)
sun.rotation_euler = Vector((-0.55, -0.85, 1.15)).normalized().to_track_quat('Z', 'Y').to_euler()

fill_data = bpy.data.lights.new('fill', 'SUN')
fill_data.energy = 0.7
fill_data.color = (0.75, 0.85, 1.0)
fill_data.angle = math.radians(40)
fill = bpy.data.objects.new('fill', fill_data)
scene.collection.objects.link(fill)
fill.rotation_euler = Vector((0.9, 0.5, 0.6)).normalized().to_track_quat('Z', 'Y').to_euler()

world = bpy.data.worlds.new('w')
scene.world = world
try:
    world.use_nodes = True
except Exception:
    pass
bg = world.node_tree.nodes.get('Background')
if bg:
    bg.inputs['Color'].default_value = (0.55, 0.68, 0.95, 1)
    bg.inputs['Strength'].default_value = 0.62

def place_camera(W, H, ox_px, oy_px, target=(0, 0, 0), pxm=None):
    """Render WxH px; world `target` lands on pixel (ox_px, oy_px)."""
    scene.render.resolution_x = int(W)
    scene.render.resolution_y = int(H)
    scene.render.resolution_percentage = 100
    cam_data.ortho_scale = W / (pxm or PXM)
    d = Vector((math.cos(ELEV) * math.cos(math.radians(45)), -math.cos(ELEV) * math.sin(math.radians(45)), math.sin(ELEV))) * 120
    cam.location = Vector(target) + d
    cam_data.shift_x = (ox_px - W / 2) / W * -1.0
    cam_data.shift_y = (oy_px - H / 2) / W * 1.0

# ---------------------------------------------------------------- materials (procedural)
def srgb(h):
    h = h.lstrip('#')
    c = [int(h[i:i + 2], 16) / 255 for i in (0, 2, 4)]
    return tuple(((v / 12.92) if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4) for v in c) + (1,)

_mats = {}
def _node(nt, t, **kw):
    n = nt.nodes.new(t)
    for k, v in kw.items():
        setattr(n, k, v)
    return n

def material(name, kind, c1, c2=None, scale=6.0, rough=0.9, bump=0.25, stretch=(1, 1, 1), emit=0.0, metal=0.0):
    key = (name)
    if key in _mats:
        return _mats[key]
    m = bpy.data.materials.new(name)
    try:
        m.use_nodes = True
    except Exception:
        pass
    nt = m.node_tree
    nt.nodes.clear()
    out = _node(nt, 'ShaderNodeOutputMaterial')
    bsdf = _node(nt, 'ShaderNodeBsdfPrincipled')
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    for nm in ('Specular IOR Level', 'Specular'):
        if nm in bsdf.inputs:
            bsdf.inputs[nm].default_value = 0.15
            break
    c1 = srgb(c1)
    c2 = srgb(c2) if c2 else c1
    tc = _node(nt, 'ShaderNodeTexCoord')
    mp = _node(nt, 'ShaderNodeMapping')
    mp.inputs['Scale'].default_value = (stretch[0], stretch[1], stretch[2])
    nt.links.new(tc.outputs['Object'], mp.inputs['Vector'])
    height = None
    if kind == 'solid':
        bsdf.inputs['Base Color'].default_value = c1
    elif kind in ('noise', 'thatch', 'wood', 'tile'):
        if kind == 'noise' or kind == 'thatch':
            tex = _node(nt, 'ShaderNodeTexNoise')
            tex.inputs['Scale'].default_value = scale
            tex.inputs['Detail'].default_value = 8 if kind == 'thatch' else 5
            tex.inputs['Roughness'].default_value = 0.6
            nt.links.new(mp.outputs['Vector'], tex.inputs['Vector'])
            fac = tex.outputs['Fac']
        else:
            tex = _node(nt, 'ShaderNodeTexWave')
            tex.wave_type = 'BANDS'
            tex.inputs['Scale'].default_value = scale
            tex.inputs['Distortion'].default_value = 2.0 if kind == 'wood' else 0.4
            tex.inputs['Detail'].default_value = 3
            nt.links.new(mp.outputs['Vector'], tex.inputs['Vector'])
            fac = tex.outputs['Fac']
        ramp = _node(nt, 'ShaderNodeValToRGB')
        ramp.color_ramp.elements[0].color = c1
        ramp.color_ramp.elements[1].color = c2
        ramp.color_ramp.elements[0].position = 0.25
        ramp.color_ramp.elements[1].position = 0.75
        nt.links.new(fac, ramp.inputs['Fac'])
        nt.links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
        height = fac
    elif kind == 'brick':
        tex = _node(nt, 'ShaderNodeTexBrick')
        tex.inputs['Color1'].default_value = c1
        tex.inputs['Color2'].default_value = c2
        tex.inputs['Mortar'].default_value = (0.12, 0.11, 0.1, 1)
        tex.inputs['Scale'].default_value = scale
        tex.inputs['Mortar Size'].default_value = 0.03
        tex.inputs['Brick Width'].default_value = 0.6
        tex.inputs['Row Height'].default_value = 0.28
        nt.links.new(mp.outputs['Vector'], tex.inputs['Vector'])
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
        height = tex.outputs['Fac']
    elif kind == 'voronoi':
        tex = _node(nt, 'ShaderNodeTexVoronoi')
        tex.inputs['Scale'].default_value = scale
        nt.links.new(mp.outputs['Vector'], tex.inputs['Vector'])
        ramp = _node(nt, 'ShaderNodeValToRGB')
        ramp.color_ramp.elements[0].color = c1
        ramp.color_ramp.elements[1].color = c2
        nt.links.new(tex.outputs['Distance'], ramp.inputs['Fac'])
        nt.links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
        height = tex.outputs['Distance']
    if height is not None and bump > 0:
        bn = _node(nt, 'ShaderNodeBump')
        bn.inputs['Strength'].default_value = bump
        bn.inputs['Distance'].default_value = 0.05
        nt.links.new(height, bn.inputs['Height'])
        nt.links.new(bn.outputs['Normal'], bsdf.inputs['Normal'])
    if emit > 0:
        for nm in ('Emission Color', 'Emission'):
            if nm in bsdf.inputs:
                bsdf.inputs[nm].default_value = c1
                break
        if 'Emission Strength' in bsdf.inputs:
            bsdf.inputs['Emission Strength'].default_value = emit
    _mats[key] = m
    return m

def build_materials():
    M = {}
    M['wood'] = material('wood', 'wood', '#6a4526', '#8a6038', scale=7, stretch=(1, 1, 5), bump=0.35)
    M['woodDark'] = material('woodDark', 'wood', '#3e2a18', '#573a20', scale=7, stretch=(1, 1, 5), bump=0.3)
    M['woodLight'] = material('woodLight', 'wood', '#9a7242', '#b88c52', scale=9, stretch=(1, 1, 5), bump=0.3)
    M['plaster'] = material('plaster', 'noise', '#bfb08a', '#d9ccaa', scale=18, bump=0.15)
    M['stone'] = material('stone', 'brick', '#8d8c86', '#a3a198', scale=3.2, bump=0.5)
    M['stoneDark'] = material('stoneDark', 'noise', '#6d6c68', '#8a8982', scale=10, bump=0.5)
    M['thatch'] = material('thatch', 'thatch', '#9d8340', '#c9ae62', scale=22, stretch=(1, 1, 0.45), bump=0.6)
    M['tile'] = material('tile', 'tile', '#8f2f22', '#b4452f', scale=14, stretch=(1, 1, 1), bump=0.5)
    M['slate'] = material('slate', 'tile', '#4a5866', '#657687', scale=12, bump=0.4)
    M['dirt'] = material('dirt', 'noise', '#6d5232', '#8a6c43', scale=9, bump=0.4)
    M['soil'] = material('soil', 'wood', '#3d2a17', '#54391f', scale=24, stretch=(1, 0.04, 1), bump=0.7)
    M['crop'] = material('crop', 'wood', '#7a9a34', '#b5c24a', scale=30, stretch=(1, 0.05, 1), bump=0.6)
    M['wheat'] = material('wheat', 'wood', '#c9a43a', '#e6cc6a', scale=30, stretch=(1, 0.05, 1), bump=0.6)
    M['canvas'] = material('canvas', 'noise', '#cdbf9d', '#e4d9b9', scale=25, bump=0.1)
    M['hide'] = material('hide', 'noise', '#6e4b31', '#8d6644', scale=12, bump=0.25)
    M['hay'] = material('hay', 'thatch', '#c9a63e', '#ead076', scale=30, bump=0.4)
    M['metal'] = material('metal', 'solid', '#8c9096', rough=0.35, metal=0.9, bump=0)
    M['iron'] = material('iron', 'solid', '#4b4e52', rough=0.45, metal=0.8, bump=0)
    M['blue'] = material('blue', 'noise', '#2447a6', '#3a63c9', scale=14, bump=0.1, rough=0.8)
    M['red'] = material('red', 'noise', '#a62424', '#c93a3a', scale=14, bump=0.1, rough=0.8)
    M['white'] = material('white', 'solid', '#e8e4d8', bump=0)
    M['black'] = material('black', 'solid', '#17130f', bump=0)
    M['gold'] = material('gold', 'solid', '#f0b81c', rough=0.28, metal=1.0, bump=0)
    M['rock'] = material('rock', 'voronoi', '#6b6a66', '#9b9a92', scale=3.0, bump=0.6)
    M['leaf1'] = material('leaf1', 'noise', '#2f6b22', '#4b9334', scale=9, bump=0.5)
    M['leaf2'] = material('leaf2', 'noise', '#3f7a2c', '#6aa845', scale=9, bump=0.5)
    M['leafPine'] = material('leafPine', 'noise', '#1f4f27', '#32723a', scale=8, bump=0.6)
    M['bark'] = material('bark', 'wood', '#4b3320', '#6a4a2f', scale=8, stretch=(1, 1, 6), bump=0.5)
    M['skin'] = material('skin', 'solid', '#e0b08a', rough=0.7, bump=0)
    M['fire'] = material('fire', 'solid', '#ff8a1a', emit=6.0, bump=0)
    M['char'] = material('char', 'noise', '#1d1713', '#3a2c22', scale=9, bump=0.5)
    M['grassTop'] = material('grassTop', 'noise', '#4f7d35', '#6c9c46', scale=7, bump=0.35)
    M['cloth1'] = material('cloth1', 'solid', '#7a5a38', bump=0)
    M['cloth2'] = material('cloth2', 'solid', '#8a8f5a', bump=0)
    return M
M = build_materials()
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else os.path.abspath('art'), 'materials2.py'), encoding='utf-8').read(), globals())
M.update(build_materials2())
def team_mat(team):
    return M['blue'] if team == 'blue' else M['red']

# ---------------------------------------------------------------- geometry helpers
_created = []
def _link(o, mat):
    scene.collection.objects.link(o)
    if mat is not None:
        o.data.materials.append(mat)
    _created.append(o)
    return o

def assign_uv(me):
    uvl = me.uv_layers.new(name='UVMap') if not me.uv_layers else me.uv_layers[0]
    rnd = random.Random(len(me.vertices) * 131 + (int(me.vertices[0].co.x * 997) if len(me.vertices) else 0) + len(_created))
    ou, ov = rnd.random() * 7, rnd.random() * 7
    for p in me.polygons:
        n = p.normal
        t = Vector((1, 0, 0)) if abs(n.z) > 0.92 else Vector((0, 0, 1)).cross(n)
        t.normalize()
        b = n.cross(t)
        for li in p.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            uvl.data[li].uv = (co.dot(t) + ou, co.dot(b) + ov)

def mesh_obj(name, verts, faces, mat):
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    me.update()
    assign_uv(me)
    for p in me.polygons:
        p.use_smooth = False
    return _link(bpy.data.objects.new(name, me), mat)

def box(x, y, z, sx, sy, sz, mat, rz=0.0, name='box'):
    """box centred in x,y; bottom at z"""
    hx, hy = sx / 2, sy / 2
    v = [(-hx, -hy, 0), (hx, -hy, 0), (hx, hy, 0), (-hx, hy, 0), (-hx, -hy, sz), (hx, -hy, sz), (hx, hy, sz), (-hx, hy, sz)]
    f = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    o.rotation_euler = (0, 0, rz)
    return o

def cyl(x, y, z, r, h, mat, verts=14, r2=None, rx=0.0, ry=0.0, rz=0.0, name='cyl'):
    r2 = r if r2 is None else r2
    v, f = [], []
    for i in range(verts):
        a = 2 * math.pi * i / verts
        v.append((math.cos(a) * r, math.sin(a) * r, 0))
    for i in range(verts):
        a = 2 * math.pi * i / verts
        v.append((math.cos(a) * r2, math.sin(a) * r2, h))
    for i in range(verts):
        j = (i + 1) % verts
        f.append((i, j, verts + j, verts + i))
    f.append(tuple(reversed(range(verts))))
    f.append(tuple(range(verts, 2 * verts)))
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    o.rotation_euler = (rx, ry, rz)
    return o

def cone(x, y, z, r, h, mat, verts=12, name='cone'):
    return cyl(x, y, z, r, h, mat, verts=verts, r2=0.001, name=name)

def sphere(x, y, z, r, mat, sx=1, sy=1, sz=1, seg=10, name='sph', noise=0.0, seed=0):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=seg, v_segments=max(5, seg // 2 + 1), radius=r)
    if noise:
        rnd = random.Random(seed)
        for vv in bm.verts:
            k = 1 + rnd.uniform(-noise, noise)
            vv.co *= k
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    assign_uv(me)
    o = _link(bpy.data.objects.new(name, me), mat)
    o.location = (x, y, z)
    o.scale = (sx, sy, sz)
    return o

def prism_roof(x, y, z, sx, sy, h, mat, over=0.14, along='x', name='roof', ridge_ext=0.0):
    """gable roof, ridge along X (default) or Y; sx,sy = wall footprint"""
    hx, hy = sx / 2 + over, sy / 2 + over
    if along == 'x':
        v = [(-hx - ridge_ext, -hy, 0), (hx + ridge_ext, -hy, 0), (hx + ridge_ext, hy, 0), (-hx - ridge_ext, hy, 0), (-hx - ridge_ext, 0, h), (hx + ridge_ext, 0, h)]
        f = [(0, 1, 5, 4), (2, 3, 4, 5), (0, 4, 3), (1, 2, 5), (0, 3, 2, 1)]
    else:
        v = [(-hx, -hy - ridge_ext, 0), (hx, -hy - ridge_ext, 0), (hx, hy + ridge_ext, 0), (-hx, hy + ridge_ext, 0), (0, -hy - ridge_ext, h), (0, hy + ridge_ext, h)]
        f = [(0, 4, 5, 3), (1, 2, 5, 4), (0, 1, 4), (3, 5, 2), (0, 3, 2, 1)]
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    return o

def hip_roof(x, y, z, sx, sy, h, mat, over=0.14, top=0.0, name='hip'):
    hx, hy = sx / 2 + over, sy / 2 + over
    t = top / 2
    v = [(-hx, -hy, 0), (hx, -hy, 0), (hx, hy, 0), (-hx, hy, 0), (-t, -t, h), (t, -t, h), (t, t, h), (-t, t, h)]
    f = [(0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7), (4, 5, 6, 7), (0, 3, 2, 1)]
    o = mesh_obj(name, v, f, mat)
    o.location = (x, y, z)
    return o

def plane(x, y, z, sx, sy, mat, rz=0.0, name='plane'):
    hx, hy = sx / 2, sy / 2
    o = mesh_obj(name, [(-hx, -hy, 0), (hx, -hy, 0), (hx, hy, 0), (-hx, hy, 0)], [(0, 1, 2, 3)], mat)
    o.location = (x, y, z)
    o.rotation_euler = (0, 0, rz)
    return o

def flag(x, y, z, h, team, w=0.42, ht=0.26, name='flag'):
    cyl(x, y, z, 0.022, h, M['woodDark'], verts=6, name=name + '_pole')
    mat = team_mat(team)
    v = [(0, 0, 0), (w, 0.0, -0.03), (w * 0.85, 0, -ht / 2), (w, 0, -ht), (0, 0, -ht)]
    f = [(0, 1, 2, 3, 4)]
    o = mesh_obj(name + '_cloth', v, f, mat)
    o.location = (x, y, z + h)
    o.rotation_euler = (0, 0, math.radians(-35))
    # cloth is single-sided: add a back face
    o.data.polygons.foreach_set('use_smooth', [False])
    o2 = mesh_obj(name + '_cloth2', v, [(4, 3, 2, 1, 0)], mat)
    o2.location = o.location
    o2.rotation_euler = o.rotation_euler
    return o

def palisade(cx, cy, hx, hy, gap=None, mat=None, post_r=0.07, spacing=0.17, hgt=0.55):
    mat = mat or M['wood']
    n = int(2 * hx / spacing)
    for side in (-1, 1):
        for i in range(n + 1):
            px = -hx + i * spacing
            if gap and gap[0] == 'y' and side == gap[1] and abs(px) < gap[2]:
                continue
            cyl(cx + px, cy + side * hy, 0, post_r, hgt + 0.06 * ((i * 7) % 3), mat, verts=6)
    n = int(2 * hy / spacing)
    for side in (-1, 1):
        for i in range(n + 1):
            py = -hy + i * spacing
            if gap and gap[0] == 'x' and side == gap[1] and abs(py) < gap[2]:
                continue
            cyl(cx + side * hx, cy + py, 0, post_r, hgt + 0.06 * ((i * 5) % 3), mat, verts=6)

def clear_models():
    global _created
    for o in _created:
        try:
            bpy.data.objects.remove(o, do_unlink=True)
        except Exception:
            pass
    _created = []
    for me in list(bpy.data.meshes):
        if me.users == 0:
            bpy.data.meshes.remove(me)

def crenellations(x, y, z, sx, sy, mat, n=6, hgt=0.12):
    hx, hy = sx / 2, sy / 2
    for i in range(n):
        t = -hx + (i + 0.5) * sx / n
        if i % 2 == 0:
            box(x + t, y - hy + 0.04, z, sx / n * 0.8, 0.08, hgt, mat)
            box(x + t, y + hy - 0.04, z, sx / n * 0.8, 0.08, hgt, mat)
    for i in range(n):
        t = -hy + (i + 0.5) * sy / n
        if i % 2 == 0:
            box(x - hx + 0.04, y + t, z, 0.08, sy / n * 0.8, hgt, mat)
            box(x + hx - 0.04, y + t, z, 0.08, sy / n * 0.8, hgt, mat)

# ---------------------------------------------------------------- render
MANIFEST = {}
def render(name, W, H, oy, target=(0, 0, 0), ox=None, pxm=None):
    ox = W / 2 if ox is None else ox
    place_camera(W, H, ox, oy, target, pxm)
    path = os.path.join(OUT, name + '.png')
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    MANIFEST[name] = {'w': int(W), 'h': int(H), 'ax': float(ox), 'ay': float(oy), 'pxm': float(pxm or PXM)}
    print('rendered', name)

def sprite_canvas(fw, fh, up):
    """canvas for a footprint fw x fh (tiles) and `up` metres of height. returns W,H,oy"""
    half_w = (fw + fh) / 2 * math.sqrt(2) / 2 * PXM * 1.0     # screen half width of the footprint diamond
    half_h = half_w / 2
    W = int(math.ceil(half_w * 2 + 1.5 * PXM)) // 2 * 2
    top = up * math.cos(ELEV) * PXM + 0.35 * PXM
    H = int(math.ceil(top + half_h + 0.75 * PXM)) // 2 * 2
    oy = top
    return W, H, oy

def should(name):
    return not ONLY or name in ONLY or any(name.startswith(o) for o in ONLY)


# ================================================================= job list
HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else os.path.abspath('art')
exec(open(os.path.join(HERE, 'models_buildings.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_world.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_castle.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_units2.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_imperial.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_walls.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_scorpion.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_detail.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_v2.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_nature.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_units_v3.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_icons.py'), encoding='utf-8').read(), globals())
exec(open(os.path.join(HERE, 'models_explore.py'), encoding='utf-8').read(), globals())

import zlib
NODECOR = ('deer', 'boar', 'wolf', 'chest', 'wall_', 'gate_', 'spike', 'oil', 'wallrubble', 'oak', 'pine', 'palm', 'node_', 'bush', 'tuft', 'rock', 'flower', 'rubble', 'scaffold', 'stump', 'decor', 'ground', 'farm')
def do_building(name, fw, fh, up, fn, *a):
    if not should(name):
        return
    clear_models()
    fn(*a)
    if not name.startswith(NODECOR):
        decorate(name, fw, fh, zlib.crc32(name.encode()) % 100000)
    W, H, oy = sprite_canvas(fw, fh, up)
    render(name, W, H, oy)

def do_unit_all():
    kinds = [('villager', ['blue'], 3), ('militia', ['blue', 'red'], 3), ('manatarms', ['blue', 'red'], 3), ('archer', ['blue', 'red'], 3),
             ('longsword', ['blue', 'red'], 3), ('crossbow', ['blue', 'red'], 3), ('knight', ['blue', 'red'], 3), ('monk', ['blue', 'red'], 3),
             ('ram', ['blue', 'red'], 1), ('mangonel', ['blue', 'red'], 1),
             ('champion', ['blue', 'red'], 3), ('arbalester', ['blue', 'red'], 3), ('paladin', ['blue', 'red'], 3), ('handcannon', ['blue', 'red'], 3),
             ('trebuchet', ['blue', 'red'], 1), ('bombard', ['blue', 'red'], 1)]
    big = ('knight', 'ram', 'mangonel', 'paladin', 'bombard')
    for kind, teams, nfr in kinds:
        for team in teams:
            for d in range(8):
                for fr in range(nfr):
                    nm = 'u_%s_%s_%d_%d' % (kind, team, d, fr)
                    if not should(nm) and not should('u_%s_%s' % (kind, team)) and not should('units') and not should('u_' + kind):
                        continue
                    clear_models()
                    if kind in ('villager', 'militia', 'manatarms', 'archer'):
                        build_unit(kind, team, d, fr)
                    elif kind in ('champion', 'arbalester', 'paladin', 'handcannon', 'trebuchet', 'bombard'):
                        build_unit3(kind, team, d, fr)
                    else:
                        build_unit2(kind, team, d, fr)
                    if kind == 'trebuchet':
                        render(nm, 160, 160, 124)
                    elif kind in big:
                        render(nm, 128, 128, 100)
                    else:
                        render(nm, 96, 104, 80)

JOBS = []
for st in (0, 1, 2):
    do_building('house_%d' % st, 2, 2, 1.8, m_house, st)
    do_building('lumber_%d' % st, 2, 2, 1.3, m_lumber, st)
    do_building('goldcamp_%d' % st, 2, 2, 1.3, m_mine, st, 'gold')
    do_building('stonecamp_%d' % st, 2, 2, 1.3, m_mine, st, 'stone')
    do_building('farm_%d' % st, 2, 2, 0.8, m_farm, st)
    do_building('barracks_%d' % st, 3, 3, 2.1, m_barracks, st)
    do_building('armycamp_%d' % st, 3, 3, 1.6, m_armycamp, st)
    do_building('archery_%d' % st, 3, 3, 1.7, m_archery, st)
for lv in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10):
    do_building('townhall_%d' % lv, 3, 3, 3.0 + 0.3 * max(0, lv - 3), m_townhall, lv)
for lv in (1, 2, 3):
    do_building('tower_%d' % lv, 2, 2, 3.4, m_tower, lv)
do_building('tower_4', 2, 2, 4.2, m_tower4)
do_building('stable_2', 3, 3, 2.0, m_stable, 2)
do_building('siege_2', 3, 3, 2.0, m_siege, 2)
do_building('monastery_2', 3, 3, 3.2, m_monastery, 2)
do_building('keep_2', 2, 2, 3.4, m_keep, 2)
for st in (1, 2):
    do_building('blacksmith_%d' % st, 2, 2, 2.2, m_blacksmith, st)
    do_building('market_%d' % st, 3, 3, 1.8, m_market, st)
do_building('castle_2', 4, 4, 4.0, m_castle, 2)
do_building('bombard_3', 2, 2, 3.2, m_bombard)
do_building('scorpion_2', 2, 2, 1.8, m_scorpion, 2)
def do_imperial(name, fw, fh, up, fn, *a):
    if not should(name):
        return
    saved = {k: M[k] for k in ('plaster', 'stone', 'slate', 'tile', 'thatch')}
    M['plaster'], M['stone'], M['slate'], M['tile'], M['thatch'] = M['marblePlaster'], M['marble'], M['imperial'], M['imperial'], M['imperial']
    try:
        do_building(name, fw, fh, up, fn, *a)
    finally:
        for k, v in saved.items():
            M[k] = v
do_imperial('house_3', 2, 2, 1.8, m_house, 3)
do_imperial('lumber_3', 2, 2, 1.3, m_lumber, 3)
do_imperial('goldcamp_3', 2, 2, 1.3, m_mine, 3, 'gold')
do_imperial('stonecamp_3', 2, 2, 1.3, m_mine, 3, 'stone')
do_imperial('farm_3', 2, 2, 0.8, m_farm, 3)
do_imperial('barracks_3', 3, 3, 2.1, m_barracks, 3)
do_imperial('archery_3', 3, 3, 1.7, m_archery, 3)
do_imperial('stable_3', 3, 3, 2.0, m_stable, 3)
do_imperial('siege_3', 3, 3, 2.0, m_siege, 3)
do_imperial('monastery_3', 3, 3, 3.2, m_monastery, 3)
do_imperial('keep_3', 2, 2, 3.4, m_keep, 3)
do_imperial('blacksmith_3', 2, 2, 2.2, m_blacksmith, 3)
do_imperial('market_3', 3, 3, 1.8, m_market, 3)
do_imperial('castle_3', 4, 4, 4.0, m_castle, 3)
do_imperial('university_3', 3, 3, 3.3, m_university, 3)
do_building('university_2', 3, 3, 3.3, m_university, 2)
do_building('storehouse_0', 2, 2, 1.4, m_storehouse)
do_building('mill_0', 2, 2, 2.4, m_mill)
for _an in ('deer', 'boar', 'wolf'):
    for _k in range(4):
        if should('%s_%d' % (_an, _k)):
            W_, H_, oy_ = sprite_canvas(1, 1, 1.3)
            m_animal(_an, _k)
            render('%s_%d' % (_an, _k), W_, H_, oy_)
do_building('chest_0', 1, 1, 0.7, m_chest)
do_building('tent_0', 2, 2, 1.4, m_tent)
do_building('chief_0', 3, 3, 2.4, m_chief)
for n in (2, 3, 4):
    do_building('scaffold_%d' % n, n, n, 1.3, m_scaffold, n)
    do_building('rubble_%d' % n, n, n, 0.9, m_rubble, n)
for k in range(4):
    do_building('oak_%d' % k, 1, 1, 2.5, m_oak, k)
for k in range(3):
    do_building('pine_%d' % k, 1, 1, 2.9, m_pine, k)
for k in range(2):
    do_building('palm_%d' % k, 1, 1, 2.7, m_palm, k)
for k in range(3):
    do_building('bush_%d' % k, 1, 1, 0.6, m_bush, k)
    do_building('tuft_%d' % k, 1, 1, 0.5, m_tuft, k)
    do_building('flowers_%d' % k, 1, 1, 0.5, m_flowers, k)
    do_building('rocks_%d' % k, 1, 1, 0.5, m_rocks, k)
do_building('stump_0', 1, 1, 0.5, m_stump)
do_building('node_gold', 1, 1, 0.8, m_goldnode)
do_building('node_stone', 1, 1, 0.8, m_stonenode)
def with_marble(fn):
    def run(*a):
        saved = {k: M[k] for k in ('plaster', 'stone', 'slate', 'tile')}
        M['stone'], M['plaster'] = M['marble'], M['marblePlaster']
        try:
            fn(*a)
        finally:
            for k, v in saved.items():
                M[k] = v
    return run
for st in range(4):
    fn = with_marble(m_wall) if st == 3 else m_wall
    for mask in range(16):
        do_building('wall_%d_%d' % (st, mask), 1, 1, 1.5, fn, st, mask)
    gf = with_marble(m_gate) if st == 3 else m_gate
    for axis in ('x', 'y'):
        do_building('gate_%d_%s' % (st, axis), 1, 1, 1.9, gf, st, axis)
do_building('spike_0', 1, 1, 0.5, m_spike)
do_building('oil_0', 1, 1, 0.6, m_oil)
do_building('wallrubble_0', 1, 1, 0.5, m_wallrubble)
for _k in UNIT_KINDS:
    if should('units') or should('u_' + _k) or any(o.startswith('u_' + _k) or o.startswith('uw_') for o in ONLY):
        render_unit_kind(_k)

for _nm, _fn, _z, _sc in ICON_JOBS:
    render_icon(_nm, _fn, _z, _sc)

if should('ground'):
    clear_models()
    build_ground2()
    PXM_G = 70.0                      # px per metre -> 1.547 sprite px per game px
    FW, FH, CW, CH = 3072, 1600, 1536, 800
    oyf = 780                         # pixel row (full image) of the map-centre grid point
    for ci in range(2):
        for cj in range(2):
            render('ground_%d_%d' % (ci, cj), CW, CH, oyf - cj * CH, target=(15, -15, 0), ox=FW / 2 - ci * CW, pxm=PXM_G)

mf = os.path.join(OUT, 'manifest.json')
old = {}
if os.path.exists(mf):
    try:
        old = json.load(open(mf))
    except Exception:
        old = {}
old.update(MANIFEST)
json.dump(old, open(mf, 'w'), indent=1)
print('DONE', len(MANIFEST), 'sprites')
