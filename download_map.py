
import osmnx as ox
from pathlib import Path

# Cấu hình máy chủ Overpass
ox.settings.overpass_url = "https://overpass-api.de/api"
ox.settings.requests_timeout = 180
ox.settings.use_cache = True

# Tọa độ tâm tạm thời quanh khu vực UTT, Triều Khúc
# Cần kiểm tra lại vị trí chính xác trên bản đồ
LAT = 20.9885
LON = 105.8035

print("Dang tai du lieu duong pho quanh UTT...")

try:
    G = ox.graph_from_point(
        (LAT, LON),
        dist=500,
        dist_type="bbox",
        network_type="drive",
        simplify=True
    )

    output = Path(__file__).resolve().parent / "roads.graphml"
    ox.save_graphml(G, filepath=output)

    print("TAI BAN DO THANH CONG!")
    print("So node:", len(G.nodes))
    print("So canh:", len(G.edges))
    print("File:", output)

except Exception as e:
    print("LOI TAI BAN DO:", repr(e))
    raise
