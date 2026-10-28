
import osmnx as ox
from pathlib import Path

folder = Path(__file__).resolve().parent
input_file = folder / "map.osm"
output_file = folder / "roads.graphml"

print("Dang doc du lieu OSM...")

G = ox.graph_from_xml(
    input_file,
    simplify=True,
    retain_all=True,
    bidirectional=False
)

# Chỉ giữ các đường phù hợp với mạng xe cơ giới.
# Loại các đường chỉ dành cho người đi bộ, xe đạp...
excluded = {
    "footway", "path", "pedestrian",
    "steps", "cycleway", "bridleway",
    "construction", "proposed"
}

remove = []
for u, v, k, data in G.edges(keys=True, data=True):
    types = data.get("highway", [])
    if isinstance(types, str):
        types = [types]
    if types and all(t in excluded for t in types):
        remove.append((u, v, k))

G.remove_edges_from(remove)
G.remove_nodes_from(list(__import__("networkx").isolates(G)))

ox.save_graphml(G, filepath=output_file)

print("Thanh cong!")
print("Nodes:", len(G.nodes))
print("Edges:", len(G.edges))
print("Saved:", output_file)
