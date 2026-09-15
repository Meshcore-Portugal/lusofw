#!/usr/bin/env python3
"""
Generate SVG visualisations from region headers.

Outputs:
  test_districts.svg   — 29 PT districts
  test_macros.svg      — 7 PT macro regions
  test_europe.svg      — Europe coarse polygon
"""

import math
import re
import os

# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def parse_pt_regions(filepath):
    with open(filepath) as f:
        content = f.read()

    polys = {}
    for m in re.finditer(r'static const GeoPoint (poly_\w+)\[\] PROGMEM = \{(.*?)\};', content, re.DOTALL):
        name = m.group(1)
        pts = [(float(lat), float(lon))
               for lat, lon in re.findall(r'\{([0-9.-]+),\s*([0-9.-]+)\}', m.group(2))]
        polys[name] = pts

    rings = {}
    for m in re.finditer(r'static const RegionRing (rings_\w+)\[\] PROGMEM = \{(.*?)\};', content, re.DOTALL):
        rname = m.group(1)
        entries = re.findall(r'\{(poly_\w+),\s*\d+\}', m.group(2))
        rings[rname] = [polys[p] for p in entries]

    def parse_table(table_name):
        m = re.search(
            rf'static const RegionPolygon ({table_name})\[\] = \{{(.*?)\}};\s*static const int NUM_{table_name}',
            content, re.DOTALL)
        items = re.findall(r'\{"([^"]+)",\s*(rings_\w+),\s*(\d+)\}', m.group(2))
        return [(name, rings[rname]) for name, rname, _ in items]

    return parse_table('PT_DISTRICTS'), parse_table('PT_MACRO_REGIONS')


def parse_europe(filepath):
    with open(filepath) as f:
        content = f.read()

    pts = [(float(lat), float(lon))
           for lat, lon in re.findall(r'\{\s*([0-9.-]+)f?,\s*([0-9.-]+)f?\s*\}', content)]
    # Remove trailing comment-only lines and keep the polygon
    return pts


# ---------------------------------------------------------------------------
# Projeção
# ---------------------------------------------------------------------------

def merc(lat, lon):
    x = math.radians(lon)
    y = math.log(math.tan(math.pi / 4 + math.radians(lat) / 2))
    return x, y


def make_proj(lat_min, lat_max, lon_min, lon_max, canvas_w, canvas_h, pad=20):
    x0, _ = merc(lat_min, lon_min)
    x1, _ = merc(lat_min, lon_max)
    _, y0 = merc(lat_min, lon_min)
    _, y1 = merc(lat_max, lon_min)

    scale_x = (canvas_w - 2 * pad) / (x1 - x0)
    scale_y = (canvas_h - 2 * pad) / (y1 - y0)
    scale = min(scale_x, scale_y)

    # Centre
    merc_w = (x1 - x0) * scale
    merc_h = (y1 - y0) * scale
    off_x = pad + (canvas_w - 2 * pad - merc_w) / 2
    off_y = pad + (canvas_h - 2 * pad - merc_h) / 2

    def to_px(lat, lon):
        x, y = merc(lat, lon)
        px = off_x + (x - x0) * scale
        py = off_y + (y1 - y) * scale   # SVG y-down
        return px, py

    return to_px, scale


# ---------------------------------------------------------------------------
# Centroid
# ---------------------------------------------------------------------------

def centroid(ring):
    pts = ring[:-1] if len(ring) > 1 and ring[0] == ring[-1] else ring
    n = len(pts)
    if n < 3:
        return sum(p[0] for p in pts) / n, sum(p[1] for p in pts) / n
    A = Clat = Clon = 0.0
    for i in range(n):
        la0, lo0 = pts[i]
        la1, lo1 = pts[(i + 1) % n]
        c = lo0 * la1 - lo1 * la0
        A += c
        Clon += (lo0 + lo1) * c
        Clat += (la0 + la1) * c
    A *= 0.5
    if abs(A) < 1e-9:
        return sum(p[0] for p in pts) / n, sum(p[1] for p in pts) / n
    return Clat / (6 * A), Clon / (6 * A)


# ---------------------------------------------------------------------------
# Short label
# ---------------------------------------------------------------------------

def short(name):
    s = name.replace('#pt-', '').replace('#', '')
    for prefix in ('ilha-de-', 'ilha-da-', 'ilha-do-', 'ilha-das-', 'ilha-'):
        s = s.replace(prefix, '')
    return s


# ---------------------------------------------------------------------------
# Palette
# ---------------------------------------------------------------------------

DISTRICT_COLORS = [
    '#0077bb', '#ee7733', '#009988', '#cc3311', '#33bbee',
    '#ee3377', '#555555', '#004488', '#ddaa33', '#997700',
    '#6699cc', '#882255', '#44aa99', '#117733', '#999933',
    '#aa4499', '#88ccee', '#cc6677', '#4477aa', '#225555',
    '#661100', '#664400', '#aa4466', '#332288', '#888822',
    '#005577', '#bbaa44', '#771155', '#2a9d8f',
]

MACRO_COLORS = [
    '#2a78d6', '#eb6834', '#1baf7a', '#4a3aa7',
    '#e34948', '#eda100', '#008855',
]


# ---------------------------------------------------------------------------
# SVG builder
# ---------------------------------------------------------------------------

class SVGBuilder:
    def __init__(self, w, h, title, subtitle=''):
        self.w = w
        self.h = h
        self.lines = []
        self.lines.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">')
        self.lines.append('  <style>polygon:hover{fill-opacity:.85;stroke-width:2.2;cursor:pointer}circle:hover{r:4}</style>')
        self.lines.append(f'  <rect width="{w}" height="{h}" fill="#f8fafc"/>')
        # Header band
        self.lines.append(f'  <rect x="0" y="0" width="{w}" height="52" fill="#0f172a"/>')
        self.lines.append(f'  <text x="20" y="30" font-family="system-ui,sans-serif" font-size="20" font-weight="700" fill="#f8fafc">{title}</text>')
        if subtitle:
            self.lines.append(f'  <text x="20" y="47" font-family="system-ui,sans-serif" font-size="12" fill="#94a3b8">{subtitle}</text>')

    def add(self, s):
        self.lines.append(s)

    def graticule(self, to_px, lat_min, lat_max, lon_min, lon_max):
        self.add('  <g stroke="#dbeafe" stroke-width=".8" stroke-dasharray="4,3" font-family="monospace" font-size="10" fill="#94a3b8">')
        for ld in range(int(math.floor(lat_min)), int(math.ceil(lat_max)) + 1, 5):
            if lat_min <= ld <= lat_max:
                x0, y = to_px(ld, lon_min)
                x1, _ = to_px(ld, lon_max)
                self.add(f'    <line x1="{x0:.1f}" y1="{y:.1f}" x2="{x1:.1f}" y2="{y:.1f}"/>')
                self.add(f'    <text x="{x0+3:.1f}" y="{y-3:.1f}">{ld}°N</text>')
        for lo in range(int(math.floor(lon_min)), int(math.ceil(lon_max)) + 1, 10):
            if lon_min <= lo <= lon_max:
                x, y0 = to_px(lat_min, lo)
                _, y1 = to_px(lat_max, lo)
                self.add(f'    <line x1="{x:.1f}" y1="{y0:.1f}" x2="{x:.1f}" y2="{y1:.1f}"/>')
                label = f'{abs(lo)}°{"W" if lo < 0 else "E"}'
                self.add(f'    <text x="{x+3:.1f}" y="{y0-3:.1f}">{label}</text>')
        self.add('  </g>')

    def scale_bar(self, to_px, lat_ref, lon_ref, km=500):
        """Draw a horizontal scale bar representing km."""
        dlat = km / 111.0
        x0, y0 = to_px(lat_ref, lon_ref)
        x1, _  = to_px(lat_ref + dlat, lon_ref)   # reuse lat trick for bar length
        # Actually: compute bar length from lat span at reference latitude
        # 1 deg lat = 111 km always; use that for consistent bar
        _, ya = to_px(lat_ref, lon_ref)
        _, yb = to_px(lat_ref + dlat, lon_ref)
        bar_h = abs(ya - yb)   # bar_h in px = km at this scale
        bar_x = x0 + 10
        bar_y = y0 + 30
        self.add(f'  <g font-family="system-ui,sans-serif" font-size="10" fill="#334155">')
        self.add(f'    <rect x="{bar_x-5:.1f}" y="{bar_y-16}" width="{bar_h+70:.1f}" height="28" fill="white" fill-opacity=".88" stroke="#cbd5e1" rx="4"/>')
        self.add(f'    <line x1="{bar_x:.1f}" y1="{bar_y:.1f}" x2="{bar_x+bar_h:.1f}" y2="{bar_y:.1f}" stroke="#0f172a" stroke-width="3"/>')
        self.add(f'    <line x1="{bar_x:.1f}" y1="{bar_y-5:.1f}" x2="{bar_x:.1f}" y2="{bar_y+5:.1f}" stroke="#0f172a" stroke-width="2"/>')
        self.add(f'    <line x1="{bar_x+bar_h:.1f}" y1="{bar_y-5:.1f}" x2="{bar_x+bar_h:.1f}" y2="{bar_y+5:.1f}" stroke="#0f172a" stroke-width="2"/>')
        self.add(f'    <text x="{bar_x:.1f}" y="{bar_y-7:.1f}" text-anchor="middle">0</text>')
        self.add(f'    <text x="{bar_x+bar_h:.1f}" y="{bar_y-7:.1f}" text-anchor="middle">{km} km</text>')
        self.add(f'  </g>')

    def finish(self):
        self.lines.append('</svg>')
        return '\n'.join(self.lines)


# ---------------------------------------------------------------------------
# Draw one region set
# ---------------------------------------------------------------------------

def draw_regions(svg, regions, colors, to_px, dot_r=1.4, font_size=10):
    """Draw polygons, vertex dots, centroid labels."""
    # Polygons
    for idx, (name, reg_rings) in enumerate(regions):
        color = colors[idx % len(colors)]
        for r_idx, ring in enumerate(reg_rings):
            pts = [to_px(lat, lon) for lat, lon in ring]
            pts_str = ' '.join(f'{x:.1f},{y:.1f}' for x, y in pts)
            title = f'{name} ring {r_idx+1}/{len(reg_rings)} ({len(ring)} pts)'
            svg.add(f'  <polygon points="{pts_str}" fill="{color}" fill-opacity=".42" '
                    f'stroke="{color}" stroke-width="1.3" stroke-linejoin="round">'
                    f'<title>{title}</title></polygon>')

    # Vertex dots
    svg.add('  <g class="verts">')
    for idx, (name, reg_rings) in enumerate(regions):
        color = colors[idx % len(colors)]
        for ring in reg_rings:
            for lat, lon in ring:
                x, y = to_px(lat, lon)
                svg.add(f'    <circle cx="{x:.1f}" cy="{y:.1f}" r="{dot_r}" fill="{color}" stroke="#fff" stroke-width=".5"/>')
    svg.add('  </g>')

    # Centroid labels
    svg.add(f'  <g font-family="system-ui,sans-serif" font-size="{font_size}" font-weight="600" text-anchor="middle" dominant-baseline="middle">')
    for idx, (name, reg_rings) in enumerate(regions):
        # Pick largest ring
        largest = max(reg_rings, key=len)
        clat, clon = centroid(largest)
        cx, cy = to_px(clat, clon)
        label = short(name)
        svg.add(f'    <text x="{cx:.1f}" y="{cy:.1f}" fill="#0f172a" stroke="#fff" stroke-width="2.5" paint-order="stroke">{label}</text>')
    svg.add('  </g>')


# ---------------------------------------------------------------------------
# Legend
# ---------------------------------------------------------------------------

def draw_legend(svg, regions, colors, x, y, col_w=270, row_h=22):
    n = len(regions)
    cols = 2 if n > 8 else 1
    split = (n + cols - 1) // cols
    pad_x = 16
    pad_y = 16
    title_h = 24
    box_w = cols * col_w + 2 * pad_x
    box_h = split * row_h + pad_y + title_h + 12
    svg.add(f'  <rect x="{x}" y="{y}" width="{box_w}" height="{box_h}" fill="white" fill-opacity=".96" stroke="#cbd5e1" stroke-width="1.2" rx="6"/>')
    svg.add(f'  <text x="{x+pad_x}" y="{y+pad_y+10}" font-family="system-ui,sans-serif" font-size="13" font-weight="700" fill="#0f172a">Legenda ({n} regiões)</text>')
    for idx, (name, reg_rings) in enumerate(regions):
        color = colors[idx % len(colors)]
        pts_count = sum(len(r) for r in reg_rings)
        col = idx // split
        row = idx % split
        rx = x + pad_x + col * col_w
        ry = y + pad_y + title_h + 14 + row * row_h
        svg.add(f'  <rect x="{rx}" y="{ry-10}" width="12" height="12" rx="2" fill="{color}" stroke="#0f172a" stroke-width=".5"/>')
        svg.add(f'  <text x="{rx+18}" y="{ry}" font-family="system-ui,sans-serif" font-size="11" fill="#1e293b">'
                f'{name} <tspan fill="#64748b" font-size="9.5">({pts_count} pts)</tspan></text>')


# ---------------------------------------------------------------------------
# Inset panel
# ---------------------------------------------------------------------------

def draw_inset(svg, regions, colors, panel_x, panel_y, panel_w, panel_h,
               lat_min, lat_max, lon_min, lon_max, title, dot_r=2.2, font_size=9):
    svg.add(f'  <rect x="{panel_x}" y="{panel_y}" width="{panel_w}" height="{panel_h}" '
            f'fill="#f0f6fc" stroke="#94a3b8" stroke-width="1.4" rx="6"/>')
    svg.add(f'  <text x="{panel_x+10}" y="{panel_y+18}" font-family="system-ui,sans-serif" '
            f'font-size="11" font-weight="700" fill="#0f172a">{title}</text>')

    inner_x = panel_x + 8
    inner_y = panel_y + 24
    inner_w = panel_w - 16
    inner_h = panel_h - 30

    to_px, _ = make_proj(lat_min, lat_max, lon_min, lon_max, inner_w, inner_h, pad=8)

    def to_abs(lat, lon):
        px, py = to_px(lat, lon)
        return inner_x + px, inner_y + py

    # Ocean background already from parent rect; draw ring frame
    svg.add(f'  <rect x="{inner_x}" y="{inner_y}" width="{inner_w}" height="{inner_h}" fill="#dbeafe" fill-opacity=".4" rx="4"/>')

    for idx, (name, reg_rings) in enumerate(regions):
        color = colors[idx % len(colors)]
        for r_idx, ring in enumerate(reg_rings):
            in_box = any(lat_min <= p[0] <= lat_max and lon_min <= p[1] <= lon_max for p in ring)
            if not in_box:
                continue
            pts = [to_abs(lat, lon) for lat, lon in ring]
            pts_str = ' '.join(f'{x:.1f},{y:.1f}' for x, y in pts)
            svg.add(f'  <polygon points="{pts_str}" fill="{color}" fill-opacity=".55" '
                    f'stroke="{color}" stroke-width="1.4" stroke-linejoin="round"><title>{name}</title></polygon>')
            for x, y in pts:
                svg.add(f'  <circle cx="{x:.1f}" cy="{y:.1f}" r="{dot_r}" fill="{color}" stroke="#fff" stroke-width=".6"/>')
            # Label for largest ring
            if r_idx == 0 or len(ring) >= max(len(r) for r in reg_rings):
                clat, clon = centroid(ring)
                cx, cy = to_abs(clat, clon)
                label = short(name)
                svg.add(f'  <text x="{cx:.1f}" y="{cy-6:.1f}" font-family="system-ui,sans-serif" '
                        f'font-size="{font_size}" font-weight="600" text-anchor="middle" '
                        f'fill="#0f172a" stroke="#fff" stroke-width="2" paint-order="stroke">{label}</text>')


# ---------------------------------------------------------------------------
# test_districts.svg
# ---------------------------------------------------------------------------

def generate_districts(districts, out_path):
    W, H = 2800, 1560

    # Main map occupies the right 1900 px; left 900 px for insets + legend
    MAP_X = 860
    MAP_Y = 60
    MAP_W = 1900
    MAP_H = H - MAP_Y - 20

    lat_min, lat_max = 29.7, 42.6
    lon_min, lon_max = -31.7, -5.7

    to_px_abs, scale = make_proj(lat_min, lat_max, lon_min, lon_max, MAP_W, MAP_H, pad=24)

    def to_px(lat, lon):
        px, py = to_px_abs(lat, lon)
        return MAP_X + px, MAP_Y + py

    svg = SVGBuilder(W, H,
                     'Distritos de Portugal (29) — src/lusofw/regions/pt_regions.h',
                     'Projeção Mercator conforme, escala 1:1. Vértices assinalados para inspeção de simplificação e contiguidade.')

    # Ocean
    svg.add(f'  <rect x="{MAP_X}" y="{MAP_Y}" width="{MAP_W}" height="{MAP_H}" fill="#e0f2fe" stroke="#7dd3fc" stroke-width="1.5" rx="6"/>')

    svg.graticule(to_px, lat_min, lat_max, lon_min, lon_max)

    draw_regions(svg, districts, DISTRICT_COLORS, to_px, dot_r=1.4, font_size=10)

    # Geo tags
    cx, cy = to_px(39.0, -22.0)
    svg.add(f'  <text x="{cx:.0f}" y="{cy:.0f}" font-family="system-ui,sans-serif" font-size="16" font-weight="700" fill="#7dd3fc" letter-spacing="5">OCEANO ATLÂNTICO</text>')

    # Scale bar (place near bottom-left of map)
    _, sy = to_px(lat_min + 0.5, lon_min + 0.5)
    sx, _ = to_px(lat_min + 0.5, lon_min + 0.5)
    svg.scale_bar(to_px, lat_min + 2, lon_min + 1, km=200)

    # Legend (top-left panel)
    draw_legend(svg, districts, DISTRICT_COLORS, x=10, y=60, col_w=280, row_h=22)

    # Inset Açores
    draw_inset(svg, districts, DISTRICT_COLORS,
               panel_x=10, panel_y=660, panel_w=840, panel_h=380,
               lat_min=36.7, lat_max=40.0, lon_min=-31.5, lon_max=-24.8,
               title='Açores — Zoom 2.5× com vértices', dot_r=2.4, font_size=9)

    # Inset Madeira + Porto Santo
    draw_inset(svg, districts, DISTRICT_COLORS,
               panel_x=10, panel_y=1060, panel_w=560, panel_h=300,
               lat_min=32.3, lat_max=33.2, lon_min=-17.4, lon_max=-16.1,
               title='Madeira &amp; Porto Santo — Zoom 5×', dot_r=2.4, font_size=9)

    # Inset Selvagens
    draw_inset(svg, districts, DISTRICT_COLORS,
               panel_x=580, panel_y=1060, panel_w=270, panel_h=300,
               lat_min=30.12, lat_max=30.18, lon_min=-15.90, lon_max=-15.84,
               title='Selvagens (30°N) — Zoom 30×', dot_r=3.0, font_size=8)

    os.makedirs(os.path.dirname(out_path), exist_ok=True) if os.path.dirname(out_path) else None
    with open(out_path, 'w') as f:
        f.write(svg.finish())
    print(f'  {out_path}  ({os.path.getsize(out_path)} bytes)')


# ---------------------------------------------------------------------------
# test_macros.svg
# ---------------------------------------------------------------------------

def generate_macros(macros, out_path):
    W, H = 2800, 1560

    MAP_X = 560
    MAP_Y = 60
    MAP_W = 2220
    MAP_H = H - MAP_Y - 20

    lat_min, lat_max = 29.7, 42.6
    lon_min, lon_max = -31.7, -5.7

    to_px_abs, _ = make_proj(lat_min, lat_max, lon_min, lon_max, MAP_W, MAP_H, pad=24)

    def to_px(lat, lon):
        px, py = to_px_abs(lat, lon)
        return MAP_X + px, MAP_Y + py

    svg = SVGBuilder(W, H,
                     'Macro Regiões de Portugal (7) — src/lusofw/regions/pt_regions.h',
                     'Dissolução gap-free de municípios CAOP 2024. Norte, Centro, Lisboa VdT, Alentejo, Algarve, Açores, Madeira.')

    svg.add(f'  <rect x="{MAP_X}" y="{MAP_Y}" width="{MAP_W}" height="{MAP_H}" fill="#e0f2fe" stroke="#7dd3fc" stroke-width="1.5" rx="6"/>')
    svg.graticule(to_px, lat_min, lat_max, lon_min, lon_max)
    draw_regions(svg, macros, MACRO_COLORS, to_px, dot_r=1.6, font_size=14)

    cx, cy = to_px(39.0, -22.0)
    svg.add(f'  <text x="{cx:.0f}" y="{cy:.0f}" font-family="system-ui,sans-serif" font-size="16" font-weight="700" fill="#7dd3fc" letter-spacing="5">OCEANO ATLÂNTICO</text>')

    svg.scale_bar(to_px, lat_min + 2, lon_min + 1, km=200)
    draw_legend(svg, macros, MACRO_COLORS, x=10, y=60, col_w=260, row_h=24)

    with open(out_path, 'w') as f:
        f.write(svg.finish())
    print(f'  {out_path}  ({os.path.getsize(out_path)} bytes)')


# ---------------------------------------------------------------------------
# test_europe.svg
# ---------------------------------------------------------------------------

def generate_europe(eu_pts, out_path):
    W, H = 1800, 1200

    MAP_X = 20
    MAP_Y = 60
    MAP_W = 1300
    MAP_H = H - MAP_Y - 60

    # Europe bounding box (with Azores + Canaries)
    lat_min, lat_max = 24.0, 80.0
    lon_min, lon_max = -40.0, 70.0

    to_px_abs, _ = make_proj(lat_min, lat_max, lon_min, lon_max, MAP_W, MAP_H, pad=24)

    def to_px(lat, lon):
        px, py = to_px_abs(lat, lon)
        return MAP_X + px, MAP_Y + py

    svg = SVGBuilder(W, H,
                     'Europe coarse polygon — src/lusofw/regions/eu_regions.h',
                     '14 vértices. Define o limite EU para aplicação da directiva de dever de ciclo e potência de transmissão.')

    # Background
    svg.add(f'  <rect x="{MAP_X}" y="{MAP_Y}" width="{MAP_W}" height="{MAP_H}" fill="#bfdbfe" stroke="#7dd3fc" stroke-width="1.5" rx="6"/>')
    svg.graticule(to_px, lat_min, lat_max, lon_min, lon_max)

    # Draw europe polygon
    pts_px = [to_px(lat, lon) for lat, lon in eu_pts]
    pts_str = ' '.join(f'{x:.1f},{y:.1f}' for x, y in pts_px)
    svg.add(f'  <polygon points="{pts_str}" fill="#2563eb" fill-opacity=".18" '
            f'stroke="#1d4ed8" stroke-width="2" stroke-linejoin="round">'
            f'<title>#europe ({len(eu_pts)} pts)</title></polygon>')

    # Vertex dots (numbered only — coordinates live in the side table)
    svg.add('  <g font-family="monospace" font-size="11" text-anchor="middle" dominant-baseline="middle">')
    for i, (lat, lon) in enumerate(eu_pts):
        x, y = to_px(lat, lon)
        fill = '#94a3b8' if i == len(eu_pts) - 1 else '#1d4ed8'
        svg.add(f'    <circle cx="{x:.1f}" cy="{y:.1f}" r="8" fill="{fill}" stroke="#fff" stroke-width="1.5"/>')
        svg.add(f'    <text x="{x:.1f}" y="{y:.1f}" fill="#fff" font-weight="700">{i}</text>')
    svg.add('  </g>')

    # Key geographic markers
    markers = [
        (40.41, -3.70, 'Madrid'),
        (48.85, 2.35, 'Paris'),
        (51.50, -0.12, 'London'),
        (55.75, 37.62, 'Moscow'),
        (28.29, -16.62, 'Tenerife'),
        (37.74, -25.67, 'Ponta Delgada'),
        (38.71, -9.14, 'Lisboa'),
        (31.63, -8.00, 'Casablanca (fora)'),
        (30.43, -9.60, 'Agadir (fora)'),
        (35.89, -5.35, 'Ceuta (dentro)'),
        (35.79, -5.85, 'Tánger (fora)'),
    ]
    svg.add('  <g font-family="system-ui,sans-serif" font-size="11">')
    for lat, lon, label in markers:
        if lat_min <= lat <= lat_max and lon_min <= lon <= lon_max:
            x, y = to_px(lat, lon)
            color = '#dc2626' if 'fora' in label else '#15803d'
            svg.add(f'    <circle cx="{x:.1f}" cy="{y:.1f}" r="3.5" fill="{color}" stroke="#fff" stroke-width="1.2"/>')
            svg.add(f'    <text x="{x+7:.1f}" y="{y+4:.1f}" fill="{color}" font-weight="600">{label}</text>')
    svg.add('  </g>')

    # Scale bar
    svg.scale_bar(to_px, 35.0, -38.0, km=1000)

    # --- Coordinate table (right panel) ---
    TBL_X = MAP_X + MAP_W + 30
    TBL_Y = MAP_Y
    TBL_W = W - TBL_X - 20
    row_h = 26
    TBL_H = row_h * len(eu_pts) + 50

    svg.add(f'  <rect x="{TBL_X}" y="{TBL_Y}" width="{TBL_W}" height="{TBL_H}" fill="white" fill-opacity=".94" stroke="#cbd5e1" rx="6"/>')
    svg.add(f'  <text x="{TBL_X+12}" y="{TBL_Y+22}" font-family="system-ui,sans-serif" font-size="13" font-weight="700" fill="#0f172a">Vértices de #europe</text>')
    svg.add(f'  <text x="{TBL_X+12}" y="{TBL_Y+38}" font-family="system-ui,sans-serif" font-size="10" fill="#64748b">O último vértice repete o primeiro (fecho do anel)</text>')

    # Table header
    hy = TBL_Y + 58
    svg.add(f'  <text x="{TBL_X+16}" y="{hy}" font-family="monospace" font-size="11" font-weight="700" fill="#334155">#</text>')
    svg.add(f'  <text x="{TBL_X+40}" y="{hy}" font-family="monospace" font-size="11" font-weight="700" fill="#334155">lat</text>')
    svg.add(f'  <text x="{TBL_X+120}" y="{hy}" font-family="monospace" font-size="11" font-weight="700" fill="#334155">lon</text>')
    svg.add(f'  <text x="{TBL_X+200}" y="{hy}" font-family="system-ui,sans-serif" font-size="11" font-weight="700" fill="#334155">Descrição</text>')

    descriptions = [
        'NW (Islândia/Groenlândia)',
        'NE (montes Urais)',
        'SE (mar Cáspio)',
        'Este Med (Síria/Turquia)',
        'Abaixo de Chipre',
        'Abaixo de Creta/Grécia',
        'Sicília–Tunísia',
        'Oeste Med (Melilla dentro)',
        'Estreito de Gibraltar (Ceuta)',
        'Saída do Estreito (Tánger fora)',
        'Atlântico (Casablanca fora)',
        'Lanzarote–Sahara Ocidental',
        'Abaixo das Canárias',
        'SW (Açores e Madeira)',
        'Fecho (repete o vértice 0)',
    ]

    for i, (lat, lon) in enumerate(eu_pts):
        ry = hy + 22 + i * row_h
        color = '#94a3b8' if i == len(eu_pts) - 1 else '#1d4ed8'
        svg.add(f'  <circle cx="{TBL_X+20:.1f}" cy="{ry-3:.1f}" r="7" fill="{color}"/>')
        svg.add(f'  <text x="{TBL_X+20:.1f}" y="{ry-3:.1f}" font-family="monospace" font-size="10" font-weight="700" fill="#fff" text-anchor="middle" dominant-baseline="middle">{i}</text>')
        svg.add(f'  <text x="{TBL_X+40}" y="{ry}" font-family="monospace" font-size="11" fill="#0f172a">{lat:6.2f}f</text>')
        svg.add(f'  <text x="{TBL_X+120}" y="{ry}" font-family="monospace" font-size="11" fill="#0f172a">{lon:6.2f}f</text>')
        svg.add(f'  <text x="{TBL_X+200}" y="{ry}" font-family="system-ui,sans-serif" font-size="10" fill="#334155">{descriptions[i] if i < len(descriptions) else ""}</text>')

    # Bounding-box info at bottom
    svg.add(f'  <text x="{MAP_X+10}" y="{MAP_Y+MAP_H+30}" font-family="monospace" font-size="12" fill="#334155">'
            f'Caixa delimitadora: lat {lat_min}°–{lat_max}°N, lon {lon_min}°–{lon_max}°E/W  |  '
            f'Atenção: vértice 0 = vértice de fecho (repetido)</text>')

    with open(out_path, 'w') as f:
        f.write(svg.finish())
    print(f'  {out_path}  ({os.path.getsize(out_path)} bytes)')


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    BASE = '/workspaces/lusofw'
    PT_HDR = f'{BASE}/src/lusofw/regions/pt_regions.h'
    EU_HDR = f'{BASE}/src/lusofw/regions/eu_regions.h'

    districts, macros = parse_pt_regions(PT_HDR)
    eu_pts = parse_europe(EU_HDR)

    print('Generating SVGs:')
    generate_districts(districts, f'{BASE}/test_districts.svg')
    generate_macros(macros, f'{BASE}/test_macros.svg')
    generate_europe(eu_pts, f'{BASE}/test_europe.svg')
    print('Done.')

if __name__ == '__main__':
    main()
