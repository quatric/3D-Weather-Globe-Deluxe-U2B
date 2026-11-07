#!/usr/bin/env python3
"""Build an Atlas.DB update (in the format of game/DB Updates/updNNNNN) that
brings the Apple Store entries in line with Apple's current store list.

Usage: apple_stores_update.py STORES.json ATLAS.DB OUT

STORES.json comes from apple_stores_scrape.py.  ATLAS.DB must be the database
the update will be applied to (the base database plus every earlier update).
The output is the zlib-compressed SQL script the app's updater expects.

  * stores missing from the database are added
  * stores that no longer exist are removed
  * stores that were renamed or moved (a new store within 900 m) are updated
  * changed addresses, phone numbers, coordinates and names are updated
"""
import json
import math
import random
import re
import sqlite3
import sys
import unicodedata
import zlib

UID_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_#"

DB_COUNTRY = {"South Korea": "Korea, South"}

# Stores whose pages are in a non-Latin script: the names used by the database.
NAMES = {
    "kyoto": "Kyoto", "umeda": "Umeda", "shinsaibashi": "Shinsaibashi", "nagoyasakae": "Nagoya Sakae",
    "ginza": "Ginza", "marunouchi": "Marunouchi", "shinjuku": "Shinjuku", "shibuya": "Shibuya",
    "omotesando": "Omotesando", "kawasaki": "Kawasaki", "fukuoka": "Fukuoka",
    "hanam": "Hanam", "jamsil": "Jamsil", "myeongdong": "Myeongdong", "garosugil": "Garosugil",
    "yeouido": "Yeouido", "gangnam": "Gangnam", "hongdae": "Hongdae",
    "xinyia13": "Xinyi A13", "taipei101": "Taipei 101",
    "globalharbor": "Global Harbor", "hongkongplaza": "Hong Kong Plaza", "wujiaochang": "Wujiaochang",
    "shanghaiiapm": "Shanghai iapm", "pudong": "Pudong", "jingan": "Jing'an", "nanjingeast": "Nanjing East",
    "qibao": "Qibao", "kunming": "Kunming", "xidanjoycity": "Xidan Joy City", "livatbeijing": "Beijing Liv",
    "chaoyangjoycity": "Chaoyang Joy City", "wangfujing": "Wangfujing", "chinacentralmall": "China Central Mall",
    "sanlitun": "Sanlitun", "mixcchengdu": "MixC Chengdu", "taikoolichengdu": "Taikoo Li Chengdu",
    "tianjinjoycity": "Tianjin Joy City", "mixctianjin": "MixC Tianjin", "riverside66tianjin": "Riverside 66 Tianjin",
    "mixchefei": "MixC Hefei", "parc66jinan": "Parc 66 Jinan", "mixcqingdao": "MixC Qingdao",
    "zhujiangnewtown": "Zhujiang New Town", "parccentral": "Parc Central", "mixcshenzhen": "MixC Shenzhen",
    "holidayplazashenzhen": "Holiday Plaza Shenzhen", "uniwalkqianhai": "Uniwalk Qianhai",
    "mixcnanning": "MixC Nanning", "wondercity": "Wonder City", "xinjiekou": "Xinjiekou",
    "xuanwulake": "Xuanwu Lake", "center66wuxi": "Center 66 Wuxi", "suzhou": "Suzhou",
    "mixczhengzhou": "MixC Zhengzhou", "tianyisquare": "Tianyi Square", "westlake": "West Lake",
    "mixchangzhou": "MixC Hangzhou", "mixcwenzhou": "MixC Wenzhou", "wuhan": "Wuhan", "changsha": "Changsha",
    "xiamenlifestylecenter": "Xiamen Lifestyle Center", "tahoeplaza": "Tahoe Plaza",
    "olympia66dalian": "Olympia 66 Dalian", "zhongjiejoycity": "Zhongjie Joy City", "mixcshenyang": "MixC Shenyang",
    "jiefangbei": "Jiefangbei", "mixcchongqing": "MixC Chongqing", "paradisewalkchongqing": "Paradise Walk Chongqing",
}

US_STATES = {
    "Alabama": "AL", "Alaska": "AK", "Arizona": "AZ", "Arkansas": "AR", "California": "CA", "Colorado": "CO",
    "Connecticut": "CT", "Delaware": "DE", "District of Columbia": "DC", "Florida": "FL", "Georgia": "GA",
    "Hawaii": "HI", "Idaho": "ID", "Illinois": "IL", "Indiana": "IN", "Iowa": "IA", "Kansas": "KS",
    "Kentucky": "KY", "Louisiana": "LA", "Maine": "ME", "Maryland": "MD", "Massachusetts": "MA",
    "Michigan": "MI", "Minnesota": "MN", "Mississippi": "MS", "Missouri": "MO", "Montana": "MT",
    "Nebraska": "NE", "Nevada": "NV", "New Hampshire": "NH", "New Jersey": "NJ", "New Mexico": "NM",
    "New York": "NY", "North Carolina": "NC", "North Dakota": "ND", "Ohio": "OH", "Oklahoma": "OK",
    "Oregon": "OR", "Pennsylvania": "PA", "Rhode Island": "RI", "South Carolina": "SC", "South Dakota": "SD",
    "Tennessee": "TN", "Texas": "TX", "Utah": "UT", "Vermont": "VT", "Virginia": "VA", "Washington": "WA",
    "West Virginia": "WV", "Wisconsin": "WI", "Wyoming": "WY", "Puerto Rico": "PR",
}
AU_STATES = {
    "Australian Capital Territory": "ACT", "New South Wales": "NSW", "Northern Territory": "NT",
    "Queensland": "QLD", "South Australia": "SA", "Tasmania": "TAS", "Victoria": "VIC", "Western Australia": "WA",
}
NON_LATIN = {"Japan", "China", "South Korea", "Taiwan", "Hong Kong", "Thailand", "Singapore"}
ZIP_FIRST = {"Germany", "Austria", "Switzerland", "Netherlands", "Spain", "Italy", "France", "Belgium", "Sweden"}

ABBR = {
    "st": "street", "ave": "avenue", "av": "avenue", "blvd": "boulevard", "rd": "road", "dr": "drive",
    "drv": "drive", "ln": "lane", "hwy": "highway", "pkwy": "parkway", "ctr": "center", "centre": "center",
    "n": "north", "s": "south", "e": "east", "w": "west", "ste": "suite", "sq": "square", "pl": "place",
    "ct": "court", "cir": "circle", "mt": "mount",
}


CITY_NAMES = {
    "京都市": "Kyoto", "大阪市": "Osaka", "名古屋市": "Nagoya", "福岡市": "Fukuoka", "川崎市": "Kawasaki",
    "上海": "Shanghai", "北京": "Beijing", "成都": "Chengdu", "天津": "Tianjin", "合肥": "Hefei", "济南": "Jinan",
    "青岛": "Qingdao", "广州": "Guangzhou", "深圳": "Shenzhen", "南宁": "Nanning", "南京": "Nanjing", "无锡": "Wuxi",
    "苏州": "Suzhou", "郑州": "Zhengzhou", "宁波": "Ningbo", "杭州": "Hangzhou", "温州": "Wenzhou", "武汉": "Wuhan",
    "长沙": "Changsha", "厦门": "Xiamen", "福州": "Fuzhou", "大连": "Dalian", "沈阳": "Shenyang", "重庆": "Chongqing",
    "昆明": "Kunming", "서울": "Seoul", "경기": "Hanam", "台北市": "Taipei", "香港": "Hong Kong",
    "กรุงเทพมหานคร": "Bangkok",
}
TOKYO_REGION = "東京都"


def fold(s):
    s = s.replace("ß", "ss")
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode().lower()
    return re.sub(r"[^a-z0-9]+", " ", s).strip()


def street_tokens(s):
    return [ABBR.get(t, t) for t in fold(s).split()]


def hav(a, b, c, d):
    p = math.pi / 180
    x = math.sin((c - a) * p / 2) ** 2 + math.cos(a * p) * math.cos(c * p) * math.sin((d - b) * p / 2) ** 2
    return 12742 * math.asin(math.sqrt(x))


def sq(s):
    return "'" + s.replace("'", "''") + "'"


def hexblob(s):
    return "X'" + s.encode("utf-8").hex().upper() + "'"


def clean(s):
    return re.sub(r"\s+", " ", (s or "").replace(" ", " ")).strip()


def address(s):
    street, city, region, zip_ = clean(s["street"]), clean(s["city"]), clean(s["region"]), clean(s["zip"])
    c = s["country"]
    if c == "United States":
        last = "%s, %s %s" % (city, US_STATES.get(region, region), zip_)
    elif c == "Canada":
        last = "%s, %s %s" % (city, region, zip_)
    elif c == "United Kingdom":
        last = "%s, %s" % (city, zip_)
    elif c == "Australia":
        last = "%s %s %s" % (city, AU_STATES.get(region, region), zip_)
    elif c in ZIP_FIRST:
        last = "%s %s" % (zip_, city)
    else:
        last = " ".join(x for x in (city, region if region != city else "", zip_) if x)
    return street + "\n" + last.strip()


def store_name(s):
    n = clean(s["name"])
    if s["slug"] == "appleparkvisitorcenter":
        return "Apple Park Visitor Center"
    if s["slug"] in NAMES:
        return NAMES[s["slug"]]
    if not n.isascii() and not re.search(r"[A-Za-zÀ-ɏ]", n):
        raise SystemExit("no English name for " + s["slug"])
    return n


def keywords(name):
    out = []
    for w in re.split(r"[^A-Za-z0-9]+", unicodedata.normalize("NFKD", name.replace("ß", "ss")).encode("ascii", "ignore").decode().upper()):
        if w and w not in out:
            out.append(w[:37])
    return out


def main(stores_path, db_path, out_path):
    stores = json.load(open(stores_path))
    for s in stores:
        s["name"] = store_name(s)
    db = sqlite3.connect(db_path)
    rng = random.Random(18)
    used = set()
    for t in ("geo_object", "entry_info", "blob_object", "geo_data"):
        used.update(r[0] for r in db.execute("select uid from %s" % t))

    def new_uid():
        while True:
            u = "".join(rng.choice(UID_CHARS) for _ in range(10))
            if u not in used:
                used.add(u)
                return u

    countries = {r[1]: r[0] for r in db.execute("select uid,name from geo_object where geotype='country'")}
    cities = {}

    def country_cities(cuid):
        if cuid not in cities:
            cities[cuid] = db.execute(
                "select uid,name,latitude,longitude,coalesce(population,0),host_state,host_county,timezone "
                "from geo_object where geotype='city' and host_country=? and latitude is not null and longitude is not null", (cuid,)).fetchall()
        return cities[cuid]

    def locate(s):
        """-> (country uid, city row or None, nearest row with a time zone)."""
        cuid = countries[DB_COUNTRY.get(s["country"], s["country"])]
        cs = country_cities(cuid)
        lat, lon = s["lat"], s["lon"]
        near = sorted(((hav(lat, lon, c[2], c[3]), c) for c in cs), key=lambda x: x[0])
        city_name = "Tokyo" if clean(s["region"]) == TOKYO_REGION else CITY_NAMES.get(clean(s["city"]), s["city"])
        named = [(d, c) for d, c in near if d < 60 and fold(c[1]) == fold(city_name)]
        city = named[0][1] if named else None
        if not city:
            close = [(d, c) for d, c in near if d < 20]
            if close:
                city = max(close, key=lambda x: (x[1][4], -x[0]))[1]
        tz = next((c for d, c in near if c[7]), None)
        return cuid, city, tz, bool(named)

    rows = db.execute(
        "select g.uid,g.name,g.latitude,g.longitude,g.alt_type from geo_object g where g.geotype='apple store'").fetchall()
    info = {}
    for uid, name, lat, lon, alt in rows:
        d = {"uid": uid, "name": name, "lat": lat, "lon": lon, "alt": alt}
        for r in db.execute("select key,val_type,uid,val from entry_info where object_uid=? and key in ('address','phone','url','host city','host city name')", (uid,)):
            if r[0] == "url" and r[1] == "s":
                d["url_row"], d["url"] = r[2], r[3]
            elif r[0] == "address":
                d["addr_row"], d["addr_blob"] = r[2], r[3]
            elif r[0] == "phone":
                d["phone_row"], d["phone"] = r[2], r[3]
        b = db.execute("select cast(blob_data as text) from blob_object where uid=?", (d.get("addr_blob"),)).fetchone()
        d["addr"] = b[0] if b else ""
        info[uid] = d

    def slug_of(url):
        m = re.search(r"retail/([^/]+)/?", url or "")
        return m.group(1) if m else None

    by_slug = {s["slug"]: s for s in stores}
    matched, orphans = {}, []
    for uid, d in info.items():
        s = by_slug.get(slug_of(d.get("url")))
        if s and s["url"] not in {m["url"] for m in matched.values()}:
            matched[uid] = s
        else:
            orphans.append(uid)
    claimed = {s["url"] for s in matched.values()}
    free = [s for s in stores if s["url"] not in claimed]
    moved, closed = {}, []
    for uid in orphans:
        d = info[uid]
        best = min(((hav(d["lat"], d["lon"], s["lat"], s["lon"]), s) for s in free), key=lambda x: x[0], default=None)
        if best and best[0] <= 0.9:
            moved[uid] = best[1]
            free.remove(best[1])
        else:
            closed.append(uid)
    added = free

    out = ["BEGIN TRANSACTION;"]
    tmp_kw = []
    log = {"closed": [], "added": [], "moved": [], "changed": []}

    def set_address(d, s):
        text = address(s)
        out.append("UPDATE OR REPLACE blob_object SET blob_data=%s WHERE uid=%s;" % (hexblob(text), sq(d["addr_blob"])))

    # closed
    for uid in closed:
        d = info[uid]
        log["closed"].append("%s (%s)" % (d["name"], d.get("url")))
        blobs = [r[0] for r in db.execute("select val from entry_info where object_uid=? and key='address'", (uid,))]
        out.append("DELETE FROM ocurences WHERE uid=%s;" % sq(uid))
        if blobs:
            out.append("DELETE FROM blob_object WHERE uid IN (%s);" % ", ".join(sq(b) for b in blobs))
        out.append("DELETE FROM geo_data WHERE object_uid=%s;" % sq(uid))
        out.append("DELETE FROM entry_info WHERE object_uid=%s;" % sq(uid))
        out.append("DELETE FROM geo_object WHERE uid=%s;" % sq(uid))

    # moved / renamed / changed
    for uid, s, kind in [(u, s, "moved") for u, s in moved.items()] + [(u, s, "changed") for u, s in matched.items()]:
        d = info[uid]
        notes = []
        sets = []
        cjk = s["country"] in NON_LATIN
        far = hav(d["lat"], d["lon"], s["lat"], s["lon"]) > 0.25
        if far:
            sets += ["latitude=%s" % round(s["lat"], 5), "longitude=%s" % round(s["lon"], 5)]
            notes.append("coords")
        if fold(d["name"]) != fold(s["name"]):
            sets.append("name=" + sq(s["name"]))
            notes.append("name %s -> %s" % (d["name"], s["name"]))
            out.append("DELETE FROM ocurences WHERE uid=%s;" % sq(uid))
            tmp_kw += [(uid, w) for w in keywords(s["name"])]
        if d["alt"] != "retail":
            sets.append("alt_type='retail'")
            notes.append("now open")
        if sets:
            out.append("UPDATE OR REPLACE geo_object SET %s WHERE uid=%s;" % (", ".join(sets), sq(uid)))
        new_addr = address(s)
        old_t, new_t = street_tokens(d["addr"].split("\n")[0]), street_tokens(new_addr.split("\n")[0])
        if cjk:
            # the stored text is an English transcription: the street numbers must still overlap
            nums = lambda x: set(re.findall(r"\d+", x))
            addr_changed = far or not nums(new_addr.split("\n")[0]) & nums(d["addr"])
        else:
            ov = len(set(old_t) & set(new_t)) / max(1, len(set(old_t) | set(new_t)))
            addr_changed = old_t != new_t and (ov < 0.8 or [t for t in old_t if t.isdigit()] != [t for t in new_t if t.isdigit()])
        if kind == "moved":
            addr_changed = True
        if addr_changed and d.get("addr_blob"):
            set_address(d, s)
            notes.append("address %r -> %r" % (d["addr"].replace("\n", " / "), new_addr.replace("\n", " / ")))
        if s["phone"] and re.sub(r"\D", "", d.get("phone") or "") != re.sub(r"\D", "", s["phone"]) and d.get("phone_row"):
            out.append("UPDATE OR REPLACE entry_info SET val=%s WHERE uid=%s;" % (sq(s["phone"]), sq(d["phone_row"])))
            notes.append("phone %s -> %s" % (d.get("phone"), s["phone"]))
        if kind == "moved":
            out.append("UPDATE OR REPLACE entry_info SET val=%s WHERE uid=%s;" % (sq(s["url"]), sq(d["url_row"])))
            notes.append("url")
        if notes:
            log[kind].append("%s: %s" % (d["name"], "; ".join(notes)))

    # added
    for s in added:
        uid = new_uid()
        cuid, city, tz, named = locate(s)
        cols = {"host_country": cuid, "geotype": "apple store", "timezone": (city[7] if city and city[7] else (tz[7] if tz else None)),
                "uid": uid, "longitude": round(s["lon"], 5), "latitude": round(s["lat"], 5), "name": s["name"], "alt_type": "retail"}
        if city and city[6]:
            cols["host_county"] = city[6]
        if city and city[5]:
            cols["host_state"] = city[5]
        cols = {k: v for k, v in cols.items() if v is not None}
        vals = ", ".join(sq(v) if isinstance(v, str) else repr(v) for v in cols.values())
        blob = new_uid()
        out.append("INSERT INTO blob_object ('DEL', 'blob_data', 'uid', 'kind') VALUES (0, %s, %s, 'text');" % (hexblob(address(s)), sq(blob)))
        out.append("INSERT INTO geo_data ('representation_type', 'object_uid', 'uid', 'blob_uid') VALUES ('entry_info_text', %s, %s, %s);" % (sq(uid), sq(blob), sq(blob)))
        out.append("INSERT INTO geo_object (%s) VALUES (%s);" % (", ".join(sq(k) for k in cols), vals))

        def entry(key, vt, val, parent=None, ):
            u = new_uid()
            c = ["object_uid", "uid", "DEL", "key", "sort_idx", "val_type"] + (["val"] if val is not None else []) + ["parent_uid"]
            v = [sq(uid), sq(u), "0", sq(key), "0", sq(vt)] + ([sq(val)] if val is not None else []) + [sq(parent or uid)]
            out.append("INSERT INTO entry_info (%s) VALUES (%s);" % (", ".join(sq(x) for x in c), ", ".join(v)))
            return u

        p = entry("url", "p", None)
        entry("url", "s", s["url"], p)
        entry("address", "t", blob)
        if city and (named or s["country"] in NON_LATIN):
            entry("host city", "s", city[0])
        else:
            entry("host city name", "s", clean(s["city"]))
        if s["phone"]:
            entry("phone", "s", s["phone"])
        tmp_kw += [(uid, w) for w in keywords(s["name"])]
        log["added"].append("%s, %s (%s)" % (s["name"], s["city"], s["country"]))

    if tmp_kw:
        out.append("CREATE TEMPORARY TABLE _tmp_kw (uid char(10), kw varchar(37));")
        out.append("CREATE TEMPORARY TRIGGER updater AFTER INSERT ON _tmp_kw BEGIN INSERT OR IGNORE INTO keyword VALUES (new.kw, substr(new.kw||'   ', 1, 3)); INSERT INTO ocurences SELECT rowid, new.uid FROM keyword WHERE kw=new.kw; END;")
        out += ["INSERT INTO _tmp_kw VALUES (%s, %s);" % (sq(u), sq(w)) for u, w in tmp_kw]
        out.append("DROP TRIGGER updater;")
        out.append("DROP TABLE _tmp_kw;")
    out.append("COMMIT;")
    sql = "\r\n".join(out) + "\r\n"
    open(out_path, "wb").write(zlib.compress(sql.encode("utf-8"), 9))
    open(out_path + ".sql", "w").write(sql)
    for k, v in log.items():
        print("== %s (%d)" % (k, len(v)))
        for line in v:
            print("  " + line)


if __name__ == "__main__":
    main(*sys.argv[1:4])
