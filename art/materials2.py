# Material library v2 — hand-painted look (exec'd by gen_assets.py).
# All textures use UV coordinates in metres (assign_uv in gen_assets.py), so planks, shingles and masonry course correctly on every face.

def _mk(name):
    m = bpy.data.materials.new(name)
    try:
        m.use_nodes = True
    except Exception:
        pass
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    for nm in ('Specular IOR Level', 'Specular'):
        if nm in bsdf.inputs:
            bsdf.inputs[nm].default_value = 0.12
            break
    tc = nt.nodes.new('ShaderNodeTexCoord')
    return m, nt, bsdf, tc.outputs['UV'], tc.outputs['Object']

def _map(nt, vec, scale=(1, 1, 1), loc=(0, 0, 0)):
    mp = nt.nodes.new('ShaderNodeMapping')
    mp.inputs['Scale'].default_value = scale
    mp.inputs['Location'].default_value = loc
    nt.links.new(vec, mp.inputs['Vector'])
    return mp.outputs['Vector']

def _noise(nt, vec, scale, detail=4, rough=0.55, stretch=(1, 1, 1)):
    v = _map(nt, vec, stretch) if stretch != (1, 1, 1) else vec
    n = nt.nodes.new('ShaderNodeTexNoise')
    n.inputs['Scale'].default_value = scale
    n.inputs['Detail'].default_value = detail
    n.inputs['Roughness'].default_value = rough
    nt.links.new(v, n.inputs['Vector'])
    return n.outputs['Fac']

def _ramp(nt, fac, stops, const=False):
    r = nt.nodes.new('ShaderNodeValToRGB')
    el = r.color_ramp.elements
    while len(el) < len(stops):
        el.new(0.5)
    for i, (p, c) in enumerate(stops):
        el[i].position = p
        el[i].color = srgb(c)
    if const:
        r.color_ramp.interpolation = 'CONSTANT'
    nt.links.new(fac, r.inputs['Fac'])
    return r.outputs['Color']

def _mixc(nt, a, b, fac):
    m = nt.nodes.new('ShaderNodeMix')
    m.data_type = 'RGBA'
    if isinstance(fac, (int, float)):
        m.inputs[0].default_value = fac
    else:
        nt.links.new(fac, m.inputs[0])
    nt.links.new(a, m.inputs[6])
    nt.links.new(b, m.inputs[7])
    return m.outputs[2]

def _mulc(nt, a, b, fac=1.0):
    m = nt.nodes.new('ShaderNodeMix')
    m.data_type = 'RGBA'
    m.blend_type = 'MULTIPLY'
    m.inputs[0].default_value = fac
    nt.links.new(a, m.inputs[6])
    nt.links.new(b, m.inputs[7])
    return m.outputs[2]

def _brick(nt, vec, c1, c2, mortar, w, h, msize=0.012, offset=0.5, bias=0.0, smooth=0.1):
    b = nt.nodes.new('ShaderNodeTexBrick')
    b.offset = offset
    b.offset_frequency = 2
    b.inputs['Color1'].default_value = srgb(c1)
    b.inputs['Color2'].default_value = srgb(c2)
    b.inputs['Mortar'].default_value = srgb(mortar)
    b.inputs['Scale'].default_value = 1.0
    b.inputs['Mortar Size'].default_value = msize
    b.inputs['Mortar Smooth'].default_value = smooth
    b.inputs['Bias'].default_value = bias
    b.inputs['Brick Width'].default_value = w
    b.inputs['Row Height'].default_value = h
    nt.links.new(vec, b.inputs['Vector'])
    return b.outputs['Color'], b.outputs['Fac']

def _bump(nt, height, normal_in=None, strength=0.3, dist=0.03):
    bn = nt.nodes.new('ShaderNodeBump')
    bn.inputs['Strength'].default_value = strength
    bn.inputs['Distance'].default_value = dist
    nt.links.new(height, bn.inputs['Height'])
    if normal_in is not None:
        nt.links.new(normal_in, bn.inputs['Normal'])
    return bn.outputs['Normal']

def _finish(nt, bsdf, col, normal=None, rough=0.9, ao=True, ao_dist=0.35, metal=0.0, emit=None):
    if ao:
        a = nt.nodes.new('ShaderNodeAmbientOcclusion')
        a.inputs['Distance'].default_value = ao_dist
        a.samples = 8
        nt.links.new(col, a.inputs['Color'])
        col = a.outputs['Color']
    nt.links.new(col, bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    if normal is not None:
        nt.links.new(normal, bsdf.inputs['Normal'])
    if emit is not None:
        nt.links.new(col, bsdf.inputs['Emission Color'] if 'Emission Color' in bsdf.inputs else bsdf.inputs['Emission'])
        if 'Emission Strength' in bsdf.inputs:
            bsdf.inputs['Emission Strength'].default_value = emit

_cache2 = {}
def mat2(name, fn):
    if name in _cache2:
        return _cache2[name]
    m, nt, bsdf, uv, obj = _mk(name)
    fn(nt, bsdf, uv, obj)
    _cache2[name] = m
    return m

# ------------------------------------------------------------------ builders
def planks(c1, c2, w=1.5, h=0.1, grain=0.35, mortar='#2b1c10'):
    def f(nt, bsdf, uv, obj):
        col, fac = _brick(nt, uv, c1, c2, mortar, w, h, 0.010, 0.31, 0.2)
        g = _noise(nt, uv, 7, 3, 0.6, (0.5, 18, 1))
        gc = _ramp(nt, g, [(0.3, '#c8c8c8'), (0.7, '#ffffff')])
        col = _mulc(nt, col, gc, 0.9)
        n = _bump(nt, fac, None, 0.5, 0.02)
        n = _bump(nt, g, n, grain, 0.02)
        _finish(nt, bsdf, col, n, 0.85)
    return f

def masonry(c1, c2, mortar, w=0.42, h=0.21, grime=True):
    def f(nt, bsdf, uv, obj):
        col, fac = _brick(nt, uv, c1, c2, mortar, w, h, 0.016, 0.5, 0.0)
        v = _noise(nt, uv, 3.0, 5, 0.6)
        vr = _ramp(nt, v, [(0.35, '#b8b8b8'), (0.65, '#ffffff')])
        col = _mulc(nt, col, vr, 0.8)
        if grime:
            gz = _noise(nt, obj, 2.2, 4, 0.6, (0.6, 0.6, 3.0))
            gr = _ramp(nt, gz, [(0.4, '#7a7468'), (0.7, '#ffffff')])
            col = _mulc(nt, col, gr, 0.45)
        n = _bump(nt, fac, None, 0.7, 0.03)
        n = _bump(nt, v, n, 0.25, 0.02)
        _finish(nt, bsdf, col, n, 0.92)
    return f

def shingles(c1, c2, mortar, w=0.13, h=0.085, vary='#ffffff'):
    def f(nt, bsdf, uv, obj):
        col, fac = _brick(nt, uv, c1, c2, mortar, w, h, 0.010, 0.5, 0.0)
        v = _noise(nt, uv, 6, 3, 0.55)
        vr = _ramp(nt, v, [(0.3, '#bdbdbd'), (0.7, '#ffffff')])
        col = _mulc(nt, col, vr, 0.55)
        n = _bump(nt, fac, None, 0.9, 0.025)
        _finish(nt, bsdf, col, n, 0.78, True, 0.25)
    return f

def thatch(c1, c2, dark):
    def f(nt, bsdf, uv, obj):
        s1 = _noise(nt, uv, 14, 6, 0.62, (0.35, 6.5, 1))            # straw bundles run down the slope (v)
        s2 = _noise(nt, uv, 40, 3, 0.5, (0.25, 5.5, 1))
        col = _ramp(nt, s1, [(0.25, dark), (0.5, c1), (0.78, c2)])
        fine = _ramp(nt, s2, [(0.3, '#b0a070'), (0.7, '#ffffff')])
        col = _mulc(nt, col, fine, 0.7)
        n = _bump(nt, s1, None, 0.9, 0.04)
        n = _bump(nt, s2, n, 0.6, 0.02)
        _finish(nt, bsdf, col, n, 0.95, True, 0.4)
    return f

def stucco(c1, c2, stain):
    def f(nt, bsdf, uv, obj):
        big = _noise(nt, uv, 1.4, 4, 0.6)
        fine = _noise(nt, uv, 26, 5, 0.6)
        col = _ramp(nt, fine, [(0.3, c1), (0.7, c2)])
        st = _ramp(nt, big, [(0.38, '#ffffff'), (0.72, stain)])
        col = _mulc(nt, col, st, 0.9)
        gz = _noise(nt, obj, 3.0, 3, 0.6, (1, 1, 2.5))
        gr = _ramp(nt, gz, [(0.45, '#8f8570'), (0.75, '#ffffff')])
        col = _mulc(nt, col, gr, 0.35)
        n = _bump(nt, fine, None, 0.35, 0.02)
        _finish(nt, bsdf, col, n, 0.93, True, 0.45)
    return f

def earth(c1, c2, pebbles=True):
    def f(nt, bsdf, uv, obj):
        big = _noise(nt, obj, 2.5, 5, 0.65)
        fine = _noise(nt, obj, 30, 5, 0.6)
        col = _ramp(nt, big, [(0.3, c1), (0.7, c2)])
        fr = _ramp(nt, fine, [(0.3, '#b0a090'), (0.7, '#ffffff')])
        col = _mulc(nt, col, fr, 0.6)
        n = _bump(nt, fine, None, 0.6, 0.03)
        _finish(nt, bsdf, col, n, 0.97, True, 0.3)
    return f

def foliage(dark, mid, light, scale=5.0):
    def f(nt, bsdf, uv, obj):
        n1 = _noise(nt, obj, scale, 5, 0.6)
        n2 = _noise(nt, obj, scale * 3.5, 4, 0.55)
        col = _ramp(nt, n1, [(0.25, dark), (0.5, mid), (0.78, light)])
        hi = _ramp(nt, n2, [(0.35, '#d0d0d0'), (0.7, '#ffffff')])
        col = _mulc(nt, col, hi, 0.7)
        # lighter at the top of the canopy (painted light from above)
        gz = nt.nodes.new('ShaderNodeSeparateXYZ'); nt.links.new(obj, gz.inputs['Vector'])
        zr = _ramp(nt, gz.outputs['Z'], [(0.0, '#9aa98a'), (1.4, '#ffffff')])
        col = _mulc(nt, col, zr, 0.65)
        n = _bump(nt, n2, None, 0.7, 0.05)
        _finish(nt, bsdf, col, n, 0.85, True, 0.5)
    return f

def solid(c, rough=0.8, metal=0.0, spec=None, ao=True):
    def f(nt, bsdf, uv, obj):
        node = nt.nodes.new('ShaderNodeRGB')
        node.outputs[0].default_value = srgb(c)
        _finish(nt, bsdf, node.outputs[0], None, rough, ao, 0.25, metal)
    return f

def cloth(c1, c2, scale=40, rough=0.9):
    def f(nt, bsdf, uv, obj):
        w = nt.nodes.new('ShaderNodeTexWave'); w.wave_type = 'BANDS'; w.inputs['Scale'].default_value = scale
        w.inputs['Distortion'].default_value = 0.3
        nt.links.new(_map(nt, uv, (1, 1, 1)), w.inputs['Vector'])
        n = _noise(nt, uv, 18, 4, 0.5)
        col = _ramp(nt, n, [(0.3, c1), (0.7, c2)])
        wr = _ramp(nt, w.outputs['Fac'], [(0.3, '#d8d8d8'), (0.7, '#ffffff')])
        col = _mulc(nt, col, wr, 0.5)
        _finish(nt, bsdf, col, _bump(nt, n, None, 0.3, 0.01), rough, True, 0.2)
    return f

def stripes(c1, c2, width=0.22):
    def f(nt, bsdf, uv, obj):
        w = nt.nodes.new('ShaderNodeTexWave'); w.wave_type = 'BANDS'; w.inputs['Scale'].default_value = 1.0 / (width * 2)
        w.inputs['Distortion'].default_value = 0.0
        w.bands_direction = 'X'
        nt.links.new(uv, w.inputs['Vector'])
        col = _ramp(nt, w.outputs['Fac'], [(0.0, c1), (0.5, c2)], const=True)
        n = _noise(nt, uv, 30, 3, 0.5)
        col = _mulc(nt, col, _ramp(nt, n, [(0.3, '#cfcfcf'), (0.7, '#ffffff')]), 0.5)
        _finish(nt, bsdf, col, _bump(nt, n, None, 0.25, 0.01), 0.9, True, 0.2)
    return f

def crop(c1, c2, tilled='#3b2a18'):
    def f(nt, bsdf, uv, obj):
        w = nt.nodes.new('ShaderNodeTexWave'); w.wave_type = 'BANDS'; w.inputs['Scale'].default_value = 5.5
        w.bands_direction = 'Y'; w.inputs['Distortion'].default_value = 0.4
        nt.links.new(uv, w.inputs['Vector'])
        n = _noise(nt, uv, 28, 5, 0.6)
        rows = _ramp(nt, w.outputs['Fac'], [(0.25, tilled), (0.5, c1), (0.8, c2)])
        col = _mulc(nt, rows, _ramp(nt, n, [(0.3, '#b0b0b0'), (0.7, '#ffffff')]), 0.8)
        nrm = _bump(nt, w.outputs['Fac'], None, 1.2, 0.05)
        _finish(nt, bsdf, col, nrm, 0.95, True, 0.3)
    return f

def build_materials2():
    M = {}
    M['wood'] = mat2('v2wood', planks('#7d5430', '#a67a42'))
    M['woodDark'] = mat2('v2woodDark', planks('#4a3220', '#6b482b', 1.2, 0.09))
    M['woodLight'] = mat2('v2woodLight', planks('#b88f55', '#dcb578', 1.6, 0.11))
    M['plaster'] = mat2('v2plaster', stucco('#d8c8a0', '#eee0bd', '#b8a57c'))
    M['stone'] = mat2('v2stone', masonry('#9b978c', '#bdb8a8', '#6f6a5f'))
    M['stoneDark'] = mat2('v2stoneDark', masonry('#6e6a60', '#8e897d', '#4d493f', 0.5, 0.25))
    M['thatch'] = mat2('v2thatch', thatch('#b0903f', '#dcb962', '#6b5628'))
    M['tile'] = mat2('v2tile', shingles('#a34a2c', '#c9683a', '#5e2a1b'))
    M['slate'] = mat2('v2slate', shingles('#54606b', '#7c8997', '#303840', 0.14, 0.09))
    M['dirt'] = mat2('v2dirt', earth('#6e5333', '#907248'))
    M['soil'] = mat2('v2soil', crop('#4a331c', '#5c4026', '#2a1c0e'))
    M['crop'] = mat2('v2crop', crop('#6f9a2e', '#a9c94a'))
    M['wheat'] = mat2('v2wheat', crop('#c49a32', '#ecce6c'))
    M['canvas'] = mat2('v2canvas', cloth('#d2c4a0', '#e8dcbc', 55))
    M['hide'] = mat2('v2hide', cloth('#6d4a2f', '#8f6844', 30))
    M['hay'] = mat2('v2hay', thatch('#c9a63e', '#ead076', '#8c6f25'))
    M['metal'] = mat2('v2metal', solid('#9096a0', 0.35, 0.9))
    M['iron'] = mat2('v2iron', solid('#4b4e54', 0.45, 0.8))
    M['blue'] = mat2('v2blue', cloth('#2447a6', '#3a63c9', 60, 0.8))
    M['red'] = mat2('v2red', cloth('#a62424', '#c93a3a', 60, 0.8))
    M['white'] = mat2('v2white', solid('#ece8dc', 0.9))
    M['black'] = mat2('v2black', solid('#17130f', 0.9, 0, None, False))
    M['gold'] = mat2('v2gold', solid('#f0b81c', 0.28, 1.0))
    M['rock'] = mat2('v2rock', masonry('#767269', '#a29e92', '#4d493f', 0.7, 0.45))
    M['leaf1'] = mat2('v2leaf1', foliage('#27561f', '#4a8a30', '#97c653'))
    M['leaf2'] = mat2('v2leaf2', foliage('#36701f', '#62a03a', '#b3d65c'))
    M['leafPine'] = mat2('v2leafPine', foliage('#17401f', '#2c6a35', '#5a9a52', 6))
    M['bark'] = mat2('v2bark', planks('#4a3322', '#6c4d33', 0.9, 0.16, mortar='#2a1a0e'))
    M['skin'] = mat2('v2skin', solid('#e2b48c', 0.7, 0, None, True))
    M['fire'] = mat2('v2fire', solid('#ff8a1a', 0.5, 0, None, False))
    M['char'] = mat2('v2char', earth('#1d1713', '#3a2c22'))
    M['cloth1'] = mat2('v2cloth1', cloth('#7a5a38', '#8e6b46', 45))
    M['cloth2'] = mat2('v2cloth2', cloth('#7c8250', '#97a066', 45))
    M['grassTop'] = mat2('v2grassTop', earth('#4f7d35', '#6c9c46'))
    M['marble'] = mat2('v2marble', masonry('#d2d0c8', '#e8e6e0', '#a9a69c', 0.5, 0.25, False))
    M['marblePlaster'] = mat2('v2marblePlaster', stucco('#e8e4d8', '#f6f2e8', '#cfc8b4'))
    M['imperial'] = mat2('v2imperial', shingles('#243566', '#3a5599', '#141f40'))
    # new for v2 detailing
    M['awningBlue'] = mat2('v2awnBlue', stripes('#2f5aa8', '#f2ede0'))
    M['awningRed'] = mat2('v2awnRed', stripes('#b03a2a', '#f2ede0'))
    M['cobble'] = mat2('v2cobble', masonry('#8a8478', '#b0a998', '#5a554a', 0.22, 0.16))
    M['flower1'] = mat2('v2flower1', solid('#f2e24a', 0.8, 0, None, False))
    M['flower2'] = mat2('v2flower2', solid('#e86a7a', 0.8, 0, None, False))
    M['flower3'] = mat2('v2flower3', solid('#f4f0e6', 0.8, 0, None, False))
    M['grassBlade'] = mat2('v2blade', foliage('#2c6422', '#58963a', '#a6d05c', 9))
    M['sand'] = mat2('v2sand', earth('#c4a874', '#e0c894'))
    M['pebble'] = mat2('v2pebble', masonry('#8a867c', '#b2ada0', '#5a554a', 0.6, 0.4, False))
    return M
