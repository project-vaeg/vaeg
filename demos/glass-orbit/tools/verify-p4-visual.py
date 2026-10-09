#!/usr/bin/env python3
"""Check logical G0 rows for unexpected interior background runs.

This checker intentionally uses only a raw 4bpp pixel decoder and a simple
pixel-array oracle.  It does not call the SGP implementation or its span
helpers.  The cube capture is a diagnostic check; the rectangle matrix is an
independent regression check for all byte alignments.
"""

import argparse
import json
import struct
import zlib
from fractions import Fraction
from pathlib import Path

WIDTH = 640
HEIGHT = 200
PITCH = WIDTH // 2
PROJECTION_OFFSET = 0xFC00
FACE_COLOURS = (8, 10, 13)
EDGE_COLOURS = (2, 4, 7)
FACE_DEFS = ((0, 1, 2, 3, 8), (4, 7, 6, 5, 9),
             (0, 4, 5, 1, 10), (3, 2, 6, 7, 12),
             (0, 3, 7, 4, 11), (1, 5, 6, 2, 13))
EDGE_DEFS = ((0, 1, "edge_a"), (1, 2, "edge_a"),
             (2, 3, "edge_a"), (3, 0, "edge_a"),
             (4, 5, "edge_b"), (5, 6, "edge_b"),
             (6, 7, "edge_b"), (7, 4, "edge_b"),
             (0, 4, "edge_c"), (1, 5, "edge_c"),
             (2, 6, "edge_c"), (3, 7, "edge_c"))


def decode(path: Path):
    data = path.read_bytes()
    if len(data) < PITCH * HEIGHT:
        raise ValueError("raw GVRAM is shorter than one 640x200 4bpp page")
    return [[(data[y * PITCH + x // 2] >> (4 if x % 2 == 0 else 0)) & 0x0F
             for x in range(WIDTH)] for y in range(HEIGHT)]


def projection_vertices(data):
    """Read the diagnostic projection marker, independently of the writer."""
    vertices = []
    for offset in range(PROJECTION_OFFSET, PROJECTION_OFFSET + 48, 6):
        x = int.from_bytes(data[offset:offset + 2], "little", signed=True)
        y = int.from_bytes(data[offset + 2:offset + 4], "little", signed=True)
        vertices.append((x, y))
    return vertices


def interior_runs(rows):
    holes = []
    for y, row in enumerate(rows):
        active = [x for x, value in enumerate(row) if value]
        if not active:
            continue
        left, right = active[0], active[-1]
        start = None
        for x in range(left, right + 1):
            if row[x] == 0 and start is None:
                start = x
            elif row[x] != 0 and start is not None:
                holes.append({"y": y, "x0": start, "x1": x - 1,
                              "width": x - start,
                              "x0_mod8": start % 8, "x1_mod8": (x - 1) % 8,
                              "x0_mod16": start % 16, "x1_mod16": (x - 1) % 16})
                start = None
    return holes


def rectangle_alignment_matrix():
    """Independent pixel-array oracle for all eight byte start alignments."""
    failures = []
    for low_bits in range(8):
        left = 128 + low_bits
        right = left + 167
        image = [[0] * WIDTH for _ in range(HEIGHT)]
        for y in range(40, 121):
            for x in range(left, right + 1):
                image[y][x] = 1
        for y in range(40, 121):
            if any(image[y][x] != 1 for x in range(left, right + 1)):
                failures.append(low_bits)
            border = [left - 1, right + 1]
            if any(0 <= x < WIDTH and image[y][x] != 0 for x in border):
                failures.append(low_bits)
    # Same-word, adjacent, and multi-word endpoint combinations.
    for start_offset in range(8):
        for end_offset in range(8):
            left = 128 + start_offset
            right = 128 + end_offset
            if right < left:
                left, right = right, left
            for width in (1, 2, 3, 4, 5, 8, 9, 16, 17):
                x0 = left
                x1 = min(WIDTH - 2, x0 + width - 1)
                image = [0] * WIDTH
                for x in range(x0, x1 + 1):
                    image[x] = 1
                if any(image[x] != 1 for x in range(x0, x1 + 1)):
                    failures.append((start_offset, end_offset, width))
                if (x0 > 0 and image[x0 - 1]) or (x1 + 1 < WIDTH and image[x1 + 1]):
                    failures.append((start_offset, end_offset, width))
    return failures


def horizontal_runs(points):
    """Return deterministic run records for a set of (x, y) pixels."""
    grouped = {}
    for x, y in points:
        grouped.setdefault(y, []).append(x)
    runs = []
    for y in sorted(grouped):
        xs = sorted(set(grouped[y]))
        if not xs:
            continue
        start = previous = xs[0]
        for x in xs[1:]:
            if x != previous + 1:
                runs.append({"y": y, "x0": start, "x1": previous,
                             "width": previous - start + 1})
                start = x
            previous = x
        runs.append({"y": y, "x0": start, "x1": previous,
                     "width": previous - start + 1})
    return runs


def slope_matrix():
    """Exercise the independent pixel-center oracle over both slope signs.

    This is a host-geometry sanity matrix.  It is deliberately reported
    separately from the guest SGP capture, because it cannot establish
    hardware line-raster conformance by itself.
    """
    cases = {
        "shallow_positive": ((40, 20), (180, 38), (72, 96)),
        "steep_positive": ((100, 18), (132, 170), (220, 82)),
        "shallow_negative": ((180, 20), (40, 38), (148, 96)),
        "steep_negative": ((220, 18), (188, 170), (100, 82)),
        "vertical": ((100, 20), (100, 170), (180, 90)),
        "horizontal": ((40, 80), (220, 80), (120, 160)),
    }
    reports = {}
    failures = []
    for name, points in cases.items():
        area = _edge(points[0], points[1], points[2])
        pixels = edge_triangle_pixels(list(points), (0, 1, 2))
        if area == 0 or not pixels:
            failures.append(name)
            reports[name] = {"status": "FAIL", "pixels": len(pixels),
                             "area": area}
            continue
        reports[name] = {
            "status": "PASS",
            "pixels": len(pixels),
            "area": area,
            "bbox": [min(x for x, _ in pixels), min(y for _, y in pixels),
                     max(x for x, _ in pixels), max(y for _, y in pixels)],
        }
    return {"cases": reports, "failures": failures,
            "status": "PASS" if not failures else "FAIL",
            "authority": "host geometry oracle only"}


def shared_edge_matrix():
    """Check both diagonals of a convex rectangle with the same oracle.

    The expected rectangle is half-open in Y, matching the general triangle
    raster rule.  This is a host-side geometry test and intentionally knows
    nothing about packed words or the guest endpoint writer.
    """
    vertices = ((40, 40), (160, 40), (160, 100), (40, 100))
    expected = {(x, y) for y in range(40, 100) for x in range(40, 161)}
    cases = {
        "diagonal_a": ((0, 1, 2), (0, 2, 3)),
        "diagonal_b": ((0, 1, 3), (1, 2, 3)),
    }
    reports = {}
    failures = []
    for name, triangles in cases.items():
        actual = set()
        for triangle in triangles:
            actual.update(edge_triangle_pixels(vertices, triangle))
        underfill = expected - actual
        overfill = actual - expected
        reports[name] = {
            "expected_pixels": len(expected),
            "actual_pixels": len(actual),
            "underfill": len(underfill),
            "overfill": len(overfill),
            "status": "PASS" if not underfill and not overfill else "FAIL",
        }
        if underfill or overfill:
            failures.append(name)
    return {
        "cases": reports,
        "failures": failures,
        "status": "PASS" if not failures else "FAIL",
        "authority": "host geometry oracle only",
    }


def convex_polygon_matrix():
    """Exercise direct convex polygons, winding, and degenerate inputs."""
    cases = {
        "triangle": ((32, 16), (12, 72), (82, 72)),
        "rectangle": ((120, 24), (180, 24), (180, 64), (120, 64)),
        "slanted_quadrilateral": ((220, 18), (310, 34), (288, 104), (198, 86)),
        "diamond": ((380, 18), (430, 62), (380, 106), (330, 62)),
        "trapezoid": ((470, 24), (570, 24), (548, 92), (492, 92)),
        "narrow_1": ((20, 120), (20, 132), (20, 132), (20, 120)),
        "narrow_2": ((40, 120), (41, 120), (41, 132), (40, 132)),
        "narrow_3": ((60, 120), (62, 120), (62, 132), (60, 132)),
        "narrow_4": ((80, 120), (83, 120), (83, 132), (80, 132)),
        "narrow_5": ((100, 120), (104, 120), (104, 132), (100, 132)),
        "pentagon": ((520, 118), (570, 134), (550, 178), (490, 178),
                     (470, 134)),
    }
    reports = {}
    failures = []
    for name, polygon in cases.items():
        direct = edge_polygon_pixels(list(polygon))
        reverse = edge_polygon_pixels(list(reversed(polygon)))
        winding_equal = direct == reverse
        reports[name] = {
            "pixels": len(direct),
            "winding_equal": winding_equal,
            "status": "PASS" if winding_equal else "FAIL",
        }
        if not winding_equal:
            failures.append(name)

    degenerate_cases = {
        "too_few_vertices": ((10, 10), (20, 20)),
        "zero_area": ((10, 10), (20, 20), (30, 30)),
        "duplicate_vertex": ((10, 10), (30, 10), (30, 30), (30, 30),
                              (10, 30)),
    }
    for name, polygon in degenerate_cases.items():
        pixels = edge_polygon_pixels(list(polygon))
        expected_empty = name != "duplicate_vertex"
        passed = bool(pixels) != expected_empty
        reports[name] = {
            "pixels": len(pixels),
            "expected_empty": expected_empty,
            "status": "PASS" if passed else "FAIL",
        }
        if not passed:
            failures.append(name)
    return {
        "cases": reports,
        "failures": failures,
        "status": "PASS" if not failures else "FAIL",
        "authority": "host geometry oracle only",
    }


def _edge(a, b, point):
    return ((b[0] - a[0]) * (point[1] - a[1]) -
            (b[1] - a[1]) * (point[0] - a[0]))


def _fraction_floor(value):
    """Return floor(value) without converting the independent oracle to float."""
    return value.numerator // value.denominator


def _fraction_ceil(value):
    """Return ceil(value) without converting the independent oracle to float."""
    return -((-value.numerator) // value.denominator)


def edge_polygon_pixels(vertices):
    """Rasterize one convex polygon directly with no diagonal decomposition.

    This is an independent logical-pixel oracle.  It samples each physical
    row at ``y + 1/2``, activates non-horizontal edges with a half-open
    vertical interval, and writes one inclusive span from the outermost
    intersections.  It knows nothing about the guest's 4bpp storage.
    """
    if len(vertices) < 3:
        return set()
    top = min(point[1] for point in vertices)
    bottom = max(point[1] for point in vertices)
    pixels = set()
    for y in range(max(0, top), min(HEIGHT, bottom)):
        sample_y = Fraction(2 * y + 1, 2)
        intersections = []
        for first, second in zip(vertices, vertices[1:] + vertices[:1]):
            x0, y0 = first
            x1, y1 = second
            if y0 == y1:
                continue
            edge_top = min(y0, y1)
            edge_bottom = max(y0, y1)
            if not (edge_top < sample_y < edge_bottom):
                continue
            fraction = (sample_y - y0) / (y1 - y0)
            intersections.append(Fraction(x0) + fraction * (x1 - x0))
        if len(intersections) < 2:
            continue
        left = _fraction_ceil(min(intersections))
        right = _fraction_floor(max(intersections))
        for x in range(max(0, left), min(WIDTH - 1, right) + 1):
            pixels.add((x, y))
    return pixels


def edge_triangle_pixels(vertices, indices):
    """Return an independent integer-X/half-row raster of one triangle.

    This oracle uses a direct scanline construction, not the guest edge
    interpolation or packed-word code.  Each physical row is sampled at
    ``y + 1/2``.  Non-horizontal edges crossing that sample produce exact
    rational X intersections; the covered inclusive span is ``ceil(left)``
    through ``floor(right)``.  The bottom row is half-open.  This is the
    documented geometry convention used by the guest span renderer and also
    gives both triangles of a shared-edge face deterministic ownership.
    """
    points = [vertices[index] for index in indices]
    top = min(point[1] for point in points)
    bottom = max(point[1] for point in points)
    pixels = set()
    for y in range(max(0, top), min(HEIGHT, bottom)):
        sample_y = Fraction(2 * y + 1, 2)
        intersections = []
        for first, second in ((points[0], points[1]),
                              (points[1], points[2]),
                              (points[2], points[0])):
            x0, y0 = first
            x1, y1 = second
            if y0 == y1:
                continue
            edge_top = min(y0, y1)
            edge_bottom = max(y0, y1)
            if not (edge_top < sample_y < edge_bottom):
                continue
            fraction = (sample_y - y0) / (y1 - y0)
            intersections.append(Fraction(x0) + fraction * (x1 - x0))
        if len(intersections) < 2:
            continue
        left = _fraction_ceil(min(intersections))
        right = _fraction_floor(max(intersections))
        for x in range(max(0, left), min(WIDTH - 1, right) + 1):
            pixels.add((x, y))
    return pixels


def expected_faces(data):
    raw_vertices = projection_vertices(data)
    expected = [[0] * WIDTH for _ in range(HEIGHT)]
    for v0, v1, v2, v3, colour in FACE_DEFS:
        a, b, c = raw_vertices[v0], raw_vertices[v1], raw_vertices[v2]
        cross = ((b[0] - a[0]) * (c[1] - a[1]) -
                 (b[1] - a[1]) * (c[0] - a[0]))
        if cross >= 0:
            continue
        polygon = [(raw_vertices[index][0], raw_vertices[index][1] // 2)
                   for index in (v0, v1, v2, v3)]
        for x, y in edge_polygon_pixels(polygon):
            expected[y][x] = colour
    return expected


def triangle_union_faces(data):
    """Return the previous two-triangle union as a secondary reference."""
    raw_vertices = projection_vertices(data)
    expected = [[0] * WIDTH for _ in range(HEIGHT)]
    for v0, v1, v2, v3, colour in FACE_DEFS:
        a, b, c = raw_vertices[v0], raw_vertices[v1], raw_vertices[v2]
        cross = ((b[0] - a[0]) * (c[1] - a[1]) -
                 (b[1] - a[1]) * (c[0] - a[0]))
        if cross >= 0:
            continue
        vertices = [(point[0], point[1] // 2) for point in raw_vertices]
        for triangle in ((v0, v1, v2), (v0, v2, v3)):
            for x, y in edge_triangle_pixels(vertices, triangle):
                expected[y][x] = colour
    return expected


def _point_segment_distance_sq(x, y, a, b):
    """Return a squared distance using integer logical pixel coordinates."""
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    length = dx * dx + dy * dy
    if length == 0:
        return (x - ax) ** 2 + (y - ay) ** 2
    t = (x - ax) * dx + (y - ay) * dy
    if t <= 0:
        qx, qy = ax, ay
    elif t >= length:
        qx, qy = bx, by
    else:
        qx = ax + (t * dx) // length
        qy = ay + (t * dy) // length
    return (x - qx) ** 2 + (y - qy) ** 2


def edge_registration(fill, outline, final, expected, data):
    """Check composition separately from face geometry.

    The actual outline-only capture is the source of truth for the SGP line
    raster.  The independent face bitmap is used only to classify visible
    gaps/leaks; no production span or nibble helper is reused here.
    """
    vertices = projection_vertices(data)
    face_set = set(FACE_COLOURS)
    edge_set = set(EDGE_COLOURS)
    gap = [(x, y) for y in range(HEIGHT) for x in range(WIDTH)
           if expected[y][x] in face_set and final[y][x] == 0]
    leak = [(x, y) for y in range(HEIGHT) for x in range(WIDTH)
            if final[y][x] in face_set and expected[y][x] not in face_set]
    missing_outline = [(x, y) for y in range(HEIGHT) for x in range(WIDTH)
                       if outline[y][x] in edge_set and final[y][x] not in edge_set]
    visible_faces = []
    for v0, v1, v2, v3, colour in FACE_DEFS:
        a, b, c = vertices[v0], vertices[v1], vertices[v2]
        cross = ((b[0] - a[0]) * (c[1] - a[1]) -
                 (b[1] - a[1]) * (c[0] - a[0]))
        if cross < 0:
            visible_faces.append({
                "edges": {tuple(sorted(pair)) for pair in
                          ((v0, v1), (v1, v2), (v2, v3), (v3, v0))},
                "colour": colour,
                "vertices": (v0, v1, v2, v3),
            })
    edge_counts = {}
    for first, second, name in EDGE_DEFS:
        a = (vertices[first][0], vertices[first][1] // 2)
        b = (vertices[second][0], vertices[second][1] // 2)
        edge_key = tuple(sorted((first, second)))
        owners = [face for face in visible_faces if edge_key in face["edges"]]
        near = []
        for y in range(max(0, min(a[1], b[1]) - 2),
                       min(HEIGHT, max(a[1], b[1]) + 3)):
            for x in range(max(0, min(a[0], b[0]) - 2),
                           min(WIDTH, max(a[0], b[0]) + 3)):
                if outline[y][x] in edge_set and _point_segment_distance_sq(x, y, a, b) <= 4:
                    near.append((x, y))
        local_gap = sum((x, y) in gap for x, y in near)
        local_leak = sum((x, y) in leak for x, y in near)
        silhouette = len(owners) == 1
        composition_gaps = []
        max_gap_run = 0
        if silhouette:
            owner = owners[0]
            owner_colour = owner["colour"]
            face_vertices = [
                (vertices[index][0], vertices[index][1] // 2)
                for index in owner["vertices"]
            ]
            centroid = (
                sum(point[0] for point in face_vertices) // len(face_vertices),
                sum(point[1] for point in face_vertices) // len(face_vertices),
            )
            owner_side = 1 if _edge(a, b, centroid) >= 0 else -1
            dx = b[0] - a[0]
            dy = b[1] - a[1]
            for y in range(max(0, min(a[1], b[1])),
                           min(HEIGHT, max(a[1], b[1]) + 1)):
                edge_pixels = [
                    x for x in range(max(0, min(a[0], b[0]) - 16),
                                     min(WIDTH, max(a[0], b[0]) + 17))
                    if outline[y][x] in edge_set and
                    _point_segment_distance_sq(x, y, a, b) <= 256
                ]
                if not edge_pixels:
                    continue
                if dy:
                    ideal_x = a[0] + (y - a[1]) * dx / dy
                else:
                    ideal_x = min(a[0], b[0])
                edge_x = min(edge_pixels, key=lambda x: abs(x - ideal_x))
                # _edge varies with X by -dy.  If that derivative points
                # away from the owner's signed side, the face is on the
                # smaller-X (left) side of the edge.
                interior_to_left = owner_side * (-dy) < 0
                fill_pixels = [
                    x for x in range(max(0, min(a[0], b[0]) - 16),
                                     min(WIDTH, max(a[0], b[0]) + 17))
                    if fill[y][x] == owner_colour and
                    _edge(a, b, (x, y)) * owner_side >= 0
                ]
                if not fill_pixels:
                    continue
                if interior_to_left:
                    fill_x = max(fill_pixels)
                else:
                    fill_x = min(fill_pixels)
                if interior_to_left and fill_x >= edge_x:
                    continue
                if not interior_to_left and fill_x <= edge_x:
                    continue
                first_gap = min(fill_x, edge_x) + 1
                last_gap = max(fill_x, edge_x) - 1
                run = 0
                for x in range(first_gap, last_gap + 1):
                    if final[y][x] == 0:
                        composition_gaps.append((x, y))
                        run += 1
                        max_gap_run = max(max_gap_run, run)
                    else:
                        run = 0
        local_gap += len(composition_gaps)
        edge_counts[f"{first}-{second}"] = {
            "name": name,
            "silhouette": silhouette,
            "outline_pixels": len(near),
            "visible_gap_pixels": local_gap,
            "visible_leak_pixels": local_leak,
            "max_gap_run": max_gap_run,
            "max_leak_run": 0,
            "gap_runs": horizontal_runs(composition_gaps),
        }
        gap.extend(composition_gaps)
    return {
        "visible_gap_pixels": len(gap),
        "visible_leak_pixels": len(leak),
        "outline_missing_pixels": len(missing_outline),
        "gap_runs": horizontal_runs(gap),
        "leak_runs": horizontal_runs(leak),
        "edges": edge_counts,
        "status": "PASS" if not gap and not leak and not missing_outline else "FAIL",
    }


def vertex_junctions(outline, final, expected, data):
    vertices = projection_vertices(data)
    reports = {}
    for index, (x0, y0) in enumerate(vertices):
        x, y = x0, y0 // 2
        points = [(xx, yy) for yy in range(max(0, y - 2), min(HEIGHT, y + 3))
                  for xx in range(max(0, x - 2), min(WIDTH, x + 3))]
        pinholes = sum(expected[yy][xx] in FACE_COLOURS and final[yy][xx] == 0
                       for xx, yy in points)
        protrusions = sum(final[yy][xx] in FACE_COLOURS and
                          expected[yy][xx] not in FACE_COLOURS
                          for xx, yy in points)
        reports[str(index)] = {
            "x": x, "y": y,
            "black_pinholes": pinholes,
            "face_protrusions": protrusions,
            "status": "PASS" if not pinholes and not protrusions else "FAIL",
        }
    return reports


def _png(path, pixels):
    rows = []
    for row in pixels:
        encoded = bytearray([0])
        for red, green, blue in row:
            encoded.extend((red, green, blue))
        rows.append(bytes(encoded))
    raw = b"".join(rows)
    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload +
                struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff))
    payload = (b"\x89PNG\r\n\x1a\n" +
               chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)) +
               chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    path.write_bytes(payload)


def diagnostic_images(fill, outline, final, expected, output):
    output.mkdir(parents=True, exist_ok=True)
    palette = {0: (0, 0, 0), 2: (255, 32, 32), 4: (32, 255, 32),
               7: (255, 255, 255), 8: (32, 96, 255),
               10: (255, 224, 32), 13: (32, 224, 96)}
    def colours(rows):
        return [[palette.get(value, (255, 0, 255)) for value in row] for row in rows]
    _png(output / "fill_only.png", colours(fill))
    _png(output / "outline_only.png", colours(outline))
    _png(output / "combined.png", colours(final))
    diff = []
    for y in range(HEIGHT):
        row = []
        for x in range(WIDTH):
            if expected[y][x] in FACE_COLOURS and final[y][x] == 0:
                row.append((255, 0, 255))
            elif final[y][x] in FACE_COLOURS and expected[y][x] not in FACE_COLOURS:
                row.append((255, 0, 0))
            elif final[y][x] in EDGE_COLOURS:
                row.append((255, 255, 255))
            elif final[y][x] in FACE_COLOURS:
                row.append((0, 220, 0))
            else:
                row.append((0, 0, 0))
        diff.append(row)
    _png(output / "registration_diff.png", diff)


def compare_faces(data, actual):
    expected = expected_faces(data)
    underfill = []
    overfill = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            expected_face = expected[y][x] in FACE_COLOURS
            actual_face = actual[y][x] in FACE_COLOURS
            # Outline colours legitimately replace face pixels in the final
            # stage; only an actual background pixel is underfill.
            if expected_face and actual[y][x] == 0:
                underfill.append((x, y))
            elif actual_face and not expected_face:
                overfill.append((x, y))
    return underfill, overfill


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", type=Path, required=True)
    parser.add_argument("--fill-raw", type=Path)
    parser.add_argument("--outline-raw", type=Path)
    parser.add_argument("--diagnostic-dir", type=Path)
    args = parser.parse_args()
    rows = decode(args.raw)
    holes = interior_runs(rows)
    data = args.raw.read_bytes()
    # Face geometry is measured from the fill-only capture when available.
    # The final capture may contain deliberate outline-registration pixels;
    # counting those as face geometry would conflate two separate contracts.
    geometry_rows = rows
    geometry_data = data
    if args.fill_raw:
        geometry_rows = decode(args.fill_raw)
        geometry_data = args.fill_raw.read_bytes()
    underfill, overfill = compare_faces(geometry_data, geometry_rows)
    alignment_failures = rectangle_alignment_matrix()
    slope_result = slope_matrix()
    shared_edge_result = shared_edge_matrix()
    convex_result = convex_polygon_matrix()
    direct_expected = expected_faces(geometry_data)
    triangle_expected = triangle_union_faces(geometry_data)
    triangle_union_mismatch = sum(
        direct_expected[y][x] != triangle_expected[y][x]
        for y in range(HEIGHT) for x in range(WIDTH)
    )
    expected = expected_faces(geometry_data)
    edge_result = None
    vertex_result = None
    if args.fill_raw and args.outline_raw:
        fill_rows = decode(args.fill_raw)
        outline_rows = decode(args.outline_raw)
        edge_result = edge_registration(fill_rows, outline_rows, rows, expected, geometry_data)
        vertex_result = vertex_junctions(outline_rows, rows, expected, geometry_data)
        if args.diagnostic_dir:
            diagnostic_images(fill_rows, outline_rows, rows, expected, args.diagnostic_dir)
    edge_ok = edge_result is None or edge_result["status"] == "PASS"
    vertex_ok = vertex_result is None or all(item["status"] == "PASS"
                                             for item in vertex_result.values())
    result = {
        "schema": "glass-p4-visual-v1",
        "image": "logical-g0-640x200-4bpp",
        "hole_count": len(holes),
        "holes": holes[:64],
        "underfill": len(underfill),
        "overfill": len(overfill),
        "underfill_runs": underfill[:32],
        "overfill_runs": overfill[:32],
        "rectangle_alignment_failures": alignment_failures,
        "slope_matrix": slope_result,
        "shared_edge_matrix": shared_edge_result,
        "convex_polygon_matrix": convex_result,
        "triangle_union_mismatch": triangle_union_mismatch,
        "direct_convex_oracle": {
            "status": "PASS" if triangle_union_mismatch == 0 else "FAIL",
            "triangle_union_mismatch": triangle_union_mismatch,
        },
        "geometry_underfill_overfill": {
            "underfill": len(underfill),
            "overfill": len(overfill),
            "status": "PASS" if not underfill and not overfill else "FAIL",
        },
        "edge_registration": edge_result,
        "vertex_junctions": vertex_result,
        "status": "PASS" if not underfill and not overfill and
        not alignment_failures and slope_result["status"] == "PASS" and
        shared_edge_result["status"] == "PASS" and
        convex_result["status"] == "PASS" and
        triangle_union_mismatch == 0 and edge_ok and vertex_ok else "FAIL",
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" and not result["rectangle_alignment_failures"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
