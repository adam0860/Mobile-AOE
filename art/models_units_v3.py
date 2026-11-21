# Unit rigs v3: jointed humanoids / horses / siege engines posed per animation frame (exec'd by gen_assets.py).
# Frames per unit: 0 = idle, 1..6 = move cycle, 7..9 = attack.  Villagers also get 4 work tools x 4 frames.
# Models are built ONCE per kind, then re-posed and re-rendered for every direction/frame.

def _jt(parent, x=0.0, y=0.0, z=0.0, name='j'):
    e = bpy.data.objects.new(name, None)
    scene.collection.objects.link(e)
    _created.append(e)
    e.parent = parent
    e.location = (x, y, z)
    return e

def _on(o, parent):
    o.parent = parent
    return o

def _limb(parent, length, rt, rb, mat, verts=8):
    o = cyl(0, 0, 0, rb, length, mat, verts=verts, r2=rt)
    o.data.transform(Matrix.Translation((0, 0, -length)))
    return _on(o, parent)

def _bx(parent, x, y, z, sx, sy, sz, mat, rz=0.0):
    return _on(box(x, y, z, sx, sy, sz, mat, rz=rz), parent)

def _cy(parent, x, y, z, r, h, mat, verts=10, r2=None, rx=0.0, ry=0.0, rz=0.0):
    return _on(cyl(x, y, z, r, h, mat, verts=verts, r2=r2, rx=rx, ry=ry, rz=rz), parent)

def _sp(parent, x, y, z, r, mat, sx=1, sy=1, sz=1, seg=10, noise=0.0, seed=0):
    return _on(sphere(x, y, z, r, mat, sx=sx, sy=sy, sz=sz, seg=seg, noise=noise, seed=seed), parent)

def _cn(parent, x, y, z, r, h, mat, verts=10):
    return _on(cone(x, y, z, r, h, mat, verts=verts), parent)

HIP = 0.46
THIGH, SHIN, UPARM, FOREARM = 0.23, 0.23, 0.19, 0.19

def make_humanoid(kind):
    R = {'kind': kind}
    root = bpy.data.objects.new('unit_root', None)
    scene.collection.objects.link(root)
    _created.append(root)
    R['root'] = root
    tm = M['blue']
    pants = {'villager': M['cloth1'], 'militia': M['hide'], 'manatarms': M['iron'], 'archer': M['hide'], 'longsword': M['iron'], 'crossbow': M['hide'],
             'monk': M['hide'], 'champion': M['metal'], 'arbalester': M['iron'], 'handcannon': M['hide']}[kind]
    boots = M['woodDark']
    # legs hang from hip joints on the root
    for side, nm in ((-1, 'R'), (1, 'L')):
        hip = _jt(root, 0, side * 0.075, HIP, 'hip' + nm)
        _limb(hip, THIGH, 0.065, 0.055, pants, 8)
        knee = _jt(hip, 0, 0, -THIGH, 'knee' + nm)
        _limb(knee, SHIN, 0.052, 0.045, boots if kind not in ('champion', 'manatarms', 'longsword', 'arbalester') else M['iron'], 8)
        _bx(knee, 0.04, 0, -SHIN - 0.02, 0.17, 0.09, 0.05, boots)                          # foot
        R['hip' + nm], R['knee' + nm] = hip, knee
    torso = _jt(root, 0, 0, HIP, 'torso')
    R['torso'] = torso
    chest = {'villager': M['cloth1'], 'militia': tm, 'manatarms': M['metal'], 'archer': M['hide'], 'longsword': M['metal'], 'crossbow': M['hide'],
             'monk': M['hide'], 'champion': M['metal'], 'arbalester': M['metal'], 'handcannon': M['hide']}[kind]
    _cy(torso, 0, 0, 0.0, 0.15, 0.4, chest, verts=12, r2=0.17)
    _cy(torso, 0, 0, 0.0, 0.155, 0.05, M['black'], verts=12)                               # belt
    _bx(torso, 0.0, 0, 0.0, 0.05, 0.05, 0.05, M['gold'])
    head = _jt(torso, 0, 0, 0.47, 'head')
    R['head'] = head
    _cy(head, 0, 0, -0.04, 0.05, 0.07, M['skin'], verts=8)                                  # neck
    _sp(head, 0, 0, 0.1, 0.115, M['skin'], seg=12)
    for s in (-1, 1):
        _sp(head, 0.1, s * 0.045, 0.11, 0.016, M['black'], seg=5)                           # eyes
    _bx(head, 0.115, 0, 0.075, 0.03, 0.03, 0.04, M['skin'])                                # nose
    # arms
    for side, nm in ((-1, 'R'), (1, 'L')):
        sh = _jt(torso, 0, side * 0.185, 0.37, 'sh' + nm)
        sleeve = {'villager': M['cloth1'], 'militia': tm, 'archer': M['hide'], 'monk': M['hide'], 'handcannon': M['hide'], 'crossbow': M['hide']}.get(kind, M['metal'])
        _limb(sh, UPARM, 0.05, 0.045, sleeve, 8)
        _sp(sh, 0, 0, 0.0, 0.065, sleeve if kind in ('militia', 'villager') else M['metal'], seg=8)      # shoulder
        el = _jt(sh, 0, 0, -UPARM, 'el' + nm)
        _limb(el, FOREARM, 0.043, 0.036, M['skin'] if kind in ('villager', 'militia', 'archer', 'monk', 'handcannon', 'crossbow') else M['metal'], 8)
        hand = _jt(el, 0, 0, -FOREARM, 'hand' + nm)
        _sp(hand, 0, 0, -0.01, 0.042, M['skin'] if kind in ('villager', 'militia', 'archer', 'monk', 'handcannon', 'crossbow') else M['metal'], seg=8)
        R['sh' + nm], R['el' + nm], R['hand' + nm] = sh, el, hand
    R['flags'] = {}
    _outfit(R, kind)
    return R

def _sword(parent, length=0.55, mat=None, w=0.045):
    mat = mat or M['metal']
    g = _jt(parent, 0, 0, -0.01, 'sword')
    _bx(g, 0, 0, 0.0, 0.05, 0.05, 0.12, M['woodDark'])                                  # grip
    _bx(g, 0, 0, 0.12, 0.05, 0.2, 0.035, M['iron'])                                     # guard
    _bx(g, 0, 0, 0.155, 0.014, w, length, mat)                                          # blade
    _cn(g, 0, 0, 0.155 + length, 0.022, 0.07, mat, 4)
    g.rotation_euler = (0, -0.5, 0)                                                      # held angled forward
    return g

def _round_shield(parent, r=0.19, mat=None):
    g = _jt(parent, 0.0, 0.07, -0.07, 'shield')
    _cy(g, 0, 0, 0, r, 0.04, mat or M['woodLight'], verts=16, ry=math.pi / 2)
    _cy(g, 0.03, 0, 0, r * 0.28, 0.03, M['metal'], verts=10, ry=math.pi / 2)
    _cy(g, 0.02, 0, 0, r * 0.82, 0.02, M['blue'], verts=16, ry=math.pi / 2)
    return g

def _kite_shield(parent):
    g = _jt(parent, 0.0, 0.08, -0.05, 'kite')
    _bx(g, 0.0, 0, -0.17, 0.045, 0.25, 0.38, M['blue'])
    _bx(g, 0.03, 0, -0.1, 0.02, 0.05, 0.28, M['white'])
    _bx(g, 0.03, 0, -0.02, 0.02, 0.19, 0.05, M['white'])
    _cn(g, 0.0, 0, -0.27, 0.0, 0.0, M['blue'], 4) if False else None
    return g

def _bow(parent):
    g = _jt(parent, 0.03, 0, -0.02, 'bow')
    for i in range(9):
        t = i / 8.0
        z = -0.32 + t * 0.64
        x = 0.07 * math.sin(t * math.pi) * -1.0
        b = _bx(g, x - 0.0, 0, z, 0.022, 0.026, 0.085, M['woodLight'])
        b.rotation_euler = (0, 0.5 * (t - 0.5) * 2 * 0.8, 0)
    _bx(g, -0.07, 0, -0.32, 0.006, 0.006, 0.64, M['white'])                             # string
    return g

def _crossbow(parent, big=False):
    g = _jt(parent, 0.05, 0, -0.02, 'xbow')
    L = 0.5 if big else 0.4
    _bx(g, L / 2, 0, 0.0, L, 0.05, 0.06, M['woodDark'])                                 # stock
    _bx(g, L * 0.8, 0, 0.0, 0.035, 0.42 if big else 0.34, 0.03, M['metal'] if big else M['woodLight'])   # prod
    _bx(g, L * 0.55, 0, 0.04, L * 0.4, 0.012, 0.012, M['white'])
    _cn(g, L * 0.95, 0, 0.045, 0.014, 0.08, M['metal'], 5).rotation_euler = (0, math.pi / 2, 0)
    return g

def _handgonne(parent):
    g = _jt(parent, 0.04, 0, -0.02, 'gonne')
    _bx(g, 0.08, 0, -0.01, 0.24, 0.055, 0.09, M['woodDark'])
    c = _cy(g, 0.2, 0, 0.0, 0.034, 0.5, M['iron'], verts=10, r2=0.04, ry=math.pi / 2)
    _cy(g, 0.62, 0, 0.0, 0.048, 0.04, M['gold'], verts=10, ry=math.pi / 2)
    flash = _sp(g, 0.74, 0, 0.0, 0.1, M['fire'], seg=8, sz=0.8)
    flash.hide_render = True
    return g, flash

def _staff(parent, h=1.05):
    g = _jt(parent, 0, 0, -0.01, 'staff')
    _cy(g, 0, 0, -0.35, 0.018, h, M['wood'], verts=6)
    _bx(g, 0, 0, h - 0.3, 0.02, 0.15, 0.02, M['gold'])
    _bx(g, 0, 0, h - 0.24, 0.02, 0.02, 0.14, M['gold'])
    return g

def _outfit(R, kind):
    tm = M['blue']
    torso, head = R['torso'], R['head']
    if kind == 'villager':
        _cy(torso, 0, 0, 0.12, 0.17, 0.2, M['cloth1'], verts=12, r2=0.2)               # tunic skirt
        _bx(torso, 0.15, 0, 0.07, 0.03, 0.2, 0.28, M['canvas'])                         # apron
        _cy(head, 0, 0, 0.17, 0.2, 0.02, M['hay'], verts=14)                            # straw hat
        _cn(head, 0, 0, 0.18, 0.12, 0.11, M['hay'], 12)
        _bx(torso, -0.17, 0, 0.1, 0.1, 0.22, 0.22, M['hide'])                           # sack on the back
        _cy(R['shL'], 0, 0, -0.02, 0.058, 0.04, M['blue'], verts=8)                      # blue armband = player's colour
    elif kind == 'militia':
        _cy(torso, 0, 0, 0.12, 0.17, 0.2, M['hide'], verts=12, r2=0.2)
        _sp(head, 0, 0, 0.13, 0.125, M['iron'], seg=12, sz=0.62)                         # leather/iron cap
        _bx(head, 0.12, 0, 0.0, 0.02, 0.025, 0.2, M['iron'])                             # nasal
        R['sword'] = _sword(R['handR'], 0.5)
        R['shield'] = _round_shield(R['elL'])
    elif kind == 'manatarms':
        _cy(torso, 0, 0, 0.1, 0.18, 0.28, M['blue'], verts=12, r2=0.21)                  # surcoat
        _bx(torso, 0.18, 0, 0.14, 0.02, 0.06, 0.18, M['white'])
        _sp(head, 0, 0, 0.09, 0.13, M['iron'], seg=10, sz=0.95)                          # mail coif
        _sp(head, 0, 0, 0.16, 0.125, M['metal'], seg=12, sz=0.62)                        # kettle crown
        _cy(head, 0, 0, 0.15, 0.2, 0.02, M['metal'], verts=14)                           # brim
        R['sword'] = _sword(R['handR'], 0.55)
        R['shield'] = _kite_shield(R['elL'])
    elif kind == 'archer':
        _cy(torso, 0, 0, 0.05, 0.17, 0.3, M['cloth2'], verts=12, r2=0.21)
        _cy(torso, 0, 0, 0.18, 0.175, 0.05, M['blue'], verts=12)                         # team sash
        _cn(head, 0, 0, 0.1, 0.15, 0.2, M['cloth2'], 12)                                 # hood
        q = _jt(torso, -0.17, 0.05, 0.2, 'quiver')
        _bx(q, 0, 0, 0, 0.08, 0.08, 0.3, M['hide'])
        for i in range(4):
            _cy(q, 0, -0.02 + i * 0.014, 0.28, 0.006, 0.12, M['white'], verts=4)
        R['bow'] = _bow(R['handL'])
    elif kind == 'longsword':
        _cy(torso, 0, 0, 0.0, 0.18, 0.4, M['blue'], verts=12, r2=0.21)                   # surcoat over mail
        _bx(torso, 0.19, 0, 0.14, 0.02, 0.05, 0.2, M['white'])
        _bx(torso, 0.19, 0, 0.24, 0.02, 0.18, 0.05, M['white'])
        _sp(head, 0, 0, 0.1, 0.13, M['metal'], seg=12, sz=1.0)                            # bascinet
        _bx(head, 0.12, 0, 0.04, 0.02, 0.17, 0.08, M['iron'])                              # visor
        for s in (-1, 1):
            _sp(R['sh' + ('L' if s > 0 else 'R')], 0, 0, 0.02, 0.085, M['metal'], seg=8)   # pauldrons
        R['sword'] = _sword(R['handR'], 0.75, M['metal'], 0.05)
    elif kind == 'crossbow':
        _cy(torso, 0, 0, 0.05, 0.17, 0.3, M['hide'], verts=12, r2=0.2)
        _cy(torso, 0, 0, 0.17, 0.175, 0.05, M['blue'], verts=12)
        _sp(head, 0, 0, 0.14, 0.125, M['metal'], seg=12, sz=0.6)                          # sallet
        _cy(head, 0, 0, 0.14, 0.2, 0.02, M['metal'], verts=14)
        q = _jt(torso, -0.17, -0.05, 0.2, 'quiver')
        _bx(q, 0, 0, 0, 0.09, 0.07, 0.22, M['hide'])
        R['xbow'] = _crossbow(R['handR'], False)
    elif kind == 'monk':
        _cy(torso, 0, 0, -0.42, 0.26, 0.82, M['hide'], verts=14, r2=0.16)                # long robe
        _cy(torso, 0, 0, 0.0, 0.17, 0.05, M['blue'], verts=12)                            # sash
        _cn(head, 0, 0, 0.0, 0.17, 0.26, M['hide'], 12)                                   # hood
        _sp(head, 0, 0, 0.08, 0.12, M['skin'], seg=10)
        R['staff'] = _staff(R['handR'])
        R['orbL'] = _sp(R['handL'], 0.0, 0, 0.12, 0.09, M['fire'], seg=8)
        R['orbR'] = _sp(R['handR'], 0.0, 0, 0.0, 0.0001, M['fire'], seg=4)
        R['orbL'].hide_render = True
        R['orbR'].hide_render = True
    elif kind == 'champion':
        _cy(torso, 0, 0, 0.0, 0.19, 0.4, M['metal'], verts=12, r2=0.22)
        _cy(torso, 0, 0, 0.1, 0.2, 0.3, M['blue'], verts=12, r2=0.18)
        for s in (-1, 1):
            _sp(R['sh' + ('L' if s > 0 else 'R')], 0, 0, 0.03, 0.1, M['metal'], seg=8, sz=0.8)
        _sp(head, 0, 0, 0.11, 0.14, M['metal'], seg=12)
        _bx(head, 0.13, 0, 0.07, 0.025, 0.14, 0.04, M['black'])
        _cn(head, 0, 0, 0.2, 0.05, 0.3, M['blue'], 6)                                     # plume
        cape = _bx(torso, -0.18, 0, -0.1, 0.03, 0.34, 0.5, M['blue'])
        R['sword'] = _sword(R['handR'], 0.85, M['metal'], 0.06)
    elif kind == 'arbalester':
        _cy(torso, 0, 0, 0.0, 0.18, 0.4, M['metal'], verts=12, r2=0.21)
        _cy(torso, 0, 0, 0.08, 0.19, 0.3, M['blue'], verts=12, r2=0.17)
        _sp(head, 0, 0, 0.14, 0.125, M['metal'], seg=12, sz=0.6)
        _cy(head, 0, 0, 0.14, 0.21, 0.02, M['metal'], verts=14)
        R['xbow'] = _crossbow(R['handR'], True)
    elif kind == 'handcannon':
        _cy(torso, 0, 0, 0.05, 0.18, 0.32, M['hide'], verts=12, r2=0.21)
        _cy(torso, 0, 0, 0.0, 0.17, 0.05, M['blue'], verts=12)
        _cy(head, 0, 0, 0.14, 0.21, 0.02, M['blue'], verts=14)
        _sp(head, 0, 0, 0.16, 0.12, M['blue'], seg=10, sz=0.6)
        _bx(head, -0.1, -0.1, 0.17, 0.03, 0.04, 0.2, M['white']).rotation_euler = (0.3, 0, 0)     # feather
        g, fl = _handgonne(R['handR'])
        R['gonne'], R['flash'] = g, fl
        _bx(torso, 0.0, 0.17, 0.0, 0.06, 0.07, 0.1, M['white'])                           # powder horn

# ------------------------------------------------------------------ poses
def _neutral():
    return {'hipL': 0.03, 'hipR': -0.03, 'kneeL': 0.0, 'kneeR': 0.0, 'shL': 0.08, 'shR': -0.08, 'elL': -0.18, 'elR': -0.18, 'lean': 0.0, 'twist': 0.0, 'bob': 0.0, 'headY': 0.0, 'headZ': 0.0}

def walk_pose(f, run=1.0):
    p = _neutral()
    ph = (f - 1) / 6.0 * 2 * math.pi
    A = 0.55 * run
    s, c = math.sin(ph), math.cos(ph)
    p['hipL'], p['hipR'] = -A * s, A * s
    p['kneeL'], p['kneeR'] = 0.95 * max(0, c) * run, 0.95 * max(0, -c) * run
    p['shL'], p['shR'] = 0.5 * s * run, -0.5 * s * run
    p['elL'], p['elR'] = -0.3 - 0.25 * max(0, -s), -0.3 - 0.25 * max(0, s)
    p['bob'] = 0.032 * (c * c) - 0.016
    p['lean'] = -0.07 * run
    p['twist'] = 0.07 * s
    return p

def attack_pose(kind, a, R=None):
    """a = 0 (wind-up), 1 (strike), 2 (recover)"""
    p = _neutral()
    p['hipL'], p['hipR'] = (-0.25, 0.3) if a != 1 else (-0.4, 0.35)
    p['kneeL'], p['kneeR'] = (0.25, 0.1)
    if kind in ('militia', 'manatarms', 'longsword', 'champion'):
        heavy = kind in ('longsword', 'champion')
        p['shR'] = (2.4, -1.2, -0.4)[a]
        p['elR'] = (-0.9, -0.2, -0.6)[a]
        p['twist'] = (0.35, -0.4, -0.1)[a]
        p['lean'] = (0.12, -0.22, -0.05)[a]
        p['shL'] = (-0.5, -0.3, -0.2)[a]
        if heavy:
            p['shL'] = (1.8, -1.0, -0.4)[a]; p['elL'] = (-0.8, -0.3, -0.6)[a]               # two-handed grip follows the sword
    elif kind == 'archer':
        p['shL'], p['elL'] = -1.5, -0.05
        p['shR'] = (-1.3, -1.4, -0.9)[a]
        p['elR'] = (-1.2, -2.0, -0.4)[a]
        p['twist'] = (0.0, 0.1, -0.05)[a]
        p['lean'] = (0.0, 0.05, 0.0)[a]
        p['hipL'], p['hipR'] = -0.1, 0.2
    elif kind in ('crossbow', 'arbalester', 'handcannon'):
        p['shL'], p['elL'] = -1.25, -0.25
        p['shR'], p['elR'] = (-1.15, -1.1, -0.9)[a], (-0.3, -0.3, -1.1)[a]
        p['lean'] = (0.0, 0.14, 0.04)[a] if kind == 'handcannon' else (0.0, 0.06, 0.0)[a]
        p['twist'] = (0.0, -0.05, 0.0)[a]
        p['hipL'], p['hipR'] = -0.1, 0.2
    elif kind == 'monk':
        p['shL'] = (-1.7, -2.4, -1.2)[a]; p['shR'] = (-0.9, -1.1, -0.5)[a]
        p['elL'], p['elR'] = (-0.3, -0.2, -0.4)[a], -0.3
        p['lean'] = (0.0, -0.12, 0.0)[a]
        p['headY'] = (0.0, 0.2, 0.0)[a]
        p['hipL'], p['hipR'] = 0.05, -0.05
        p['kneeL'], p['kneeR'] = 0.0, 0.0
    elif kind == 'villager':
        p['shR'] = (2.0, -0.9, -0.3)[a]; p['elR'] = (-0.8, -0.3, -0.5)[a]; p['lean'] = (0.1, -0.2, 0)[a]; p['twist'] = (0.25, -0.3, 0)[a]
        p['hipL'], p['hipR'], p['kneeL'], p['kneeR'] = -0.15, 0.2, 0.15, 0.05
    return p

TOOLS = ('hammer', 'axe', 'pick', 'hoe')
def work_pose(tool, f):
    p = _neutral()
    t = f / 4.0 * 2 * math.pi
    s = (1 + math.sin(t - 1.2)) / 2.0                   # 0..1 raise -> strike
    if tool == 'hoe':
        p['lean'] = -0.35 + 0.1 * s
        p['shR'] = -0.9 - 0.5 * s; p['elR'] = -0.3
        p['shL'] = -0.8 - 0.4 * s; p['elL'] = -0.3
        p['hipL'], p['hipR'] = -0.2, 0.15
        p['kneeL'], p['kneeR'] = 0.3, 0.1
    else:
        up = {'hammer': 1.9, 'axe': 2.3, 'pick': 2.4}[tool]
        p['shR'] = up * (1 - s) + (-0.9) * s
        p['elR'] = -0.9 * (1 - s) - 0.25 * s
        p['shL'] = 0.4 * (1 - s) - 0.4 * s
        p['lean'] = 0.1 * (1 - s) - 0.22 * s
        p['twist'] = 0.28 * (1 - s) - 0.3 * s
        p['hipL'], p['hipR'] = -0.2, 0.25
        p['kneeL'], p['kneeR'] = 0.25, 0.1
    return p

def apply_pose(R, p):
    for nm in ('hipL', 'hipR', 'kneeL', 'kneeR'):
        R[nm].rotation_euler = (0, p[nm], 0)
    for nm in ('shL', 'shR', 'elL', 'elR'):
        R[nm].rotation_euler = (0, p[nm], 0)
    R['torso'].rotation_euler = (0, p['lean'], p['twist'])
    R['head'].rotation_euler = (0, p['headY'], -p['twist'] * 0.6)
    R['torso'].location = (0, 0, HIP + p['bob'])
    for nm in ('hipL', 'hipR'):
        R[nm].location = (0, R[nm].location.y, HIP + p['bob'])

def _tool(R, tool):
    for k in ('sword',):
        pass
    hand = R['handR']
    g = _jt(hand, 0, 0, -0.01, 'tool')
    if tool == 'hammer':
        _cy(g, 0, 0, 0.0, 0.017, 0.36, M['wood'], verts=6)
        _bx(g, 0, 0, 0.34, 0.07, 0.16, 0.07, M['iron'])
    elif tool == 'axe':
        _cy(g, 0, 0, 0.0, 0.017, 0.5, M['wood'], verts=6)
        _bx(g, 0, 0, 0.42, 0.04, 0.17, 0.12, M['metal'])
        _bx(g, 0, 0, 0.46, 0.05, 0.05, 0.06, M['iron'])
    elif tool == 'pick':
        _cy(g, 0, 0, 0.0, 0.017, 0.5, M['wood'], verts=6)
        _bx(g, 0, 0, 0.48, 0.03, 0.34, 0.03, M['iron'])
        _cn(g, 0, 0.19, 0.49, 0.025, 0.1, M['iron'], 5).rotation_euler = (-math.pi / 2, 0, 0)
        _cn(g, 0, -0.19, 0.49, 0.025, 0.1, M['iron'], 5).rotation_euler = (math.pi / 2, 0, 0)
    else:
        _cy(g, 0, 0, 0.0, 0.017, 0.7, M['wood'], verts=6)
        _bx(g, 0.05, 0, 0.68, 0.14, 0.1, 0.03, M['iron'])
    g.rotation_euler = (0, -0.6, 0)
    return g

# ------------------------------------------------------------------ horse + rider
def make_horse(kind):
    R = {'kind': kind}
    root = bpy.data.objects.new('unit_root', None)
    scene.collection.objects.link(root); _created.append(root)
    R['root'] = root
    pal = kind == 'paladin'
    coat = M['woodLight'] if pal else M['hide']
    s = 1.05 if pal else 1.0
    body = _jt(root, 0, 0, 0.7 * s, 'body')
    R['body'] = body
    _sp(body, 0, 0, 0, 0.34 * s, coat, sx=1.6, sy=0.62, sz=0.78, seg=12)
    _sp(body, 0.4 * s, 0, 0.1 * s, 0.19 * s, coat, sx=1.15, sy=0.75, sz=1.0, seg=10)         # chest
    _sp(body, -0.4 * s, 0, 0.0, 0.2 * s, coat, sx=1.0, sy=0.9, sz=0.95, seg=10)              # haunch
    neck = _jt(body, 0.46 * s, 0, 0.12 * s, 'neck')
    _on(cyl(0, 0, 0, 0.1 * s, 0.38 * s, coat, verts=9, r2=0.075 * s, ry=math.radians(-38)), neck)
    head = _jt(neck, 0.24 * s, 0, 0.33 * s, 'hhead')
    _sp(head, 0.12 * s, 0, 0.0, 0.1 * s, coat, sx=1.6, sy=0.7, sz=0.8, seg=9)
    for sd in (-1, 1):
        _on(cone(0, sd * 0.045 * s, 0.08 * s, 0.025 * s, 0.08 * s, coat, verts=5), head)
    _bx(neck, 0.0, 0, 0.0, 0.07, 0.03, 0.3 * s, M['black']).rotation_euler = (0, math.radians(-38), 0)    # mane
    tail = _jt(body, -0.52 * s, 0, 0.05 * s, 'tail')
    _on(cyl(0, 0, 0, 0.035 * s, 0.4 * s, M['black'], verts=6, r2=0.012 * s, ry=math.radians(160)), tail)
    R['neck'], R['tail'], R['hhead'] = neck, tail, head
    legs = {}
    for nm, lx, ly in (('FL', 0.32, 0.12), ('FR', 0.32, -0.12), ('BL', -0.32, 0.12), ('BR', -0.32, -0.12)):
        hj = _jt(body, lx * s, ly * s, -0.12 * s, 'h' + nm)
        _on(cyl(0, 0, 0, 0.05 * s, 0.28 * s, coat, verts=7, r2=0.065 * s), hj).data.transform(Matrix.Translation((0, 0, -0.28 * s)))
        kj = _jt(hj, 0, 0, -0.28 * s, 'k' + nm)
        c2 = _on(cyl(0, 0, 0, 0.034 * s, 0.26 * s, coat, verts=7, r2=0.045 * s), kj); c2.data.transform(Matrix.Translation((0, 0, -0.26 * s)))
        _on(box(0.01, 0, -0.26 * s - 0.04, 0.08, 0.06, 0.06, M['black']), kj)
        legs[nm] = (hj, kj)
    R['legs'] = legs
    if pal:                                                  # barding plates
        _bx(body, 0.42 * s, 0, 0.05 * s, 0.22, 0.32, 0.3, M['metal'])
        _bx(neck, 0.28 * s, 0, 0.35 * s, 0.2, 0.14, 0.12, M['metal'])
    tm = M['blue']
    _bx(body, 0, 0, 0.17 * s, 0.62 * s, 0.62 * s, 0.05, tm)                                    # caparison
    for sd in (-1, 1):
        _bx(body, 0, sd * 0.31 * s, -0.12 * s, 0.62 * s, 0.03, 0.34 * s, tm)
        _bx(body, 0, sd * 0.325 * s, -0.12 * s, 0.64 * s, 0.02, 0.045, M['white'])
    # rider on the saddle
    rider = _jt(body, -0.02, 0, 0.22 * s, 'rider')
    R['rider'] = rider
    _cy(rider, 0, 0, 0.0, 0.15, 0.36, M['metal'] if pal else M['iron'], verts=10, r2=0.12)
    _cy(rider, 0, 0, 0.08, 0.16, 0.22, tm, verts=10, r2=0.13)
    rh = _jt(rider, 0, 0, 0.42, 'rhead')
    _sp(rh, 0, 0, 0.1, 0.115, M['metal'], seg=10, sz=1.05)
    _bx(rh, 0.11, 0, 0.05, 0.025, 0.16, 0.06, M['black'])
    _cn(rh, 0, 0, 0.19, 0.045, 0.28 if pal else 0.14, tm, 6)
    for sd in (-1, 1):
        _bx(rider, 0.02, sd * 0.1, -0.18, 0.1, 0.05, 0.18, M['iron'])                          # thigh
    arm = _jt(rider, 0, -0.18, 0.36, 'rarm')
    _limb(arm, 0.3, 0.045, 0.04, M['metal'], 7)
    lance = _jt(arm, 0.0, 0, -0.3, 'lance')
    _on(cyl(0, 0, -0.3, 0.018, 1.7, M['wood'], verts=6, ry=math.radians(90)), lance)
    _on(cone(1.4, 0, 0.0, 0.035, 0.18, M['metal'], verts=6), lance).rotation_euler = (0, math.radians(90), 0)
    _on(box(1.25, 0, -0.02, 0.2, 0.01, 0.12, tm), lance)
    R['arm'], R['lance'] = arm, lance
    lsh = _jt(rider, 0, 0.19, 0.3, 'lsh')
    _bx(lsh, 0.02, 0.05, -0.2, 0.03, 0.2, 0.36, tm)
    _bx(lsh, 0.04, 0.05, -0.15, 0.02, 0.04, 0.26, M['white'])
    return R

def horse_pose(R, f, mode):
    s = R['legs']
    ph = (f - 1) / 6.0 * 2 * math.pi if mode == 'run' else 0.0
    offs = {'FL': 0.0, 'FR': 0.5, 'BL': 0.25, 'BR': 0.75}
    for nm, (hj, kj) in s.items():
        if mode == 'run':
            t = ph + offs[nm] * 2 * math.pi
            hj.rotation_euler = (0, -0.7 * math.sin(t), 0)
            kj.rotation_euler = (0, 1.1 * max(0, math.cos(t)), 0)
        else:
            hj.rotation_euler = (0, 0.03 if nm[0] == 'F' else -0.03, 0)
            kj.rotation_euler = (0, 0.0, 0)
    R['body'].location = (0, 0, (0.7 * (1.05 if R['kind'] == 'paladin' else 1.0)) + (0.03 * math.sin(2 * ph) if mode == 'run' else 0.0))
    R['body'].rotation_euler = (0, 0.04 * math.sin(ph) if mode == 'run' else 0, 0)
    R['neck'].rotation_euler = (0, 0.1 * math.sin(2 * ph + 1) if mode == 'run' else 0, 0)
    R['tail'].rotation_euler = (0, 0.25 * math.sin(ph + 1) if mode == 'run' else 0, 0.1 * math.sin(ph * 2))
    R['arm'].rotation_euler = (0, -1.2, 0)
    R['lance'].rotation_euler = (0, 0.05, 0)
    R['lance'].location = (0, 0, -0.3)

def horse_attack(R, a):
    horse_pose(R, 1, 'idle')
    R['body'].rotation_euler = (0, (-0.05, 0.1, 0.0)[a], 0)
    R['legs']['FL'][0].rotation_euler = (0, (-0.5, -0.9, -0.2)[a], 0); R['legs']['FR'][0].rotation_euler = (0, (-0.4, -0.8, -0.1)[a], 0)
    R['legs']['FL'][1].rotation_euler = (0, (0.6, 0.9, 0.3)[a], 0); R['legs']['FR'][1].rotation_euler = (0, (0.5, 0.8, 0.2)[a], 0)
    R['arm'].rotation_euler = (0, (-0.6, -1.5, -1.0)[a], 0)
    R['lance'].location = (0, 0, -0.3)
    R['lance'].rotation_euler = (0, (0.5, -0.1, 0.2)[a], 0)

# ------------------------------------------------------------------ siege engines
def make_siege(kind):
    R = {'kind': kind, 'wheels': []}
    root = bpy.data.objects.new('unit_root', None)
    scene.collection.objects.link(root); _created.append(root)
    R['root'] = root
    tm = M['blue']
    def wheel(parent, x, y, z, r, th=0.05):
        w = _jt(parent, x, y, z, 'wheel')
        _cy(w, 0, 0, -th / 2, r, th, M['wood'], verts=14, rx=math.pi / 2)
        _cy(w, 0, 0, -th / 2 - 0.01, r * 0.2, th + 0.02, M['iron'], verts=8, rx=math.pi / 2)
        _cy(w, 0, 0, -th / 2 - 0.02, r, 0.01, M['iron'], verts=14, rx=math.pi / 2)
        for i in range(6):
            a = i * math.pi / 3
            sp = _bx(w, math.cos(a) * r * 0.5, 0, math.sin(a) * r * 0.5, r * 0.95, th * 0.8, 0.05, M['woodDark'])
            sp.rotation_euler = (0, -a, 0)
            sp.location = (math.cos(a) * r * 0.5, 0, math.sin(a) * r * 0.5)
        R['wheels'].append(w)
        return w
    if kind == 'ram':
        for sy in (-0.3, 0.3):
            _bx(root, 0, sy, 0.16, 1.1, 0.06, 0.06, M['woodDark'])
        for sx in (-0.35, 0.35):
            _bx(root, sx, 0, 0.16, 0.06, 0.66, 0.06, M['woodDark'])
        for sx in (-0.38, 0.38):
            for sy in (-0.36, 0.36):
                wheel(root, sx, sy, 0.15, 0.15)
        for sx in (-0.35, 0.35):
            for sy in (-0.28, 0.28):
                _cy(root, sx, sy, 0.18, 0.035, 0.42, M['wood'], verts=6)
        _on(prism_roof(0, 0, 0.58, 0.95, 0.56, 0.26, M['wood'], over=0.05, along='x', ridge_ext=0.05), root)
        _bx(root, 0, 0, 0.6, 0.9, 0.5, 0.012, tm)
        sw = _jt(root, 0, 0, 0.62, 'swing')
        _cy(sw, -0.45, 0, -0.26, 0.1, 1.15, M['bark'], verts=10, ry=math.pi / 2)
        _cn(sw, 0.7, 0, -0.26, 0.11, 0.2, M['iron'], 8).rotation_euler = (0, math.pi / 2, 0)
        for sx in (-0.2, 0.3):
            _bx(sw, sx, 0, -0.14, 0.02, 0.02, 0.14, M['black'])
        R['swing'] = sw
        flg = _jt(root, -0.45, 0, 0.9, 'flag'); _on(flag(0, 0, 0, 0.3, 'blue'), flg)
    elif kind == 'mangonel':
        _bx(root, 0, 0, 0.14, 0.9, 0.55, 0.07, M['woodDark'])
        for sx in (-0.3, 0.3):
            for sy in (-0.3, 0.3):
                wheel(root, sx, sy, 0.14, 0.13)
        for sy in (-0.2, 0.2):
            _bx(root, -0.05, sy, 0.2, 0.07, 0.07, 0.4, M['wood'])
        _bx(root, 0.0, 0, 0.55, 0.1, 0.5, 0.06, M['wood'])
        arm = _jt(root, -0.05, 0, 0.5, 'arm')
        _cy(arm, 0, 0, 0, 0.035, 0.95, M['wood'], verts=6)
        bucket = _jt(arm, 0, 0, 0.95, 'bucket')
        _bx(bucket, 0, 0, 0, 0.12, 0.1, 0.05, M['woodDark'])
        R['ball'] = _sp(bucket, 0, 0, 0.09, 0.09, M['rock'], seg=6, noise=0.25)
        R['arm'] = arm
        _bx(root, -0.4, 0, 0.2, 0.14, 0.14, 0.12, M['stoneDark'])
        flag(-0.35, 0.26, 0.2, 0.4, 'blue')
    elif kind == 'trebuchet':
        for sy in (-0.45, 0.45):
            _bx(root, 0, sy, 0.0, 1.5, 0.1, 0.1, M['woodDark'])
            for lean in (12, -12):
                c = _on(cyl(-0.15, sy, 0.05, 0.05, 0.95, M['wood'], verts=6), root)
                c.rotation_euler = (0, math.radians(lean), 0)
        _bx(root, -0.15, 0, 0.9, 0.08, 1.0, 0.08, M['wood'])
        arm = _jt(root, -0.15, 0, 0.95, 'arm')
        _cy(arm, 0, 0, -0.35, 0.05, 1.7, M['wood'], verts=7)
        cw = _bx(arm, 0, 0, -0.55, 0.3, 0.3, 0.3, M['stoneDark'])
        sling = _jt(arm, 0, 0, 1.3, 'sling')
        _cy(sling, 0, 0, -0.5, 0.008, 0.5, M['canvas'], verts=4)
        R['ball'] = _sp(sling, 0, 0, -0.52, 0.09, M['rock'], seg=6, noise=0.2)
        R['arm'], R['sling'] = arm, sling
        for sy in (-0.45, 0.45):
            _cy(root, -0.6, sy, 0.08, 0.08, 0.05, M['iron'], verts=8, rx=math.pi / 2)
            _cy(root, 0.6, sy, 0.08, 0.08, 0.05, M['iron'], verts=8, rx=math.pi / 2)
        flag(-0.7, -0.45, 0.1, 0.5, 'blue')
    elif kind == 'bombard':
        _bx(root, 0, 0, 0.16, 1.1, 0.5, 0.1, M['woodDark'])
        for sx in (-0.3, 0.3):
            for sy in (-0.3, 0.3):
                wheel(root, sx, sy, 0.15, 0.15, 0.06)
        _bx(root, 0, 0, 0.26, 0.7, 0.4, 0.14, M['wood'])
        bar = _jt(root, 0, 0, 0.62, 'barrel')
        _cy(bar, -0.45, 0, 0, 0.16, 1.15, M['iron'], verts=14, r2=0.12, ry=math.radians(84))
        for t in (0.1, 0.35, 0.6):
            _cy(bar, -0.45 + t, 0, 0, 0.17 - t * 0.06, 0.05, M['gold'], verts=14, ry=math.radians(84))
        R['barrel'] = bar
        R['flash'] = _sp(bar, 0.85, 0, 0.1, 0.22, M['fire'], seg=8)
        R['flash'].hide_render = True
        for i in range(3):
            _sp(root, -0.35 + i * 0.13, -0.45, 0.07, 0.07, M['black'], seg=6)
        flag(-0.55, -0.22, 0.4, 0.45, 'blue')
    return R

def siege_pose(R, f, mode, a=0):
    k = R['kind']
    if mode == 'move':
        ang = (f - 1) * math.radians(10)
        for w in R['wheels']:
            w.rotation_euler = (0, -ang, 0)
    else:
        for w in R['wheels']:
            w.rotation_euler = (0, 0, 0)
    if k == 'ram':
        sw = R['swing']
        if mode == 'attack':
            sw.location = (0, 0, 0.62)
            sw.rotation_euler = (0, (0.28, -0.12, 0.05)[a], 0)
            sw.location = ((-0.35, 0.28, 0.05)[a] * 0.0, 0, 0.62)
        else:
            sw.rotation_euler = (0, 0, 0)
    elif k == 'mangonel':
        arm = R['arm']
        if mode == 'attack':
            arm.rotation_euler = (0, math.radians((-150, -25, -80)[a]), 0)
            R['ball'].hide_render = a != 0
        else:
            arm.rotation_euler = (0, math.radians(-110), 0)
            R['ball'].hide_render = False
    elif k == 'trebuchet':
        arm = R['arm']
        if mode == 'attack':
            arm.rotation_euler = (0, math.radians((-125, 40, 5)[a]), 0)
            R['sling'].rotation_euler = (0, math.radians((60, -70, 0)[a]), 0)
            R['ball'].hide_render = a == 2
        else:
            arm.rotation_euler = (0, math.radians(-125), 0)
            R['sling'].rotation_euler = (0, math.radians(60), 0)
            R['ball'].hide_render = False
    elif k == 'bombard':
        b = R['barrel']
        if mode == 'attack':
            b.location = (((0.0, -0.18, -0.07)[a]), 0, 0.62)
            R['flash'].hide_render = a != 1
        else:
            b.location = (0, 0, 0.62)
            R['flash'].hide_render = True

# ------------------------------------------------------------------ render driver
HUMANOIDS = ('villager', 'militia', 'manatarms', 'archer', 'longsword', 'crossbow', 'monk', 'champion', 'arbalester', 'handcannon')
CAVALRY = ('knight', 'paladin')
SIEGE = ('ram', 'mangonel', 'trebuchet', 'bombard')
UNIT_KINDS = HUMANOIDS + CAVALRY + SIEGE
UNIT_SCALE = {'knight': 0.9, 'paladin': 0.92, 'ram': 0.95, 'mangonel': 0.95, 'trebuchet': 0.85, 'bombard': 0.95}

def render_unit_kind(kind):
    clear_models()
    scale = UNIT_SCALE.get(kind, 0.82)
    if kind in HUMANOIDS:
        R = make_humanoid(kind)
    elif kind in CAVALRY:
        R = make_horse(kind)
    else:
        R = make_siege(kind)
    root = R['root']
    for o in list(_created):
        if o.parent is None and o is not root:
            o.parent = root
    root.scale = (scale, scale, scale)
    W, H, oy = (176, 176, 140) if kind == 'trebuchet' else ((144, 160, 128) if kind in CAVALRY else ((128, 128, 100) if kind in SIEGE else (96, 104, 80)))
    def shoot(nm):
        for d in range(8):
            root.rotation_euler = (0, 0, math.radians(45 * d))
            render('%s_%d' % (nm, d if False else d), W, H, oy)
    for f in range(10):
        # pose
        if kind in HUMANOIDS:
            pose = _neutral() if f == 0 else walk_pose(f) if f <= 6 else attack_pose(kind, f - 7)
            apply_pose(R, pose)
            if kind == 'monk':
                R['orbL'].hide_render = not (f >= 7)
                R['orbL'].scale = (1.0 + 0.5 * ((f - 7) % 3 == 1), ) * 3 if f >= 7 else (1, 1, 1)
            if kind == 'handcannon':
                R['flash'].hide_render = f != 8
            if kind == 'archer':
                pass
        elif kind in CAVALRY:
            if f == 0: horse_pose(R, 1, 'idle')
            elif f <= 6: horse_pose(R, f, 'run')
            else: horse_attack(R, f - 7)
        else:
            if f == 0: siege_pose(R, 1, 'idle')
            elif f <= 6: siege_pose(R, f, 'move')
            else: siege_pose(R, 1, 'attack', f - 7)
        for d in range(8):
            nm = 'u_%s_blue_%d_%d' % (kind, d, f)
            if not (should(nm) or should('u_%s' % kind) or should('units')):
                continue
            root.rotation_euler = (0, 0, math.radians(45 * d))
            render(nm, W, H, oy)
    if kind == 'villager':
        for tool in TOOLS:
            T = _tool(R, tool)
            for f in range(4):
                apply_pose(R, work_pose(tool, f))
                for d in range(8):
                    nm = 'uw_%s_blue_%d_%d' % (tool, d, f)
                    if not (should(nm) or should('u_villager') or should('units')):
                        continue
                    root.rotation_euler = (0, 0, math.radians(45 * d))
                    render(nm, W, H, oy)
            bpy.data.objects.remove(T, do_unlink=True)
