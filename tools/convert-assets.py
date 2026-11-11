#!/usr/bin/env python3
"""Convert the CleanHome web assets into native-friendly files.

Run from the project root:

    python3 tools/convert-assets.py ~/CleanHome-wgt
"""
import os
import shutil
import sys

from fontTools.ttLib import TTFont
import cairosvg
from PIL import Image


def ensure(path):
    os.makedirs(path, exist_ok=True)


def convert_fonts(src, dst):
    ensure(dst)
    for name in ("outfit-300.woff2", "outfit-500.woff2"):
        source = os.path.join(src, "fonts", name)
        target = os.path.join(dst, name.replace(".woff2", ".ttf"))
        if not os.path.exists(source):
            print("missing font", source)
            continue
        font = TTFont(source)
        font.flavor = None
        font.save(target)
        print("font", target)


def convert_icons(src, dst):
    ensure(dst)
    icons = os.path.join(src, "icons")
    for name in sorted(os.listdir(icons)):
        source = os.path.join(icons, name)
        if not os.path.isfile(source):
            continue
        if name.endswith(".svg"):
            if os.path.getsize(source) == 0:
                print("skip empty", name)
                continue
            target = os.path.join(dst, name[:-4] + ".png")
            size = 512 if "card" in name else 256
            cairosvg.svg2png(url=source, write_to=target,
                             output_width=size, output_height=size)
            print("icon", target)
        elif name.endswith(".png"):
            target = os.path.join(dst, name)
            shutil.copy2(source, target)
            print("icon", target)

    app_src = os.path.join(icons, "apps")
    app_dst = os.path.join(dst, "apps")
    ensure(app_dst)
    if os.path.isdir(app_src):
        for name in sorted(os.listdir(app_src)):
            if name.endswith(".png"):
                shutil.copy2(os.path.join(app_src, name), os.path.join(app_dst, name))
                print("app icon", os.path.join(app_dst, name))


def convert_wallpapers(src, dst):
    ensure(dst)
    thumbs_dst = os.path.join(dst, "thumbs")
    ensure(thumbs_dst)
    wp_src = os.path.join(src, "wallpapers")
    for name in sorted(os.listdir(wp_src)):
        source = os.path.join(wp_src, name)
        if not os.path.isfile(source) or not name.endswith(".jpg"):
            continue
        target = os.path.join(dst, name)
        shutil.copy2(source, target)
        print("wallpaper", target)
    thumbs_src = os.path.join(wp_src, "thumbs")
    if os.path.isdir(thumbs_src):
        for name in sorted(os.listdir(thumbs_src)):
            if name.endswith(".jpg"):
                shutil.copy2(os.path.join(thumbs_src, name), os.path.join(thumbs_dst, name))
                print("thumb", os.path.join(thumbs_dst, name))


def convert_launcher_icon(src, dst, tizen_dst):
    source = os.path.join(src, "icon.png")
    if not os.path.exists(source):
        return
    im = Image.open(source).convert("RGBA")
    im.thumbnail((512, 512))
    target = os.path.join(dst, "icon.png")
    im.save(target)
    print("icon", target)
    ensure(tizen_dst)
    im.save(os.path.join(tizen_dst, "icon.png"))


def main():
    source = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else "~/CleanHome-wgt")
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    assets = os.path.join(root, "assets")
    ensure(assets)
    convert_fonts(source, os.path.join(assets, "fonts"))
    convert_icons(source, os.path.join(assets, "icons"))
    convert_wallpapers(source, os.path.join(assets, "wallpapers"))
    convert_launcher_icon(source, assets, os.path.join(root, "tizen", "shared", "res"))
    print("done")


if __name__ == "__main__":
    main()
