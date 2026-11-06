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
# Upright solid of revolution: a rounded apex and a softly domed base.
# Its horizontal sections are circles, not a shallow extruded triangle.
triangle = bezier((0, -1.10), [
    ((.10, -1.10), (.16, -.98), (.22, -.84)),
    ((.46, -.40), (.77, .16), (.94, .48)),
    ((.99, .57), (.99, .70), (.86, .77)),
    ((.64, .90), (.32, .93), (0, .93)),
    ((-.32, .93), (-.64, .90), (-.86, .77)),
    ((-.99, .70), (-.99, .57), (-.94, .48)),
    ((-.77, .16), (-.46, -.40), (-.22, -.84)),
    ((-.16, -.98), (-.10, -1.10), (0, -1.10)),
])
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
        return .88+.18*math.cos(4*angle)
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
# Runtime faces are upright and centered in object space. The WebUI applies
# the selection thumbnail's camera/roll separately; it is not a model transform.
faces = [[scale, 0, 0, 0] for scale in
         [1.000, 1.012, .881, .773, .907, .863, 1.000, .995,
          .820, .846, .819, 1.030, .868]]
# A single small smoothing pass removes C0 circle seams without changing identity.
for index in (1, 4, 7, 12):
    values = catalog[index]
    catalog[index] = [round(.15*values[i-1]+.7*values[i]+.15*values[(i+1) % 64], 5) for i in range(64)]

# Rounded extrusion: depth and inward bevel width. Large bevels form inflated
# bodies; small bevels leave a genuinely planar front, unlike a shared sphere.
surfaces = [[1, 1], [.62, .65], [.34, .25], [.38, .32], [.54, .56],
            [.68, .72], [.88, 1.05], [.73, .73], [1, 1], [.36, .27],
            [.32, .26], [.40, .30], [.42, .40]]
rings = [0, .2, .4, .6, .75, .86, .94, .985, 1]
directions = [(math.cos(-math.pi+TAU*i/64), math.sin(-math.pi+TAU*i/64)) for i in range(64)]
contours = [[(r*x, r*y) for r, (x, y) in zip(row, directions)] for row in catalog]


def cone_radius(y):
    hits = []
    points = contours[8]
    for (ax, ay), (bx, by) in zip(points, points[1:]+points[:1]):
        if abs(by-ay) > 1e-9 and min(ay, by) <= y <= max(ay, by):
            hits.append(abs(ax+(bx-ax)*(y-ay)/(by-ay)))
    return max(hits, default=0)


cone_min = min(y for x, y in contours[8])
cone_max = max(y for x, y in contours[8])
cone_profile = [round(cone_radius(cone_min+(cone_max-cone_min)*i/64), 7) for i in range(65)]

def height(index, x, y):
    if index == 8:
        return math.sqrt(max(0, cone_radius(y)**2-x*x))
    distance = float('inf')
    for (ax, ay), (bx, by) in zip(contours[index], contours[index][1:]+contours[index][:1]):
        dx, dy = bx-ax, by-ay
        t = max(0, min(1, ((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy)))
        distance = min(distance, math.hypot(x-ax-t*dx, y-ay-t*dy))
    depth, bevel = surfaces[index]
    t = max(0, 1-distance/bevel)
    return depth*math.sqrt(max(0, 1-t*t))


# All decals share the runtime model coordinates, including its neutral pose.
surface_faces = [list(face) for face in faces]
depths = [[[round(height(index, r*x, r*y)*16384) if r < 1 else 0
            for x, y in contours[index]] for r in rings] for index in range(13)]
header = "/* Generated by scripts/generate-interaction-shapes.py; original geometry. */\nstatic const float shape_radii[13][64] = {\n"
header += ",\n".join("    {" + ",".join(f"{v:.5f}f" for v in row) + "}" for row in catalog)
header += "\n};\n"
(ROOT / "components/agent_interaction/interaction_surface_data.h").write_text(
    "/* Generated shape-specific rounded solids. Depths are Q14. */\n"+
    f"static const float cone_min_y = {cone_min:.7f}f, cone_max_y = {cone_max:.7f}f;\n"+
    "static const float cone_profile[65] = {"+",".join(f"{v:.7f}f" for v in cone_profile)+"};\n"+
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
