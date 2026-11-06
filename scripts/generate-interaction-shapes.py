#!/usr/bin/env python3
"""Generate the standalone firmware radial silhouettes and rounded surface tables.

No data or code from the Grok Bot bundle is used. Run from any directory.
"""
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TAU = 2 * math.pi


def bezier(start, curves):
    points = [start]
    for a, b, end in curves:
        for i in range(1, 65):
            t = i / 64
            points.append(tuple((1-t)**3*start[k]+3*(1-t)**2*t*a[k]+3*(1-t)*t*t*b[k]+t**3*end[k] for k in (0, 1)))
        start = end
    return points


def rounded(vertices, amount):
    points = []
    for i, v in enumerate(vertices):
        before, after = vertices[i-1], vertices[(i+1) % len(vertices)]
        a = tuple(v[k] + (before[k]-v[k])*amount for k in (0, 1))
        b = tuple(v[k] + (after[k]-v[k])*amount for k in (0, 1))
        for j in range(17):
            t = j/16
            points.append(tuple((1-t)**2*a[k]+2*(1-t)*t*v[k]+t*t*b[k] for k in (0, 1)))
    return points


def radial(points, angle):
    dx, dy = math.cos(angle), math.sin(angle)
    hits = []
    for (x, y), (ex, ey) in zip(points, points[1:] + points[:1]):
        vx, vy = ex-x, ey-y
        cross = dx*vy-dy*vx
        if abs(cross) < 1e-9:
            continue
        r, t = (x*vy-y*vx)/cross, (x*dy-y*dx)/cross
        if r > 0 and 0 <= t <= 1:
            hits.append(r)
    assert hits, (angle, points)
    return max(hits)


def circles(items, angle):
    dx, dy = math.cos(angle), math.sin(angle)
    hits = []
    for x, y, radius in items:
        dot = x*dx+y*dy
        discriminant = dot*dot+radius*radius-x*x-y*y
        if discriminant >= 0:
            hits.append(dot+math.sqrt(discriminant))
    return max(hits)


# Keep the lobes, but join the lower curves with a horizontal tangent.
# The old opposing diagonal tangents met in a sharp tip at y=.99.
heart = bezier((0, -.62), [
    ((.38, -1.16), (1.02, -.91), (.98, -.24)),
    ((.96, .27), (.32, .90), (0, .90)),
    ((-.32, .90), (-.96, .27), (-.98, -.24)),
    ((-1.02, -.91), (-.38, -1.16), (0, -.62)),
])
# Round belly and broad, rounded crown, calibrated against the supplied avatar.
# The previous pear-shaped curve pinched the shoulders and flattened the belly.
drop = bezier((0, -1.38), [((.28, -1.38), (.47, -1.04), (.80, -.60)), ((.93, -.43), (1, -.22), (1, 0)), ((1, .5523), (.5523, 1), (0, 1)), ((-.5523, 1), (-1, .5523), (-1, 0)), ((-1, -.22), (-.93, -.43), (-.80, -.60)), ((-.47, -1.04), (-.28, -1.38), (0, -1.38))])
drop = [(x*1.03, y) for x, y in drop]
square = rounded([(-.85, -.85), (.85, -.85), (.85, .85), (-.85, .85)], .20)
triangle = rounded([(0, -1.10), (1.02, .76), (-1.02, .76)], .22)
triangle = [(x*math.cos(math.pi/2+.42)-y*math.sin(math.pi/2+.42), x*math.sin(math.pi/2+.42)+y*math.cos(math.pi/2+.42)) for x, y in triangle]
house = rounded([(0, -1.06), (.82, -.48), (.82, .87), (-.82, .87), (-.82, -.48)], .18)
hexagon = rounded([(math.cos(TAU*i/6)*1.04, math.sin(TAU*i/6)*1.04) for i in range(6)], .18)


def shape(index, angle):
    if index == 0:
        return 1
    if index == 1:
        return circles([(-.35, -.30, .64), (.27, -.35, .57), (.55, .10, .51), (.06, .36, .64), (-.48, .34, .47)], angle)
    if index in (2, 5, 6, 8, 9, 11):
        return radial({2: square, 5: heart, 6: drop, 8: triangle, 9: house, 11: hexagon}[index], angle)
    if index == 3:
        return .80+.26*math.cos(4*angle)
    if index == 4:
        return circles([(-.34, -.34, .62), (.34, -.34, .62), (.34, .34, .62), (-.34, .34, .62)], angle)
    if index == 7:
        if abs(math.sin(angle)) > 1e-9:
            side = .73/abs(math.sin(angle))
            if abs(side*math.cos(angle)) <= .37:
                return side
        return circles([(-.37, 0, .73), (.37, 0, .73)], angle)
    if index == 10:
        return .87+.20*math.cos(6*angle)
    return circles([(.57*math.cos(TAU*i/5-math.pi/2), .57*math.sin(TAU*i/5-math.pi/2), .48) for i in range(5)], angle)


catalog = [[round(shape(index, -math.pi+TAU*i/64), 5) for i in range(64)] for index in range(13)]
# Face scale, anchor x/y, and eye-roll offset (in body-radius units/radians).
# Measured against the user's selection screenshot. A shared centre/scale makes
# eyes escape the narrow arms of stars and the crown of the triangle.
faces = [
    [1.000, .000, .000, -.171], [1.012, -.172, -.030, -.034],
    [.881, -.306, .190, -.038], [.773, -.217, .054, -.040],
    [.907, -.296, .065, -.039], [.863, -.108, -.027, .021],
    [1.000, .000, .000, -.005], [.995, -.131, .013, -.017],
    [.740, -.065, .154, -.490], [.846, -.306, .208, -.028],
    [.819, -.265, .191, -.027], [1.030, -.362, .225, -.045],
    [.868, -.284, .198, -.040],
]
# A single small smoothing pass removes C0 circle seams without changing identity.
for index in (1, 4, 7, 12):
    values = catalog[index]
    catalog[index] = [round(.15*values[i-1]+.7*values[i]+.15*values[(i+1) % 64], 5) for i in range(64)]

# Rounded extrusion: depth and inward bevel width. Large bevels form inflated
# bodies; small bevels leave a genuinely planar front, unlike a shared sphere.
surfaces = [[1, 1], [.62, .65], [.34, .25], [.38, .32], [.54, .56],
            [.68, .72], [.88, 1.05], [.73, .73], [.32, .16], [.36, .27],
            [.32, .26], [.40, .30], [.42, .40]]
rings = [0, .2, .4, .6, .75, .86, .94, .985, 1]
directions = [(math.cos(-math.pi+TAU*i/64), math.sin(-math.pi+TAU*i/64)) for i in range(64)]
contours = [[(r*x, r*y) for r, (x, y) in zip(row, directions)] for row in catalog]


def height(index, x, y):
    distance = float('inf')
    for (ax, ay), (bx, by) in zip(contours[index], contours[index][1:]+contours[index][:1]):
        dx, dy = bx-ax, by-ay
        t = max(0, min(1, ((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy)))
        distance = min(distance, math.hypot(x-ax-t*dx, y-ay-t*dy))
    depth, bevel = surfaces[index]
    t = max(0, 1-distance/bevel)
    return depth*math.sqrt(max(0, 1-t*t))


# Preserve the calibrated resting face position while changing its surface.
# Solve the front-surface anchor in object space, then keep it fixed there.
surface_faces = []
cy, sy, cp, sp = math.cos(.58), math.sin(.58), math.cos(.28), math.sin(.28)
for index, (scale, ax, ay, roll) in enumerate(faces):
    cr, sr = math.cos(roll), math.sin(roll)
    target_x = ax+scale*(sy*cp*cr+sp*sr)
    target_y = ay+scale*(sy*cp*sr-sp*cr)
    x, y = ax, ay
    for _ in range(40):
        z = height(index, x, y)
        y = .5*y+.5*(target_y+z*sp)/cp
        x = .5*x+.5*(target_x-(y*sp+z*cp)*sy)/cy
    surface_faces.append([round(scale, 5), round(x, 5), round(y, 5), roll])
# A triangle's front is narrower near its upper edge. Keep even surprised eyes
# on the central face instead of carrying the old spherical anchor to its rim.
surface_faces[8][1:3] = [-.04, .02]
depths = [[[round(height(index, r*x, r*y)*16384) if r < 1 else 0
            for x, y in contours[index]] for r in rings] for index in range(13)]
header = "/* Generated by scripts/generate-interaction-shapes.py; original geometry. */\nstatic const float shape_radii[13][64] = {\n"
header += ",\n".join("    {" + ",".join(f"{v:.5f}f" for v in row) + "}" for row in catalog)
header += "\n};\n"
(ROOT / "components/agent_interaction/interaction_surface_data.h").write_text(
    "/* Generated shape-specific rounded solids. Depths are Q14. */\n"+
    "static const float surface_profiles[13][2] = {\n"+
    ",\n".join("{"+",".join(f"{v:.5f}f" for v in row)+"}" for row in surfaces)+"\n};\n"+
    "static const float surface_faces[13][4] = {\n"+
    ",\n".join("{"+",".join(f"{v:.5f}f" for v in row)+"}" for row in surface_faces)+"\n};\n"+
    "static const float surface_rings[9] = {"+",".join(f"{v:.5f}f" for v in rings)+"};\n"+
    "static const float surface_directions[64][2] = {\n"+
    ",\n".join("{"+",".join(f"{v:.7f}f" for v in row)+"}" for row in directions)+"\n};\n"+
    "static const uint16_t surface_depths[13][9][64] = {\n"+
    ",\n".join("{"+",".join("{"+",".join(map(str, row))+"}" for row in shape)+"}" for shape in depths)+"\n};\n")
header += "/* scale, anchor x, anchor y, roll offset */\nstatic const float face_profiles[13][4] = {\n"
header += ",\n".join("    {" + ",".join(f"{v:.5f}f" for v in row) + "}" for row in faces)
header += "\n};\n"
(ROOT / "components/agent_interaction/interaction_shapes.h").write_text(header)
