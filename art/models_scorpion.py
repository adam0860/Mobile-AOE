# Scorpion emplacement (2x2): a giant fixed crossbow on a stone and timber platform (exec'd by gen_assets.py)

def m_scorpion(style=2):
    # platform
    box(0, 0, 0, 1.7, 1.7, 0.22, M['stone'])
    box(0, 0, 0.22, 1.5, 1.5, 0.05, M['woodDark'])
    crenellations(0, 0, 0.0, 1.7, 1.7, M['stone'], n=6, hgt=0.38)
    # pivot post and carriage
    cyl(0, 0, 0.27, 0.12, 0.42, M['woodDark'], verts=8)
    box(0.05, 0, 0.62, 1.1, 0.16, 0.12, M['wood'])                       # stock (points +X)
    box(-0.55, 0, 0.58, 0.14, 0.34, 0.2, M['wood'])                      # winch housing
    cyl(-0.58, 0.24, 0.62, 0.1, 0.05, M['iron'], verts=10, rx=math.pi / 2)  # winch wheel
    for k in range(6):
        a = k * math.pi / 3
        box(-0.58 + math.cos(a) * 0.09, 0.27, 0.62 + math.sin(a) * 0.09, 0.03, 0.03, 0.03, M['woodLight'])
    # bow arms swept back + string
    for s in (-1, 1):
        arm = box(0.42, s * 0.42, 0.68, 0.18, 0.78, 0.07, M['woodLight'])
        arm.rotation_euler = (0, 0, s * math.radians(-18))
        box(0.5, s * 0.78, 0.68, 0.05, 0.08, 0.07, M['iron'])
    box(0.28, 0, 0.7, 0.012, 1.55, 0.012, M['white'])                    # string
    # loaded bolt with a steel head
    cyl(-0.3, 0, 0.73, 0.03, 1.55, M['wood'], verts=6, ry=math.radians(90))
    cone(1.25, 0, 0.73, 0.06, 0.2, M['metal'], verts=6).rotation_euler = (0, math.radians(90), 0)
    for s in (-1, 1):
        box(-0.2, s * 0.03, 0.76, 0.12, 0.012, 0.05, M['hide'])          # fletching
    # reserve bolts + barrel + banner
    for i in range(4):
        cyl(-0.45, -0.55 + i * 0.06, 0.3, 0.02, 0.8, M['wood'], verts=5, ry=math.radians(90))
    barrel(-0.5, 0.55, 0.0)
    flag(-0.72, -0.72, 0.38, 0.9, 'blue')
