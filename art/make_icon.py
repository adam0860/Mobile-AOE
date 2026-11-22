# Renders the game icon emblem (crowned heater shield over crossed swords) with a transparent background.
#   blender -b -P art/make_icon.py -- <absolute output png>
import bpy, bmesh, math, sys
from mathutils import Vector

out = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else 'emblem.png'
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'; sc.cycles.samples = 160; sc.cycles.use_denoising = True
try: sc.cycles.device = 'CPU'
except Exception: pass
sc.render.film_transparent = True
sc.render.resolution_x = sc.render.resolution_y = 1024
sc.render.image_settings.file_format = 'PNG'; sc.render.image_settings.color_mode = 'RGBA'
try: sc.view_settings.view_transform = 'Standard'
except Exception: pass

def mat(name, col, metal=0.0, rough=0.5, emit=0.0):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*col, 1); b.inputs['Metallic'].default_value = metal; b.inputs['Roughness'].default_value = rough
    if emit:
        for k in ('Emission Color', 'Emission'):
            if k in b.inputs: b.inputs[k].default_value = (*col, 1); break
        if 'Emission Strength' in b.inputs: b.inputs['Emission Strength'].default_value = emit
    return m
GOLD = mat('gold', (1.0, 0.62, 0.1), 1.0, 0.26); GOLD2 = mat('gold2', (0.8, 0.42, 0.05), 1.0, 0.34)
BLUE = mat('blue', (0.015, 0.05, 0.36), 0.0, 0.3); BLUE2 = mat('blue2', (0.03, 0.12, 0.6), 0.0, 0.28)
RED = mat('red', (0.62, 0.02, 0.04), 0.0, 0.3); WHITE = mat('white', (0.85, 0.85, 0.8), 0.0, 0.4)
STEEL = mat('steel', (0.78, 0.82, 0.9), 0.65, 0.2); LEATHER = mat('leather', (0.22, 0.06, 0.03), 0.0, 0.5)
GEM = mat('gem', (0.75, 0.0, 0.04), 0.0, 0.06, 0.6); GEM2 = mat('gem2', (0.02, 0.28, 0.9), 0.0, 0.06, 0.5); PEARL = mat('pearl', (0.95, 0.93, 0.88), 0.0, 0.1)

def obj_from(name, verts, faces, m, smooth=False):
    me = bpy.data.meshes.new(name); me.from_pydata(verts, [], faces); me.update()
    o = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(o); o.data.materials.append(m)
    for p in me.polygons: p.use_smooth = smooth
    return o
def box(x, y, z, sx, sy, sz, m, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(x, y, z)); o = bpy.context.object; o.scale = (sx, sy, sz); o.rotation_euler = rot; o.data.materials.append(m); return o
def cyl(x, y, z, r, h, m, rot=(0, 0, 0), verts=32, r2=None):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r, radius2=r if r2 is None else r2, depth=h, location=(x, y, z)); o = bpy.context.object; o.rotation_euler = rot; o.data.materials.append(m)
    bpy.ops.object.shade_smooth(); return o
def sph(x, y, z, r, m, sc=(1, 1, 1)):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, segments=32, ring_count=16, location=(x, y, z)); o = bpy.context.object; o.scale = sc; o.data.materials.append(m)
    bpy.ops.object.shade_smooth(); return o
def bevel(o, w=0.02, seg=4):
    md = o.modifiers.new('bv', 'BEVEL'); md.width = w; md.segments = seg; md.limit_method = 'ANGLE'
def extrude_poly(pts, depth, m, y0=0.0):
    bm = bmesh.new(); vs = [bm.verts.new((x, y0, z)) for x, z in pts]
    f = bm.faces.new(vs); r = bmesh.ops.extrude_face_region(bm, geom=[f]); vv = [v for v in r['geom'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=(0, depth, 0), verts=vv); bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    me = bpy.data.meshes.new('ext'); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new('ext', me); bpy.context.scene.collection.objects.link(o); o.data.materials.append(m); return o

def shield_outline(scale):
    right = [(0.0 + 0.66 * (i / 6.0), 0.62) for i in range(0, 7)]                  # flat top
    for i in range(1, 25):                                                       # sides sweeping down to the point
        t = i / 24.0; right.append((0.66 * (1 - t ** 2.3) ** 0.62, 0.62 - 1.55 * t))
    left = [(-x, z) for (x, z) in reversed(right[1:])]
    return [(x * scale, z * scale) for (x, z) in right + left]

root = bpy.data.objects.new('emblem', None); bpy.context.scene.collection.objects.link(root)

def sword(angle):
    g = bpy.data.objects.new('sw', None); bpy.context.scene.collection.objects.link(g)
    bl = obj_from('blade', [(-0.1, 0, -0.5), (0.1, 0, -0.5), (0.1, 0, 1.55), (0, 0, 1.85), (-0.1, 0, 1.55), (-0.1, 0.07, 1.55), (0, 0.1, 1.85), (0.1, 0.07, 1.55), (0.1, 0.07, -0.5), (-0.1, 0.07, -0.5)],
                  [(0, 1, 2, 3, 4), (9, 8, 7, 6, 5), (0, 9, 5, 4), (1, 8, 9, 0), (2, 7, 8, 1), (3, 6, 7, 2), (4, 5, 6, 3)], STEEL); bl.parent = g
    fu = box(0, -0.01, 0.55, 0.035, 0.02, 1.9, GOLD2); fu.parent = g
    cg = box(0, 0.035, -0.55, 0.78, 0.13, 0.13, GOLD); cg.parent = g; bevel(cg, 0.04)
    for s in (-1, 1): sp = sph(s * 0.39, 0.035, -0.55, 0.09, GOLD); sp.parent = g
    gr = cyl(0, 0.035, -0.82, 0.055, 0.42, LEATHER); gr.parent = g
    pm = sph(0, 0.035, -1.06, 0.12, GOLD); pm.parent = g
    g.rotation_euler = (0, math.radians(angle), 0); g.location = (0, 0.3, 0.05); g.parent = root
sword(38); sword(-38)

rim = extrude_poly(shield_outline(1.0), 0.2, GOLD, 0.0); rim.parent = root; rim.location = (0, -0.22, -0.1); bevel(rim, 0.03, 5)
field = extrude_poly(shield_outline(0.87), 0.08, BLUE, 0.0); field.parent = root; field.location = (0, -0.32, -0.1)
inner = extrude_poly(shield_outline(0.8), 0.04, BLUE2, 0.0); inner.parent = root; inner.location = (0, -0.38, -0.1)
# heraldry: gold chevron, white sword-tower line and stars
for s in (-1, 1):
    ch = box(s * 0.25, -0.43, -0.12, 0.62, 0.05, 0.17, GOLD, rot=(0, math.radians(s * 38), 0)); ch.parent = root; bevel(ch, 0.015)
tw = box(0, -0.44, 0.22, 0.36, 0.05, 0.3, WHITE); tw.parent = root; bevel(tw, 0.015)
for sx in (-1, 0, 1):
    cr = box(sx * 0.13, -0.44, 0.42, 0.09, 0.05, 0.1, WHITE); cr.parent = root
gt = box(0, -0.455, 0.14, 0.12, 0.04, 0.2, BLUE); gt.parent = root
for (x, z) in ((-0.38, 0.38), (0.38, 0.38), (0.0, -0.62)):
    st = cyl(x, -0.43, z, 0.095, 0.045, GOLD, rot=(math.pi / 2, 0, 0), verts=5); st.parent = root
# crown: flat zig-zag outline extruded, band, pearls and gems
pts = [(-0.52, 0.0), (0.52, 0.0), (0.52, 0.2)]
spikes = [(0.44, 0.46), (0.22, 0.36), (0.0, 0.58), (-0.22, 0.36), (-0.44, 0.46)]
for i, (x, h) in enumerate(spikes):
    pts.append((x + 0.09, 0.2)) if False else None
    pts.append((x + 0.085, 0.24)); pts.append((x, h)); pts.append((x - 0.085, 0.24))
pts.append((-0.52, 0.2))
crown = extrude_poly(pts, 0.16, GOLD, 0.0); crown.parent = root; crown.location = (0, -0.4, 0.74); bevel(crown, 0.018, 4)
bandb = box(0, -0.425, 0.74 + 0.1, 1.05, 0.04, 0.2, GOLD2); bandb.parent = root; bevel(bandb, 0.01)
for i, (x, h) in enumerate(spikes):
    pr = sph(x, -0.32, 0.74 + h + 0.04, 0.062 if i != 2 else 0.085, PEARL if i != 2 else GEM); pr.parent = root
for i, x in enumerate((-0.32, -0.16, 0.0, 0.16, 0.32)):
    gm = sph(x, -0.5, 0.74 + 0.1, 0.05, GEM if i % 2 == 0 else GEM2, (1, 0.55, 1.1)); gm.parent = root
root.rotation_euler = (math.radians(-6), 0, 0)

for name, loc, energy, size in (('key', (-3, -5, 4.5), 330, 3.5), ('fill', (4, -4, 1), 110, 4), ('rim', (0, 3, 4), 230, 3), ('under', (0, -4, -3), 70, 4)):
    bpy.ops.object.light_add(type='AREA', location=loc); l = bpy.context.object; l.data.energy = energy; l.data.size = size
    d = (Vector((0, 0, 0.1)) - l.location); l.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
w = bpy.data.worlds.new('w'); sc.world = w; w.use_nodes = True; w.node_tree.nodes['Background'].inputs['Color'].default_value = (0.45, 0.5, 0.65, 1); w.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.22
bpy.ops.object.camera_add(location=(0, -9, 0.08)); cam = bpy.context.object; cam.data.type = 'ORTHO'; cam.data.ortho_scale = 3.5; cam.rotation_euler = (math.radians(90), 0, 0); sc.camera = cam
sc.render.filepath = out; bpy.ops.render.render(write_still=True); print('icon rendered', out)
