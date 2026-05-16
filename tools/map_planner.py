#!/usr/bin/env python3
"""Generate an interactive HTML map planner for editing map_object_config overrides.

Reads from the v33 map_* tables (raw map dump) and the v34 map_object_config
table (overrides). Output is one self-contained HTML file per map — open in
any browser, edit, export incremental SQL ready to drop into a migration.

Usage:
  python3 tools/map_planner.py --map 1P_CPLab03
  python3 tools/map_planner.py --map 1P_CPLab03 --db server.db --out tools/maps

What the page gives you:
  - top-down SVG map with one dot per actor, color-coded by class
  - sidebar split into:
    - column filter (substring, "|" for OR — e.g. "task|group")
    - searchable object list (every actor on the map)
    - per-object property editor showing the filtered columns
  - per-cell override input with static / variant pool mode toggle
  - "Export SQL" emits incremental INSERT / UPDATE / DELETE on
    map_object_config — no wipe-and-reinsert, so you can come back
    and tweak without exploding the migration history.
"""
import argparse
import json
import os
import sqlite3
import sys

# Columns we DON'T expose as user-overrideable: PK / map-key / locations.
# Common-but-overrideable columns (tag, group) go through OVERRIDABLE_COMMON
# below so they show up once per object regardless of class.
HIDDEN_COMMON_COLS = {
    'id', 'map_name', 'class_name',
    'location_x', 'location_y', 'location_z',
    'rotation_pitch', 'rotation_yaw', 'rotation_roll',
    # Array tables — handled separately, not class scalars.
    'array_index', 'nav_point_tag',
    # PK on map_<class>
    'map_object_id',
}
OVERRIDABLE_COMMON = ['tag', 'group']

# Visual config per class. r = radius (or half-side for non-circles), shape:
#   circle | square | diamond | star
CLASS_STYLE = {
    'TgBotFactory':              {'fill': '#e74c3c', 'r': 7,  'shape': 'circle',  'label': 'Bot factory'},
    'TgBotFactorySpawnable':     {'fill': '#c0392b', 'r': 6,  'shape': 'circle',  'label': 'Bot factory (spawnable)'},
    'TgBeaconFactory':           {'fill': '#27ae60', 'r': 7,  'shape': 'square',  'label': 'Beacon factory'},
    'TgDeployableFactory':       {'fill': '#9b59b6', 'r': 6,  'shape': 'square',  'label': 'Deployable factory'},
    'TgDestructibleFactory':     {'fill': '#7d3c98', 'r': 5,  'shape': 'square',  'label': 'Destructible factory'},
    'TgHexItemFactory':          {'fill': '#af7ac5', 'r': 5,  'shape': 'square',  'label': 'Hex item factory'},
    'TgMissionObjective_Bot':         {'fill': '#f1c40f', 'r': 10, 'shape': 'star', 'label': 'Mission obj (bot)'},
    'TgMissionObjective_Kismet':      {'fill': '#f39c12', 'r': 9,  'shape': 'star', 'label': 'Mission obj (kismet)'},
    'TgMissionObjective_Proximity':   {'fill': '#e67e22', 'r': 9,  'shape': 'star', 'label': 'Mission obj (proximity)'},
    'TgMissionObjective_Escort':      {'fill': '#d35400', 'r': 9,  'shape': 'star', 'label': 'Mission obj (escort)'},
    'TgBaseObjective_CTFBot':         {'fill': '#e67e22', 'r': 10, 'shape': 'star', 'label': 'Base obj (CTF)'},
    'TgBaseObjective_KOTH':           {'fill': '#e67e22', 'r': 10, 'shape': 'star', 'label': 'Base obj (KOTH)'},
    'TgTeamPlayerStart':         {'fill': '#3498db', 'r': 6,  'shape': 'diamond', 'label': 'Player start (team)'},
    'TgStartPoint':              {'fill': '#5dade2', 'r': 5,  'shape': 'diamond', 'label': 'Start point'},
    'TgBotStart':                {'fill': '#7f8c8d', 'r': 3,  'shape': 'circle',  'label': 'Bot start'},
    'TgActionPoint':             {'fill': '#16a085', 'r': 4,  'shape': 'diamond', 'label': 'Action point'},
    'TgAlarmPoint':              {'fill': '#c0392b', 'r': 4,  'shape': 'diamond', 'label': 'Alarm point'},
    'TgCoverPoint':              {'fill': '#34495e', 'r': 4,  'shape': 'diamond', 'label': 'Cover point'},
    'TgDefensePoint':            {'fill': '#2c3e50', 'r': 4,  'shape': 'diamond', 'label': 'Defense point'},
    'TgHoldSpot':                {'fill': '#1a252f', 'r': 4,  'shape': 'diamond', 'label': 'Hold spot'},
    'TgNavigationPoint':         {'fill': '#95a5a6', 'r': 3,  'shape': 'circle',  'label': 'Nav point'},
    'TgNavigationPointSpawnable':{'fill': '#bdc3c7', 'r': 3,  'shape': 'circle',  'label': 'Nav point (spawnable)'},
    'TgPointOfInterest':         {'fill': '#f1c40f', 'r': 5,  'shape': 'diamond', 'label': 'Point of interest'},
    'TgOmegaVolume':             {'fill': '#8e44ad', 'r': 6,  'shape': 'diamond', 'label': 'Omega volume'},
    'TgModifyPawnPropertiesVolume':   {'fill': '#9b59b6', 'r': 5, 'shape': 'diamond', 'label': 'Pawn-mod volume'},
    'TgDeviceVolume':            {'fill': '#7d3c98', 'r': 5,  'shape': 'diamond', 'label': 'Device volume'},
    'TgFlagCaptureVolume':       {'fill': '#16a085', 'r': 6,  'shape': 'diamond', 'label': 'Flag capture volume'},
    'TgHelpAlertVolume':         {'fill': '#1abc9c', 'r': 5,  'shape': 'diamond', 'label': 'Help alert volume'},
    'TgMissionListVolume':       {'fill': '#48c9b0', 'r': 5,  'shape': 'diamond', 'label': 'Mission list volume'},
    'TgTeleporter':              {'fill': '#2980b9', 'r': 6,  'shape': 'diamond', 'label': 'Teleporter'},
    'TgActorFactory':            {'fill': '#c0392b', 'r': 5,  'shape': 'circle',  'label': 'Actor factory (base)'},
}
DEFAULT_STYLE = {'fill': '#666', 'r': 4, 'shape': 'circle', 'label': '(other)'}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--map', required=True, help='map_name to render (e.g. 1P_CPLab03)')
    ap.add_argument('--db', default='server.db', help='path to server.db')
    ap.add_argument('--out', default='tools/maps', help='output directory')
    args = ap.parse_args()

    if not os.path.exists(args.db):
        print(f"DB not found: {args.db}", file=sys.stderr)
        sys.exit(1)

    conn = sqlite3.connect(args.db)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()

    schema = discover_map_schema(cur)
    if not schema:
        print("No map_* tables found — has migration v33 run?", file=sys.stderr)
        sys.exit(1)

    objects = load_objects(cur, args.map, schema)
    if not objects:
        print(f"No map_* rows for map '{args.map}'", file=sys.stderr)
        sys.exit(1)

    overrides = load_overrides(cur, list(objects.keys()))
    location_lists = load_location_lists(cur, args.map)

    conn.close()

    for mid, lists in location_lists.items():
        if mid in objects:
            objects[mid]['locations']   = lists.get('location_list', [])
            objects[mid]['patrol_path'] = lists.get('patrol_path', [])

    objects_list = list(objects.values())
    xs = [o['location_x'] for o in objects_list]
    ys = [o['location_y'] for o in objects_list]
    for o in objects_list:
        for p in o.get('locations', []):
            xs.append(p['x']); ys.append(p['y'])
        for p in o.get('patrol_path', []):
            xs.append(p['x']); ys.append(p['y'])
    bounds = {'min_x': min(xs), 'max_x': max(xs), 'min_y': min(ys), 'max_y': max(ys)}

    # Build a column directory: for each leaf class, list every column the
    # editor should expose (union of own-scalars across the object's chain +
    # the OVERRIDABLE_COMMON list). The chain is whichever map_* tables had
    # a row for any object of that class — derived during load_objects.
    column_directory = build_column_directory(objects_list, schema)

    payload = {
        'map_name':         args.map,
        'objects':          objects_list,
        'overrides':        overrides,
        'class_style':      CLASS_STYLE,
        'default_style':    DEFAULT_STYLE,
        'bounds':           bounds,
        'column_directory': column_directory,
    }

    os.makedirs(args.out, exist_ok=True)
    out_path = os.path.join(args.out, f'{args.map}.html')
    with open(out_path, 'w') as f:
        f.write(render_html(payload))

    abs_out = os.path.abspath(out_path)
    print(f"Wrote {out_path}")
    print(f"Open: file://{abs_out}")


def discover_map_schema(cur):
    """Return {table_name: [{name, type}]} for every map_<class> table (no
    array tables — their schemas differ and they're handled separately)."""
    cur.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'map_%' "
        "AND name NOT LIKE '%_location_list' AND name NOT LIKE '%_patrol_path' "
        "AND name != 'map_object_config' "
        "ORDER BY name"
    )
    tables = [r[0] for r in cur.fetchall()]
    schema = {}
    for t in tables:
        cur.execute(f"PRAGMA table_info({t})")
        schema[t] = [{'name': r[1], 'type': r[2]} for r in cur.fetchall()]
    return schema


def load_objects(cur, map_name, schema):
    """For every map_object_id with rows in any map_<class> table on this map,
    build one merged record: { id, cls, tag, group, location_*, rotation_*,
    tables: [...], values: {col_name: value} }."""
    objects = {}
    for table, cols in schema.items():
        cur.execute(f"SELECT * FROM {table} WHERE map_name = ?", (map_name,))
        for row in cur.fetchall():
            mid = row['map_object_id']
            if mid not in objects:
                objects[mid] = {
                    'id':             mid,
                    'cls':            row['class_name'],
                    'tag':            row['tag'] or '',
                    'group':          row['group'] or '',
                    'location_x':     row['location_x'] or 0.0,
                    'location_y':     row['location_y'] or 0.0,
                    'location_z':     row['location_z'] or 0.0,
                    'rotation_pitch': row['rotation_pitch'] or 0,
                    'rotation_yaw':   row['rotation_yaw']   or 0,
                    'rotation_roll':  row['rotation_roll']  or 0,
                    'tables':         [],
                    'values':         {},
                }
            obj = objects[mid]
            if table not in obj['tables']:
                obj['tables'].append(table)
            for c in cols:
                if c['name'] in HIDDEN_COMMON_COLS:
                    continue
                if c['name'] in OVERRIDABLE_COMMON:
                    # Common columns: only take from the leaf class's row so
                    # we don't overwrite with parent-table duplicates.
                    if c['name'] not in obj['values']:
                        v = row[c['name']]
                        obj['values'][c['name']] = '' if v is None else v
                    continue
                v = row[c['name']]
                obj['values'][c['name']] = '' if v is None else v
    return objects


def build_column_directory(objects_list, schema):
    """For each class present on this map, list which columns the editor
    should expose (and which table each comes from).  The browser uses this
    to decide what columns to render for a given selected object.

    Output: { class_name: [ {name, type, table} ] }  (ordered)
    """
    cls_to_tables = {}  # cls -> ordered list of table names this class derives from
    for o in objects_list:
        cls_to_tables.setdefault(o['cls'], list(o['tables']))

    directory = {}
    for cls, tables in cls_to_tables.items():
        seen = set()
        cols = []
        # OVERRIDABLE_COMMON first — they appear on every map_<class> table
        # so anchor them to the first table the object derives from.
        anchor = tables[0] if tables else None
        for cname in OVERRIDABLE_COMMON:
            if cname in seen:
                continue
            cols.append({'name': cname, 'type': 'TEXT', 'table': anchor})
            seen.add(cname)
        for t in tables:
            for c in schema.get(t, []):
                if c['name'] in HIDDEN_COMMON_COLS:
                    continue
                if c['name'] in seen:
                    continue
                cols.append({'name': c['name'], 'type': c['type'], 'table': t})
                seen.add(c['name'])
        directory[cls] = cols
    return directory


def load_overrides(cur, map_object_ids):
    """All map_object_config rows for the given map_object_ids. Empty list if
    the table doesn't exist yet (v34 not applied) — page still works, just
    starts with no overrides loaded."""
    if not map_object_ids:
        return []
    qmarks = ','.join('?' for _ in map_object_ids)
    try:
        cur.execute(f"""
            SELECT id, map_object_id, column_name, value,
                   variant_group, variant_id, weight
            FROM map_object_config
            WHERE map_object_id IN ({qmarks})
            ORDER BY map_object_id, column_name, variant_group, variant_id, id
        """, list(map_object_ids))
        return [dict(r) for r in cur.fetchall()]
    except sqlite3.OperationalError:
        return []


def load_location_lists(cur, map_name):
    """LocationList + PatrolPath for every TgBotFactory on this map."""
    out = {}
    for kind, table in [('location_list', 'map_tg_bot_factory_location_list'),
                        ('patrol_path',   'map_tg_bot_factory_patrol_path')]:
        try:
            cur.execute(f"""
                SELECT map_object_id, array_index, location_x, location_y, location_z, nav_point_tag
                FROM {table} WHERE map_name = ?
                ORDER BY map_object_id, array_index
            """, (map_name,))
            for r in cur.fetchall():
                mid = r['map_object_id']
                out.setdefault(mid, {'location_list': [], 'patrol_path': []})
                out[mid][kind].append({
                    'idx': r['array_index'],
                    'x':   r['location_x'],
                    'y':   r['location_y'],
                    'z':   r['location_z'],
                    'tag': r['nav_point_tag'] or '',
                })
        except sqlite3.OperationalError:
            # Migration v33 not applied yet — skip silently, page still works.
            pass
    return out


def render_html(payload):
    data_json = json.dumps(payload, ensure_ascii=False, default=str)
    return HTML_TEMPLATE.replace('__DATA_JSON__', data_json)


# Single-file HTML template. All CSS + JS inline so the artifact is portable.
HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Map Planner</title>
<style>
  * { box-sizing: border-box; }
  html, body { margin: 0; height: 100%; font: 13px/1.4 -apple-system, "Segoe UI", sans-serif; color: #222; background: #f4f4f4; }
  body { display: grid; grid-template-rows: auto 1fr; grid-template-columns: 1fr 520px; grid-template-areas: "topbar topbar" "map sidebar"; height: 100vh; }
  #topbar { grid-area: topbar; padding: 8px 14px; background: #1f2933; color: #fff; display: flex; align-items: center; gap: 16px; }
  #topbar h1 { font-size: 14px; margin: 0; font-weight: 600; }
  #topbar .stat { font-size: 12px; opacity: 0.85; }
  #topbar .pending { font-size: 12px; color: #f1c40f; margin-left: 12px; }
  #topbar button { margin-left: auto; background: #3498db; color: #fff; border: 0; padding: 6px 14px; border-radius: 3px; cursor: pointer; font-size: 12px; }
  #topbar button:hover { background: #2980b9; }
  #map { grid-area: map; position: relative; overflow: hidden; background: #fff; border-right: 1px solid #d0d6db; }
  #map svg { width: 100%; height: 100%; cursor: grab; display: block; }
  #map svg.dragging { cursor: grabbing; }
  .dot { transition: opacity .1s; }
  .dot.dim { opacity: 0.25; }
  /* Apply hover scale to the inner shape only — the text label stays its
     original size so it doesn't crowd neighboring markers when scaled. */
  .dot > circle, .dot > rect, .dot > polygon {
    transform-box: fill-box;
    transform-origin: center;
    transition: transform .08s ease-out, stroke-width .08s ease-out;
  }
  .dot.hi > circle, .dot.hi > rect, .dot.hi > polygon {
    transform: scale(2);
    stroke: #000;
    stroke-width: 2.5;
  }
  .dot.has-overrides { filter: drop-shadow(0 0 4px #f1c40f); }
  .dot-label { font-size: 9px; fill: #444; pointer-events: none; }
  .loc { pointer-events: none; opacity: 0.18; transition: opacity .1s; }
  .loc.hi { opacity: 1; }
  .loc-line { stroke: #888; stroke-width: 0.5; stroke-dasharray: 2 2; opacity: 0; pointer-events: none; transition: opacity .1s; }
  .loc-line.hi { opacity: 0.7; stroke-width: 1; }
  #legend { position: absolute; top: 10px; left: 10px; background: rgba(255,255,255,.92); border: 1px solid #ccc; padding: 8px 10px; border-radius: 3px; font-size: 11px; max-height: 70vh; overflow-y: auto; }
  #legend .row { display: flex; align-items: center; gap: 6px; margin: 2px 0; }
  #legend .swatch { width: 12px; height: 12px; display: inline-block; border: 1px solid #333; }
  #tooltip { position: absolute; background: rgba(0,0,0,.85); color: #fff; padding: 6px 8px; border-radius: 3px; font-size: 11px; pointer-events: none; display: none; z-index: 10; max-width: 320px; }

  #sidebar { grid-area: sidebar; display: flex; flex-direction: column; background: #fff; height: 100%; overflow: hidden; }
  .pane-hdr { padding: 6px 10px; background: #eef1f4; border-bottom: 1px solid #d0d6db; font-size: 11px; font-weight: 600; color: #2c3e50; text-transform: uppercase; letter-spacing: .04em; }

  .col-filter { padding: 8px 10px; background: #fafafa; border-bottom: 1px solid #ddd; }
  .col-filter label { display: block; font-size: 11px; color: #555; margin-bottom: 4px; }
  .col-filter input { width: 100%; padding: 6px 9px; font-size: 12px; border: 1px solid #ccc; border-radius: 3px; font-family: ui-monospace, "SF Mono", Menlo, monospace; }
  .col-filter .hint { font-size: 10px; color: #888; margin-top: 4px; }

  #list-pane { flex: 0 0 38%; min-height: 160px; display: flex; flex-direction: column; border-bottom: 2px solid #d0d6db; }
  #list-filter { padding: 6px 10px; background: #fff; border-bottom: 1px solid #eee; display: flex; gap: 6px; }
  #list-filter input { flex: 1; padding: 4px 7px; font-size: 12px; border: 1px solid #ccc; border-radius: 3px; }
  #list-filter select { padding: 3px; font-size: 11px; border: 1px solid #ccc; border-radius: 3px; max-width: 130px; }
  #list { flex: 1; overflow-y: auto; }
  .item { padding: 5px 10px; border-bottom: 1px solid #eee; cursor: pointer; display: flex; gap: 7px; align-items: center; }
  .item:hover { background: #f0f6ff; }
  .item.selected { background: #d6e9fb; }
  .item .badge { width: 10px; height: 10px; flex-shrink: 0; border: 1px solid #333; }
  .item .meta { flex: 1; min-width: 0; }
  .item .id { font-weight: 600; font-size: 12px; }
  .item .id .cls { font-weight: 400; color: #666; margin-left: 4px; font-size: 11px; }
  .item .grp { font-size: 10px; color: #555; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
  .item .ov-dots { font-size: 11px; color: #d35400; letter-spacing: -1px; }
  .item .ov-dots.match { color: #16a085; font-weight: 700; }

  #editor-pane { flex: 1; min-height: 200px; display: flex; flex-direction: column; }
  #editor-hdr { padding: 8px 10px; background: #fafafa; border-bottom: 1px solid #ddd; }
  #editor-hdr h3 { margin: 0 0 2px; font-size: 13px; }
  #editor-hdr .sub { font-size: 11px; color: #777; }
  #editor-body { flex: 1; overflow-y: auto; padding: 6px 0; }
  #editor-body.empty { display: flex; align-items: center; justify-content: center; color: #999; font-style: italic; }

  .prop { padding: 7px 10px; border-bottom: 1px solid #f0f0f0; }
  .prop.has-override { background: #fffbea; border-left: 3px solid #f1c40f; }
  .prop .name { font-weight: 600; font-size: 12px; color: #2c3e50; font-family: ui-monospace, "SF Mono", Menlo, monospace; }
  .prop .table-hint { font-size: 9px; color: #999; margin-left: 6px; font-weight: 400; }
  .prop .curr { font-size: 11px; color: #777; margin: 2px 0; }
  .prop .mode-row { display: flex; align-items: center; gap: 10px; font-size: 11px; margin: 4px 0; }
  .prop .mode-row label { display: inline-flex; align-items: center; gap: 3px; cursor: pointer; }
  .prop .static-input { width: 100%; padding: 4px 7px; font-size: 12px; border: 1px solid #ccc; border-radius: 3px; font-family: ui-monospace, "SF Mono", Menlo, monospace; }
  .prop .variant-box { background: #fff; border: 1px solid #ddd; border-radius: 3px; padding: 6px; margin-top: 4px; }
  .prop .vgroup-row { display: flex; gap: 6px; align-items: center; margin-bottom: 6px; font-size: 11px; }
  .prop .vgroup-row input { flex: 1; padding: 3px 6px; font-size: 11px; border: 1px solid #ccc; border-radius: 2px; font-family: ui-monospace, monospace; }
  .prop .variant-row { display: grid; grid-template-columns: 70px 1fr 50px 22px; gap: 4px; margin-top: 4px; font-size: 11px; align-items: center; }
  .prop .variant-row input { padding: 3px 5px; font-size: 11px; border: 1px solid #ccc; border-radius: 2px; font-family: ui-monospace, monospace; min-width: 0; }
  .prop .variant-row .v-del { background: #e74c3c; color: #fff; border: 0; border-radius: 2px; cursor: pointer; font-size: 11px; padding: 0; height: 22px; }
  .prop .variant-row .v-del:hover { background: #c0392b; }
  .prop .v-add { margin-top: 4px; background: #3498db; color: #fff; border: 0; padding: 3px 8px; border-radius: 2px; cursor: pointer; font-size: 11px; }
  .prop .v-add:hover { background: #2980b9; }
  .prop .var-hdr { font-weight: 600; font-size: 10px; color: #555; display: grid; grid-template-columns: 70px 1fr 50px 22px; gap: 4px; padding: 0 1px; }

  #sql-modal { position: fixed; inset: 0; background: rgba(0,0,0,.5); display: none; align-items: center; justify-content: center; z-index: 100; }
  #sql-modal.open { display: flex; }
  #sql-modal .box { background: #fff; width: 880px; max-width: 92vw; max-height: 88vh; display: flex; flex-direction: column; border-radius: 4px; padding: 14px; }
  #sql-modal h2 { margin: 0 0 8px; font-size: 14px; }
  #sql-modal .summary { font-size: 11px; color: #555; margin-bottom: 8px; }
  #sql-modal textarea { flex: 1; font-family: ui-monospace, "SF Mono", Menlo, monospace; font-size: 11px; border: 1px solid #ccc; padding: 8px; resize: none; min-height: 380px; }
  #sql-modal .row { margin-top: 8px; display: flex; gap: 8px; }
  #sql-modal button { padding: 5px 14px; border: 0; border-radius: 3px; cursor: pointer; font-size: 12px; }
  #sql-modal .close { background: #95a5a6; color: #fff; }
  #sql-modal .copy  { background: #3498db; color: #fff; margin-left: auto; }
</style>
</head>
<body>
<div id="topbar">
  <h1 id="map-title">map</h1>
  <span class="stat" id="counts"></span>
  <span class="pending" id="pending"></span>
  <button id="sql-btn">Export SQL …</button>
</div>
<div id="map">
  <svg id="svg"></svg>
  <div id="legend"></div>
  <div id="tooltip"></div>
</div>
<div id="sidebar">
  <div class="pane-hdr">Column filter</div>
  <div class="col-filter">
    <input id="col-filter" placeholder="substring, | for OR  (e.g. task|spawn|group)">
    <div class="hint" id="col-filter-hint">type to filter the property editor below</div>
  </div>

  <div id="list-pane">
    <div class="pane-hdr" id="list-hdr">Objects</div>
    <div id="list-filter">
      <input id="obj-filter" placeholder="filter id / group / tag">
      <select id="cls-filter"><option value="">all classes</option></select>
    </div>
    <div id="list"></div>
  </div>

  <div id="editor-pane">
    <div class="pane-hdr">Properties</div>
    <div id="editor-hdr">
      <h3 id="editor-title">— pick an object —</h3>
      <div class="sub" id="editor-sub"></div>
    </div>
    <div id="editor-body" class="empty">no selection</div>
  </div>
</div>

<div id="sql-modal"><div class="box">
  <h2>Generated SQL</h2>
  <div class="summary" id="sql-summary"></div>
  <textarea id="sql-text" readonly></textarea>
  <div class="row">
    <button class="close" id="sql-close">Close</button>
    <button class="copy"  id="sql-copy">Copy to clipboard</button>
  </div>
</div></div>

<script>
const DATA = __DATA_JSON__;
const OBJECTS = DATA.objects;
const STYLE = DATA.class_style;
const DEFAULT_STYLE = DATA.default_style;
const COL_DIR = DATA.column_directory;   // class -> ordered list of {name, type, table}

// ============================================================
// State
// ============================================================
// ORIGINAL: snapshot of map_object_config rows on load (immutable). Map<id, row>.
// CURRENT:  live edit state. Map<key, row>, where key is either the DB id (for
//           pre-existing rows) or a synthesised "new:N" string for newly-added
//           rows. Both share the same shape:
//               { id?:int, key:str, map_object_id, column_name, value,
//                 variant_group, variant_id, weight }
// We diff CURRENT vs ORIGINAL on export to produce incremental SQL.
const ORIGINAL = new Map();
const CURRENT  = new Map();
let nextNewKey = 1;

for (const o of DATA.overrides) {
  const row = { ...o, key: String(o.id) };
  ORIGINAL.set(row.key, Object.freeze({ ...row }));
  CURRENT.set(row.key, { ...row });
}

let selectedId  = null;
let columnFilter = '';

// ============================================================
// State helpers
// ============================================================
function rowsForObject(mid) {
  const out = [];
  for (const r of CURRENT.values()) if (r.map_object_id === mid) out.push(r);
  return out;
}
function rowsForCell(mid, column) {
  return rowsForObject(mid).filter(r => r.column_name === column);
}
function overrideCountForObject(mid) {
  const cols = new Set();
  for (const r of CURRENT.values()) {
    if (r.map_object_id === mid) cols.add(r.column_name);
  }
  return cols.size;
}
function overrideCountForObjectMatchingFilter(mid) {
  const cols = filteredColumnsForObject(mid).map(c => c.name);
  const set = new Set(cols);
  let n = 0;
  const seen = new Set();
  for (const r of CURRENT.values()) {
    if (r.map_object_id !== mid) continue;
    if (!set.has(r.column_name)) continue;
    if (seen.has(r.column_name)) continue;
    seen.add(r.column_name);
    n++;
  }
  return n;
}
function newKey() { return 'new:' + (nextNewKey++); }

function addRow(mid, column, value, variantGroup, variantId, weight) {
  const k = newKey();
  const row = { key: k, map_object_id: mid, column_name: column,
                value: String(value), variant_group: variantGroup,
                variant_id: variantId, weight: weight ?? 1.0 };
  CURRENT.set(k, row);
  return row;
}
function removeRow(key) { CURRENT.delete(key); }

// ============================================================
// Column filter (substring, "|" = OR)
// ============================================================
function parseFilter(s) {
  return (s || '').toLowerCase().split('|').map(x => x.trim()).filter(Boolean);
}
function columnMatchesFilter(colName, terms) {
  if (terms.length === 0) return true;
  const n = colName.toLowerCase();
  for (const t of terms) if (n.includes(t)) return true;
  return false;
}
function filteredColumnsForObject(mid) {
  const o = OBJECTS.find(x => x.id === mid);
  if (!o) return [];
  const dir = COL_DIR[o.cls] || [];
  const terms = parseFilter(columnFilter);
  return dir.filter(c => columnMatchesFilter(c.name, terms));
}

// ============================================================
// SVG map (kept close to the previous planner — pan/zoom/dots/grid)
// ============================================================
const SVG_PAD = 60;
const SVG_W = 1600, SVG_H = 1200;
const svg = document.getElementById('svg');
const tooltip = document.getElementById('tooltip');
const mapDiv  = document.getElementById('map');
const bounds = DATA.bounds;
const wx = bounds.max_x - bounds.min_x || 1;
const wy = bounds.max_y - bounds.min_y || 1;
const scale = Math.min((SVG_W - 2 * SVG_PAD) / wy, (SVG_H - 2 * SVG_PAD) / wx);
function worldToSvg(x, y) {
  return {
    sx: SVG_PAD + (y - bounds.min_y) * scale,
    sy: SVG_PAD + (bounds.max_x - x) * scale,
  };
}
svg.setAttribute('viewBox', `0 0 ${SVG_W} ${SVG_H}`);
svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');

// Grid
{
  const grid = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  const stepWorld = pickGridStep(Math.max(wx, wy));
  const xStart = Math.ceil(bounds.min_x / stepWorld) * stepWorld;
  const yStart = Math.ceil(bounds.min_y / stepWorld) * stepWorld;
  for (let xw = xStart; xw <= bounds.max_x; xw += stepWorld) {
    const p1 = worldToSvg(xw, bounds.min_y), p2 = worldToSvg(xw, bounds.max_y);
    grid.appendChild(makeSvg('line', { x1: p1.sx, y1: p1.sy, x2: p2.sx, y2: p2.sy, stroke: '#eee', 'stroke-width': 1 }));
  }
  for (let yw = yStart; yw <= bounds.max_y; yw += stepWorld) {
    const p1 = worldToSvg(bounds.min_x, yw), p2 = worldToSvg(bounds.max_x, yw);
    grid.appendChild(makeSvg('line', { x1: p1.sx, y1: p1.sy, x2: p2.sx, y2: p2.sy, stroke: '#eee', 'stroke-width': 1 }));
  }
  svg.appendChild(grid);
}

const linesLayer = document.createElementNS('http://www.w3.org/2000/svg', 'g');
const locsLayer  = document.createElementNS('http://www.w3.org/2000/svg', 'g');
const dotsLayer  = document.createElementNS('http://www.w3.org/2000/svg', 'g');
svg.appendChild(linesLayer);
svg.appendChild(locsLayer);
svg.appendChild(dotsLayer);

function pickGridStep(span) {
  const target = span / 12;
  const pow = Math.pow(10, Math.floor(Math.log10(target)));
  for (const m of [1, 2, 5, 10]) if (m * pow >= target) return m * pow;
  return pow * 10;
}
function makeSvg(tag, attrs) {
  const e = document.createElementNS('http://www.w3.org/2000/svg', tag);
  for (const k in attrs) e.setAttribute(k, attrs[k]);
  return e;
}
function styleFor(cls) { return STYLE[cls] || DEFAULT_STYLE; }
function shapeFor(cls, sx, sy, r) {
  const st = styleFor(cls);
  const fill = st.fill;
  switch (st.shape) {
    case 'square':
      return makeSvg('rect', { x: sx - r, y: sy - r, width: 2 * r, height: 2 * r, fill, stroke: '#222', 'stroke-width': 1 });
    case 'diamond': {
      const pts = `${sx},${sy - r} ${sx + r},${sy} ${sx},${sy + r} ${sx - r},${sy}`;
      return makeSvg('polygon', { points: pts, fill, stroke: '#222', 'stroke-width': 1 });
    }
    case 'star': {
      const pts = starPoints(sx, sy, r, r * 0.45, 5);
      return makeSvg('polygon', { points: pts, fill, stroke: '#222', 'stroke-width': 1 });
    }
    default:
      return makeSvg('circle', { cx: sx, cy: sy, r, fill, stroke: '#222', 'stroke-width': 1 });
  }
}
function starPoints(cx, cy, ro, ri, n) {
  let out = [];
  for (let i = 0; i < 2 * n; i++) {
    const r = i % 2 ? ri : ro;
    const a = (Math.PI / n) * i - Math.PI / 2;
    out.push((cx + r * Math.cos(a)).toFixed(2) + ',' + (cy + r * Math.sin(a)).toFixed(2));
  }
  return out.join(' ');
}

const dotsById = {};
for (const o of OBJECTS) {
  const st = styleFor(o.cls);
  const p = worldToSvg(o.location_x, o.location_y);
  const g = makeSvg('g', { 'class': 'dot', 'data-id': o.id });
  g.appendChild(shapeFor(o.cls, p.sx, p.sy, st.r));
  // Every marker gets a map_object_id label so you can always identify what
  // you're hovering over without having to read the tooltip.
  {
    const txt = makeSvg('text', { x: p.sx + st.r + 2, y: p.sy + 3, 'class': 'dot-label' });
    txt.textContent = o.id;
    g.appendChild(txt);
  }
  g.addEventListener('mouseenter', () => onHover(o.id, true));
  g.addEventListener('mouseleave', () => onHover(o.id, false));
  g.addEventListener('mousemove',  (e) => moveTooltip(e, o));
  g.addEventListener('click',      () => selectObject(o.id));
  dotsLayer.appendChild(g);
  dotsById[o.id] = g;

  // Spawn point cloud for factories
  if (o.locations && o.locations.length) {
    for (const lp of o.locations) {
      const lpv = worldToSvg(lp.x, lp.y);
      linesLayer.appendChild(makeSvg('line', {
        x1: p.sx, y1: p.sy, x2: lpv.sx, y2: lpv.sy,
        'class': 'loc-line', 'data-parent-id': o.id,
      }));
      const arm = 5;
      const cross = makeSvg('g', { 'class': 'loc', 'data-parent-id': o.id });
      cross.appendChild(makeSvg('line', { x1: lpv.sx - arm, y1: lpv.sy - arm, x2: lpv.sx + arm, y2: lpv.sy + arm, stroke: st.fill, 'stroke-width': 2 }));
      cross.appendChild(makeSvg('line', { x1: lpv.sx - arm, y1: lpv.sy + arm, x2: lpv.sx + arm, y2: lpv.sy - arm, stroke: st.fill, 'stroke-width': 2 }));
      locsLayer.appendChild(cross);
    }
  }
}

// Pan + zoom
let viewBox = { x: 0, y: 0, w: SVG_W, h: SVG_H };
function applyView() { svg.setAttribute('viewBox', `${viewBox.x} ${viewBox.y} ${viewBox.w} ${viewBox.h}`); }
mapDiv.addEventListener('wheel', (e) => {
  e.preventDefault();
  const rect = svg.getBoundingClientRect();
  const mx = ((e.clientX - rect.left) / rect.width)  * viewBox.w + viewBox.x;
  const my = ((e.clientY - rect.top)  / rect.height) * viewBox.h + viewBox.y;
  const f  = Math.pow(1.0015, e.deltaY);
  viewBox.x = mx - (mx - viewBox.x) * f;
  viewBox.y = my - (my - viewBox.y) * f;
  viewBox.w *= f; viewBox.h *= f;
  applyView();
}, { passive: false });
let drag = null;
svg.addEventListener('mousedown', (e) => {
  drag = { x: e.clientX, y: e.clientY, vx: viewBox.x, vy: viewBox.y };
  svg.classList.add('dragging');
});
window.addEventListener('mouseup', () => { drag = null; svg.classList.remove('dragging'); });
window.addEventListener('mousemove', (e) => {
  if (!drag) return;
  const rect = svg.getBoundingClientRect();
  const dx = (e.clientX - drag.x) * viewBox.w / rect.width;
  const dy = (e.clientY - drag.y) * viewBox.h / rect.height;
  viewBox.x = drag.vx - dx;
  viewBox.y = drag.vy - dy;
  applyView();
});

function moveTooltip(e, o) {
  tooltip.style.display = 'block';
  tooltip.style.left = (e.clientX - mapDiv.getBoundingClientRect().left + 10) + 'px';
  tooltip.style.top  = (e.clientY - mapDiv.getBoundingClientRect().top  + 10) + 'px';
  let html = `<b>${o.cls} #${o.id}</b>`;
  if (o.group) html += `<br>group: ${escapeHtml(o.group)}`;
  if (o.tag)   html += `<br>tag: ${escapeHtml(o.tag)}`;
  html += `<br>x=${o.location_x.toFixed(0)}  y=${o.location_y.toFixed(0)}  z=${o.location_z.toFixed(0)}`;
  const oc = overrideCountForObject(o.id);
  if (oc > 0) html += `<br><b>${oc}</b> override${oc === 1 ? '' : 's'}`;
  if (o.locations && o.locations.length) html += `<br>LocationList: <b>${o.locations.length}</b>`;
  tooltip.innerHTML = html;
}
function escapeHtml(s) { return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c])); }
function onHover(id, on) {
  const dot = dotsById[id];
  if (dot) {
    dot.classList.toggle('hi', on);
    // Bring the hovered marker to the top of the paint order so it isn't
    // hidden behind overlapping neighbours. SVG paints in document order;
    // appendChild moves the node to the end of its parent.
    if (on) dotsLayer.appendChild(dot);
  }
  const item = document.querySelector(`.item[data-id="${id}"]`);
  if (item && id !== selectedId) item.classList.toggle('hover', on);
  document.querySelectorAll(`.loc[data-parent-id="${id}"]`).forEach(el => el.classList.toggle('hi', on));
  document.querySelectorAll(`.loc-line[data-parent-id="${id}"]`).forEach(el => el.classList.toggle('hi', on));
  if (!on) tooltip.style.display = 'none';
}

// Refresh override halo on dots whenever CURRENT changes
function refreshDotHalos() {
  for (const o of OBJECTS) {
    const dot = dotsById[o.id];
    if (!dot) continue;
    dot.classList.toggle('has-overrides', overrideCountForObject(o.id) > 0);
  }
}

// ============================================================
// Object list
// ============================================================
const listEl = document.getElementById('list');
const colFilterEl = document.getElementById('col-filter');
const objFilterEl = document.getElementById('obj-filter');
const clsFilterEl = document.getElementById('cls-filter');
colFilterEl.addEventListener('input', () => {
  columnFilter = colFilterEl.value;
  renderList();
  if (selectedId !== null) renderEditor(selectedId);
});
objFilterEl.addEventListener('input', renderList);
clsFilterEl.addEventListener('change', renderList);

// Populate class filter
{
  const classes = new Set(OBJECTS.map(o => o.cls));
  for (const c of [...classes].sort()) {
    const opt = document.createElement('option');
    opt.value = c; opt.textContent = c.replace('Tg', '');
    clsFilterEl.appendChild(opt);
  }
}

function renderList() {
  const q = objFilterEl.value.trim().toLowerCase();
  const clsq = clsFilterEl.value;
  listEl.innerHTML = '';
  let n = 0;
  for (const o of OBJECTS) {
    if (clsq && o.cls !== clsq) continue;
    if (q) {
      const hay = `${o.id} ${o.cls} ${o.group} ${o.tag}`.toLowerCase();
      if (!hay.includes(q)) continue;
    }
    listEl.appendChild(itemEl(o));
    n++;
  }
  document.getElementById('list-hdr').textContent = `Objects (${n}/${OBJECTS.length})`;
  refreshCounts();
  refreshDotHalos();
}
function itemEl(o) {
  const d = document.createElement('div');
  d.className = 'item' + (o.id === selectedId ? ' selected' : '');
  d.dataset.id = o.id;
  const st = styleFor(o.cls);
  const badge = `<span class="badge" style="background:${st.fill}"></span>`;
  const ovCount = overrideCountForObject(o.id);
  const ovMatchCount = overrideCountForObjectMatchingFilter(o.id);
  let dots = '';
  if (ovCount > 0) {
    const matchHtml = '●'.repeat(ovMatchCount);
    const restHtml  = '●'.repeat(ovCount - ovMatchCount);
    dots = `<span class="ov-dots match">${matchHtml}</span><span class="ov-dots">${restHtml}</span>`;
  }
  d.innerHTML = `
    ${badge}
    <div class="meta">
      <div class="id">#${o.id}<span class="cls">${o.cls.replace('Tg', '')}</span></div>
      <div class="grp">${escapeHtml(o.group || '(no group)')}</div>
    </div>
    ${dots}
  `;
  d.addEventListener('mouseenter', () => onHover(o.id, true));
  d.addEventListener('mouseleave', () => onHover(o.id, false));
  d.addEventListener('click', () => selectObject(o.id));
  return d;
}

function refreshCounts() {
  const total = OBJECTS.length;
  const withOverrides = OBJECTS.filter(o => overrideCountForObject(o.id) > 0).length;
  const totalRows = CURRENT.size;
  const origRows = ORIGINAL.size;
  const newRows = [...CURRENT.values()].filter(r => !ORIGINAL.has(r.key)).length;
  const removedRows = [...ORIGINAL.keys()].filter(k => !CURRENT.has(k)).length;
  const changedRows = [...CURRENT.values()].filter(r => {
    if (!ORIGINAL.has(r.key)) return false;
    return !rowsEqual(r, ORIGINAL.get(r.key));
  }).length;
  document.getElementById('counts').textContent =
    `${total} objects · ${withOverrides} with overrides · ${totalRows} config rows (orig ${origRows})`;
  const pending = newRows + changedRows + removedRows;
  document.getElementById('pending').textContent = pending > 0
    ? `pending: +${newRows} new, ~${changedRows} changed, −${removedRows} deleted`
    : '';
}

function rowsEqual(a, b) {
  return a.map_object_id === b.map_object_id
      && a.column_name   === b.column_name
      && String(a.value) === String(b.value)
      && (a.variant_group ?? null) === (b.variant_group ?? null)
      && (a.variant_id    ?? null) === (b.variant_id    ?? null)
      && Number(a.weight) === Number(b.weight);
}

// ============================================================
// Property editor (per-cell static / variant toggle)
// ============================================================
const editorBody = document.getElementById('editor-body');
const editorTitle = document.getElementById('editor-title');
const editorSub   = document.getElementById('editor-sub');

function selectObject(id) {
  selectedId = id;
  document.querySelectorAll('.item').forEach(el => el.classList.toggle('selected', Number(el.dataset.id) === id));
  renderEditor(id);
}

function renderEditor(mid) {
  const o = OBJECTS.find(x => x.id === mid);
  if (!o) {
    editorTitle.textContent = '— pick an object —';
    editorSub.textContent = '';
    editorBody.classList.add('empty');
    editorBody.textContent = 'no selection';
    return;
  }
  editorTitle.textContent = `#${o.id}  ${o.cls.replace('Tg', '')}`;
  editorSub.textContent = `tag=${o.tag || '(none)'}  ·  group=${o.group || '(none)'}  ·  tables: ${o.tables.length}`;
  editorBody.classList.remove('empty');
  editorBody.innerHTML = '';

  const cols = filteredColumnsForObject(mid);
  if (cols.length === 0) {
    editorBody.classList.add('empty');
    editorBody.textContent = parseFilter(columnFilter).length === 0
      ? '(no columns — odd)' : 'no columns match the filter';
    return;
  }
  for (const c of cols) {
    editorBody.appendChild(renderProp(o, c));
  }
}

function renderProp(o, col) {
  const wrap = document.createElement('div');
  wrap.className = 'prop';
  const cellRows = rowsForCell(o.id, col.name);
  const hasOverride = cellRows.length > 0;
  if (hasOverride) wrap.classList.add('has-override');

  const currentVal = (col.name in o.values) ? String(o.values[col.name]) : (col.name === 'group' ? o.group : col.name === 'tag' ? o.tag : '');
  wrap.innerHTML = `
    <div class="name">${col.name}<span class="table-hint">${col.table || ''}</span></div>
    <div class="curr">current: <code>${escapeHtml(currentVal === '' ? '(empty)' : currentVal)}</code></div>
    <div class="mode-row">
      <label><input type="radio" name="mode-${o.id}-${col.name}" value="static"> static</label>
      <label><input type="radio" name="mode-${o.id}-${col.name}" value="variant"> variants</label>
      <label><input type="radio" name="mode-${o.id}-${col.name}" value="none"> no override</label>
    </div>
    <div class="body"></div>
  `;
  const body = wrap.querySelector('.body');
  const radios = wrap.querySelectorAll(`input[name="mode-${o.id}-${col.name}"]`);

  // Decide initial mode
  let initialMode;
  if (cellRows.length === 0)                              initialMode = 'none';
  else if (cellRows.length === 1 && cellRows[0].variant_group == null) initialMode = 'static';
  else                                                    initialMode = 'variant';
  radios.forEach(r => { if (r.value === initialMode) r.checked = true; });
  radios.forEach(r => r.addEventListener('change', () => switchMode(o, col, r.value, body)));
  drawBodyFor(o, col, initialMode, body);
  return wrap;
}

function drawBodyFor(o, col, mode, body) {
  body.innerHTML = '';
  if (mode === 'none') return;
  if (mode === 'static') {
    let row = rowsForCell(o.id, col.name).find(r => r.variant_group == null);
    if (!row) row = addRow(o.id, col.name, '', null, null, 1.0);
    const inp = document.createElement('input');
    inp.className = 'static-input';
    inp.value = row.value;
    inp.placeholder = 'override value';
    inp.addEventListener('input', () => { row.value = inp.value; queueRefresh(); });
    body.appendChild(inp);
  } else if (mode === 'variant') {
    let rows = rowsForCell(o.id, col.name).filter(r => r.variant_group != null);
    if (rows.length === 0) {
      // Promote any existing static value into the first variant, else seed empty.
      const stat = rowsForCell(o.id, col.name).find(r => r.variant_group == null);
      const seedValue = stat ? stat.value : '';
      if (stat) removeRow(stat.key);
      addRow(o.id, col.name, seedValue, 'pool', 'v1', 1.0);
      rows = rowsForCell(o.id, col.name).filter(r => r.variant_group != null);
    }
    renderVariantBlock(o, col, rows, body);
  }
  queueRefresh();
}

function switchMode(o, col, newMode, body) {
  const cellRows = rowsForCell(o.id, col.name);
  if (newMode === 'none') {
    for (const r of cellRows) removeRow(r.key);
  } else if (newMode === 'static') {
    // Keep one value: the first variant's value if we were in variant mode.
    let seed = '';
    if (cellRows.length > 0) seed = cellRows[0].value;
    for (const r of cellRows) removeRow(r.key);
    addRow(o.id, col.name, seed, null, null, 1.0);
  } else if (newMode === 'variant') {
    // If static existed, promote it into v1 of a new pool. Otherwise add v1 blank.
    let seed = '';
    const stat = cellRows.find(r => r.variant_group == null);
    if (stat) { seed = stat.value; removeRow(stat.key); }
    if (rowsForCell(o.id, col.name).filter(r => r.variant_group != null).length === 0) {
      addRow(o.id, col.name, seed, 'pool', 'v1', 1.0);
    }
  }
  drawBodyFor(o, col, newMode, body);
}

function renderVariantBlock(o, col, rows, body) {
  const box = document.createElement('div');
  box.className = 'variant-box';
  // All variant rows in a cell share variant_group. We pick the first row's
  // group as the canonical name; editing this input rewrites it on every row
  // so the SQL produced is internally consistent.
  const group = rows[0].variant_group;
  const grpRow = document.createElement('div');
  grpRow.className = 'vgroup-row';
  grpRow.innerHTML = `<label style="font-size:11px;color:#555">variant_group:</label> <input value="${escapeHtml(group)}" placeholder="pool">`;
  const grpInp = grpRow.querySelector('input');
  grpInp.addEventListener('input', () => {
    for (const r of rowsForCell(o.id, col.name)) {
      r.variant_group = grpInp.value || 'pool';
    }
    queueRefresh();
  });
  box.appendChild(grpRow);

  const hdr = document.createElement('div');
  hdr.className = 'var-hdr';
  hdr.innerHTML = '<div>variant_id</div><div>value</div><div>weight</div><div></div>';
  box.appendChild(hdr);

  function paintVariantRows() {
    // Wipe and re-paint variant rows so add/remove stays in sync.
    [...box.querySelectorAll('.variant-row')].forEach(el => el.remove());
    const fresh = rowsForCell(o.id, col.name).filter(r => r.variant_group != null);
    const addBtn = box.querySelector('.v-add');
    for (const r of fresh) {
      const vr = document.createElement('div');
      vr.className = 'variant-row';
      vr.innerHTML = `
        <input class="v-id"     value="${escapeHtml(r.variant_id || '')}">
        <input class="v-value"  value="${escapeHtml(r.value)}">
        <input class="v-weight" type="number" step="0.1" min="0" value="${r.weight}">
        <button class="v-del" title="remove this variant">✕</button>
      `;
      vr.querySelector('.v-id').addEventListener('input', (e) => { r.variant_id = e.target.value; queueRefresh(); });
      vr.querySelector('.v-value').addEventListener('input', (e) => { r.value = e.target.value; queueRefresh(); });
      vr.querySelector('.v-weight').addEventListener('input', (e) => { r.weight = Number(e.target.value) || 0; queueRefresh(); });
      vr.querySelector('.v-del').addEventListener('click', () => {
        removeRow(r.key);
        if (rowsForCell(o.id, col.name).filter(rr => rr.variant_group != null).length === 0) {
          // No variants left — pop back into 'none' mode.
          const radio = document.querySelector(`input[name="mode-${o.id}-${col.name}"][value="none"]`);
          if (radio) { radio.checked = true; radio.dispatchEvent(new Event('change')); }
        } else {
          paintVariantRows();
        }
        queueRefresh();
      });
      if (addBtn) box.insertBefore(vr, addBtn);
      else        box.appendChild(vr);
    }
  }

  const addBtn = document.createElement('button');
  addBtn.className = 'v-add';
  addBtn.textContent = '+ add variant';
  addBtn.addEventListener('click', () => {
    // Auto-generate a fresh variant_id ("v1", "v2", ...) that doesn't collide
    // with existing IDs in this cell.
    const existing = new Set(rowsForCell(o.id, col.name).map(r => r.variant_id));
    let n = 1; while (existing.has(`v${n}`)) n++;
    addRow(o.id, col.name, '', rows[0].variant_group || 'pool', `v${n}`, 1.0);
    paintVariantRows();
    queueRefresh();
  });
  box.appendChild(addBtn);

  paintVariantRows();
  body.appendChild(box);
}

// Debounce minor counts/list refreshes — every keystroke shouldn't repaint
// the whole list, but the counts/halo do want to stay live.
let refreshTimer = null;
function queueRefresh() {
  if (refreshTimer) return;
  refreshTimer = setTimeout(() => {
    refreshTimer = null;
    refreshCounts();
    refreshDotHalos();
    // Also re-stamp dots in the list (override-dot indicator only — don't
    // rebuild the whole list since that would steal input focus).
    document.querySelectorAll('.item').forEach(el => {
      const id = Number(el.dataset.id);
      const ovCount = overrideCountForObject(id);
      const ovMatch = overrideCountForObjectMatchingFilter(id);
      const wrap = el.querySelector('.meta').nextSibling;  // text node maybe — re-render dots span
      [...el.querySelectorAll('.ov-dots')].forEach(s => s.remove());
      if (ovCount > 0) {
        const m = document.createElement('span'); m.className = 'ov-dots match'; m.textContent = '●'.repeat(ovMatch);
        const r = document.createElement('span'); r.className = 'ov-dots';       r.textContent = '●'.repeat(ovCount - ovMatch);
        el.appendChild(m); el.appendChild(r);
      }
    });
  }, 80);
}

// ============================================================
// SQL export — diff CURRENT vs ORIGINAL
// ============================================================
const sqlBtn   = document.getElementById('sql-btn');
const sqlModal = document.getElementById('sql-modal');
const sqlText  = document.getElementById('sql-text');
const sqlSum   = document.getElementById('sql-summary');
sqlBtn.addEventListener('click', () => {
  const { sql, summary } = generateSql();
  sqlText.value = sql;
  sqlSum.textContent = summary;
  sqlModal.classList.add('open');
});
document.getElementById('sql-close').addEventListener('click', () => sqlModal.classList.remove('open'));
document.getElementById('sql-copy').addEventListener('click', () => {
  sqlText.select();
  document.execCommand('copy');
});

function sqlQuote(s) { return `'${String(s).replace(/'/g, "''")}'`; }
function sqlNull(v)  { return (v == null || v === '') ? 'NULL' : sqlQuote(v); }

function generateSql() {
  const inserts = [];
  const updates = [];
  const deletes = [];

  for (const r of CURRENT.values()) {
    const orig = ORIGINAL.get(r.key);
    if (!orig) {
      // New row
      inserts.push(`  (${r.map_object_id}, ${sqlQuote(r.column_name)}, ${sqlQuote(r.value)}, ` +
                   `${sqlNull(r.variant_group)}, ${sqlNull(r.variant_id)}, ${Number(r.weight) || 1.0})`);
    } else if (!rowsEqual(r, orig)) {
      updates.push(
        `UPDATE map_object_config SET ` +
        `value = ${sqlQuote(r.value)}, ` +
        `variant_group = ${sqlNull(r.variant_group)}, ` +
        `variant_id = ${sqlNull(r.variant_id)}, ` +
        `weight = ${Number(r.weight) || 1.0} ` +
        `WHERE id = ${r.id};`
      );
    }
  }
  for (const k of ORIGINAL.keys()) {
    if (!CURRENT.has(k)) deletes.push(`DELETE FROM map_object_config WHERE id = ${ORIGINAL.get(k).id};`);
  }

  const lines = [];
  lines.push(`-- map_planner output for ${DATA.map_name}`);
  lines.push(`-- ${new Date().toISOString()}`);
  if (deletes.length) {
    lines.push('');
    lines.push(`-- ${deletes.length} delete${deletes.length === 1 ? '' : 's'}`);
    for (const s of deletes) lines.push(s);
  }
  if (updates.length) {
    lines.push('');
    lines.push(`-- ${updates.length} update${updates.length === 1 ? '' : 's'}`);
    for (const s of updates) lines.push(s);
  }
  if (inserts.length) {
    lines.push('');
    lines.push(`-- ${inserts.length} new override${inserts.length === 1 ? '' : 's'}`);
    lines.push('INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES');
    lines.push(inserts.join(',\n') + ';');
  }
  if (deletes.length + updates.length + inserts.length === 0) {
    lines.push('');
    lines.push('-- (no changes — nothing edited)');
  }
  const summary = `${inserts.length} INSERT · ${updates.length} UPDATE · ${deletes.length} DELETE`;
  return { sql: lines.join('\n'), summary };
}

// ============================================================
// Legend
// ============================================================
{
  const el = document.getElementById('legend');
  // Only classes that are actually present on this map
  const present = new Set(OBJECTS.map(o => o.cls));
  for (const cls of Object.keys(STYLE)) {
    if (!present.has(cls)) continue;
    const st = STYLE[cls];
    el.innerHTML += `<div class="row"><span class="swatch" style="background:${st.fill}"></span>${st.label}</div>`;
  }
  el.innerHTML += `<div class="row"><span class="swatch" style="background:transparent;border:0;color:#e74c3c;font-weight:bold;text-align:center">×</span>Spawn point</div>`;
  el.innerHTML += `<div class="row"><span class="swatch" style="background:#fff;border:0;box-shadow:0 0 4px #f1c40f"></span>Has override(s)</div>`;
}

document.getElementById('map-title').textContent = DATA.map_name;
renderList();
applyView();
</script>
</body>
</html>
"""


if __name__ == '__main__':
    main()
