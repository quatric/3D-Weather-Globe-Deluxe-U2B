#!/usr/bin/env python3
"""Scrape Apple's current retail store list into a JSON file.

Usage: apple_stores_scrape.py OUT.json

For every locale that has a store list, the list page is read for the store
URLs and each store page's JSON-LD block supplies the name, address, phone and
coordinates.  The result feeds apple_stores_update.py.
"""
import json
import re
import sys
import urllib.request
from concurrent.futures import ThreadPoolExecutor

# locale path -> (host, country name as stored in Atlas.DB)
LOCALES = {
    "": ("www.apple.com", "United States"),
    "ca": ("www.apple.com", "Canada"),
    "uk": ("www.apple.com", "United Kingdom"),
    "au": ("www.apple.com", "Australia"),
    "jp": ("www.apple.com", "Japan"),
    "cn": ("www.apple.com.cn", "China"),
    "de": ("www.apple.com", "Germany"),
    "fr": ("www.apple.com", "France"),
    "it": ("www.apple.com", "Italy"),
    "es": ("www.apple.com", "Spain"),
    "chde": ("www.apple.com", "Switzerland"),
    "befr": ("www.apple.com", "Belgium"),
    "nl": ("www.apple.com", "Netherlands"),
    "at": ("www.apple.com", "Austria"),
    "se": ("www.apple.com", "Sweden"),
    "hk": ("www.apple.com", "Hong Kong"),
    "kr": ("www.apple.com", "South Korea"),
    "tw": ("www.apple.com", "Taiwan"),
    "sg": ("www.apple.com", "Singapore"),
    "ae": ("www.apple.com", "United Arab Emirates"),
    "br": ("www.apple.com", "Brazil"),
    "mx": ("www.apple.com", "Mexico"),
    "in": ("www.apple.com", "India"),
    "th": ("www.apple.com", "Thailand"),
    "tr": ("www.apple.com", "Turkey"),
    "my": ("www.apple.com", "Malaysia"),
}


def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    for _ in range(3):
        try:
            return urllib.request.urlopen(req, timeout=40).read().decode("utf-8", "replace")
        except Exception:
            pass
    return None


def parse_store(host, locale, slug, country):
    base = "https://%s%s/retail/%s/" % (host, "/" + locale if locale and host == "www.apple.com" else "", slug)
    page = fetch(base)
    if not page:
        return None
    for m in re.finditer(r'<script type="application/ld\+json"[^>]*>(.*?)</script>', page, re.S):
        try:
            d = json.loads(m.group(1))
        except ValueError:
            continue
        if d.get("@type") == "LocalBusiness" and "geo" in d:
            a = d["address"]
            return {
                "url": base,
                "slug": slug,
                "locale": locale,
                "country": country,
                "code": d.get("branchCode"),
                "name": re.sub(r"^Apple\s+", "", d["name"]),
                "phone": (d.get("telephone") or "").strip(),
                "street": (a.get("streetAddress") or "").strip(),
                "city": (a.get("addressLocality") or "").strip(),
                "region": (a.get("addressRegion") or "").strip(),
                "zip": (a.get("postalCode") or "").strip(),
                "lat": d["geo"]["latitude"],
                "lon": d["geo"]["longitude"],
            }
    return None


def main(out):
    jobs = []
    for locale, (host, country) in LOCALES.items():
        prefix = "/" + locale if locale and host == "www.apple.com" else ""
        page = fetch("https://%s%s/retail/storelist/" % (host, prefix))
        if not page:
            print("no list for", locale or "us")
            continue
        slugs = []
        for s in re.findall(r'data-store-number="R\d+" href="(?:[^"]*)/retail/([a-z0-9_\-]+)/"', page):
            if s != "storelist" and s not in slugs:
                slugs.append(s)
        print(locale or "us", len(slugs))
        jobs += [(host, locale, s, country) for s in slugs]
    with ThreadPoolExecutor(8) as ex:
        res = list(ex.map(lambda j: parse_store(*j), jobs))
    stores = [r for r in res if r]
    for j, r in zip(jobs, res):
        if not r:
            print("FAILED", j)
    json.dump(stores, open(out, "w"), indent=1, ensure_ascii=False)
    print(len(stores), "stores")


if __name__ == "__main__":
    main(sys.argv[1])
