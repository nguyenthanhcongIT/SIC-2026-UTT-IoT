from pathlib import Path
import json
import math
from threading import RLock
import networkx as nx
import osmnx as ox
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

ROOT = Path(__file__).resolve().parent
GRAPH_PATH = ROOT / 'roads.graphml'
if not GRAPH_PATH.exists():
    raise RuntimeError('Thiếu roads.graphml. Hãy chạy: python download_map.py')
G = ox.io.load_graphml(GRAPH_PATH)
area = json.loads((ROOT / 'area.json').read_text(encoding='utf-8'))
FLOOD_FILE = ROOT / 'flooded.json'
lock = RLock()
# Key: edge id, value: set of directed (u,v,k) belonging to the physical segment
flooded_ids = set(json.loads(FLOOD_FILE.read_text()) if FLOOD_FILE.exists() else [])
app = FastAPI(title='UTT Flood Routing API')
app.add_middleware(CORSMiddleware, allow_origins=['http://localhost:5173', 'http://127.0.0.1:5173'], allow_methods=['*'], allow_headers=['*'])

class RouteRequest(BaseModel):
    start: list[float]  # [lat, lon]
    end: list[float]

class FloodRequest(BaseModel):
    id: str


def coords_of(u, v, data):
    geom = data.get('geometry')
    if geom is None:
        return [[float(G.nodes[u]['y']), float(G.nodes[u]['x'])], [float(G.nodes[v]['y']), float(G.nodes[v]['x'])]]
    pts = [[float(y), float(x)] for x, y in geom.coords]
    start = [float(G.nodes[u]['y']), float(G.nodes[u]['x'])]
    if math.dist(pts[0], start) > math.dist(pts[-1], start):
        pts.reverse()
    return pts


def edge_id(u, v, data):
    # Two opposite directed edges representing the same geometry share one ID.
    # Parallel roads remain distinct by their geometry and OSM id.
    pts = coords_of(u, v, data)
    canonical = min(tuple(tuple(p) for p in pts), tuple(tuple(p) for p in reversed(pts)))
    osm_id = data.get('osmid', '')
    if isinstance(osm_id, list):
        osm_id = sorted(str(x) for x in osm_id)
    return json.dumps([min(u, v), max(u, v), osm_id, canonical], separators=(',', ':'))

EDGES = []
EDGE_BY_ID = {}
for u, v, k, data in G.edges(keys=True, data=True):
    eid = edge_id(u, v, data)
    record = {'id': eid, 'u': u, 'v': v, 'key': k, 'coords': coords_of(u, v, data), 'length': float(data.get('length', 0)), 'name': str(data.get('name', 'Đường không tên'))}
    EDGES.append(record)
    EDGE_BY_ID.setdefault(eid, []).append((u, v, k))

@app.get('/api/map')
def get_map():
    with lock:
        ids = flooded_ids.copy()
    return {'area': area, 'bounds': [[min(float(d['y']) for _, d in G.nodes(data=True)), min(float(d['x']) for _, d in G.nodes(data=True))], [max(float(d['y']) for _, d in G.nodes(data=True)), max(float(d['x']) for _, d in G.nodes(data=True))]], 'roads': [{**e, 'flooded': e['id'] in ids} for e in EDGES]}

@app.post('/api/flood')
def toggle_flood(req: FloodRequest):
    if req.id not in EDGE_BY_ID:
        raise HTTPException(404, 'Không tìm thấy đoạn đường')
    with lock:
        if req.id in flooded_ids:
            flooded_ids.remove(req.id)
        else:
            flooded_ids.add(req.id)
        FLOOD_FILE.write_text(json.dumps(sorted(flooded_ids)), encoding='utf-8')
        return {'id': req.id, 'flooded': req.id in flooded_ids}

@app.post('/api/reset')
def reset():
    with lock:
        flooded_ids.clear()
        FLOOD_FILE.write_text('[]', encoding='utf-8')
    return {'ok': True}

@app.post('/api/route')
def route(req: RouteRequest):
    for p in (req.start, req.end):
        if len(p) != 2 or not all(math.isfinite(x) for x in p):
            raise HTTPException(400, 'Tọa độ không hợp lệ')
    with lock:
        blocked = flooded_ids.copy()
    H = G.copy()
    for eid in blocked:
        H.remove_edges_from(EDGE_BY_ID.get(eid, []))
    try:
        start = ox.distance.nearest_nodes(G, X=req.start[1], Y=req.start[0])
        end = ox.distance.nearest_nodes(G, X=req.end[1], Y=req.end[0])
        nodes = nx.shortest_path(H, start, end, weight='length')
    except (nx.NetworkXNoPath, nx.NodeNotFound):
        raise HTTPException(404, 'Không có tuyến an toàn trong khu vực bản đồ')
    polyline = []
    length = 0.0
    for u, v in zip(nodes, nodes[1:]):
        k, data = min(H[u][v].items(), key=lambda kv: float(kv[1].get('length', float('inf'))))
        pts = coords_of(u, v, data)
        polyline.extend(pts if not polyline else pts[1:])
        length += float(data.get('length', 0))
    if not polyline:
        polyline = [[float(G.nodes[nodes[0]]['y']), float(G.nodes[nodes[0]]['x'])]]
    return {'coords': polyline, 'distance_m': round(length, 1), 'node_count': len(nodes)}
