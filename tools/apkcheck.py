#!/usr/bin/env python3
import argparse
import os
import re
import struct
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ENGINES = [
    ("libunity.so", "Unity", "не влезет в память Виты"),
    ("libil2cpp.so", "Unity (IL2CPP)", "не влезет в память Виты"),
    ("libUE4.so", "Unreal Engine 4", "не влезет в память Виты"),
    ("libUnreal.so", "Unreal Engine", "не влезет в память Виты"),
    ("libyoyo.so", "GameMaker", "лучше запускать через YoYo Loader"),
    ("libgodot_android.so", "Godot", "нужен GLES3, шансов мало"),
    ("libcocos2dcpp.so", "Cocos2d-x", "реально, такие порты есть"),
    ("libcocos2djs.so", "Cocos2d-JS", "тяжело, но возможно"),
    ("libgdx.so", "libGDX", "логика игры на Java, не подойдёт"),
    ("libSDL2.so", "SDL2", "реально, вход через SDL_main"),
    ("libmonodroid.so", "Xamarin/Mono", "логика на C#, не подойдёт"),
    ("libflutter.so", "Flutter", "не подойдёт"),
]

GLES3_HINTS = {"glBindVertexArray", "glGenVertexArrays", "glDrawArraysInstanced", "glBindBufferRange", "glTexStorage2D",
               "glBlitFramebuffer", "glDrawBuffers", "glMapBufferRange", "glUniformBlockBinding", "glTexImage3D"}


def dynlib_symbols():
    text = open(os.path.join(ROOT, "source", "dynlib.c"), encoding="utf-8", errors="replace").read()
    return set(re.findall(r'\{\s*"([^"]+)"\s*,', text))


def parse_elf(data):
    if data[:4] != b"\x7fELF":
        return None
    info = {"class": data[4], "machine": struct.unpack_from("<H", data, 18)[0], "needed": [], "undefined": set(), "defined": set()}
    if info["class"] != 1 or info["machine"] != 40:
        return info
    shoff, = struct.unpack_from("<I", data, 32)
    shentsize, shnum = struct.unpack_from("<HH", data, 46)
    sections = [struct.unpack_from("<IIIIIIIIII", data, shoff + i * shentsize) for i in range(shnum)]
    for sec in sections:
        stype, offset, size, link, entsize = sec[1], sec[4], sec[5], sec[6], sec[9]
        if stype == 11 and entsize:
            stroff, strsize = sections[link][4], sections[link][5]
            strtab = data[stroff:stroff + strsize]
            for i in range(1, size // entsize):
                name, value, ssize, sinfo, other, shndx = struct.unpack_from("<IIIBBH", data, offset + i * entsize)
                end = strtab.find(b"\0", name)
                sym = strtab[name:end].decode("utf-8", "replace")
                if not sym:
                    continue
                (info["undefined"] if shndx == 0 else info["defined"]).add(sym)
        elif stype == 6 and entsize:
            stroff, strsize = sections[link][4], sections[link][5]
            strtab = data[stroff:stroff + strsize]
            for i in range(size // entsize):
                tag, val = struct.unpack_from("<iI", data, offset + i * entsize)
                if tag == 1:
                    end = strtab.find(b"\0", val)
                    info["needed"].append(strtab[val:end].decode("utf-8", "replace"))
    return info


def mb(n):
    return "%.1f МБ" % (n / 1048576)


def main():
    ap = argparse.ArgumentParser(description="Проверка APK на пригодность для VLoader и распаковка в папку игры")
    ap.add_argument("apk")
    ap.add_argument("--out", help="куда разложить .so и assets (папка ux0:data/vloader/TITLEID)")
    ap.add_argument("--profile", help="profile.txt, который положить рядом")
    ap.add_argument("--all", action="store_true", help="показать все недостающие функции")
    args = ap.parse_args()

    z = zipfile.ZipFile(args.apk)
    names = z.namelist()
    abis = sorted({n.split("/")[1] for n in names if n.startswith("lib/") and n.count("/") >= 2})
    print("APK:", os.path.basename(args.apk), mb(os.path.getsize(args.apk)))
    print("Архитектуры:", ", ".join(abis) if abis else "нет нативных библиотек")
    if "armeabi-v7a" not in abis:
        if abis == ["armeabi"]:
            print("ВЕРДИКТ: только старый ARMv5 (armeabi), теоретически можно, но не проверялось")
        elif abis:
            print("ВЕРДИКТ: не подойдёт, нет 32-битной версии armeabi-v7a")
        else:
            print("ВЕРДИКТ: не подойдёт, игра целиком на Java/Kotlin")
        if "armeabi" not in abis:
            return 1
    abi = "armeabi-v7a" if "armeabi-v7a" in abis else "armeabi"

    libs = [n for n in names if n.startswith("lib/%s/" % abi) and n.endswith(".so")]
    assets = [n for n in names if n.startswith("assets/") and not n.endswith("/")]
    assets_size = sum(z.getinfo(n).file_size for n in assets)
    known = dynlib_symbols()
    parsed = {}
    for n in libs:
        parsed[n] = parse_elf(z.read(n))
    provided = set()
    for info in parsed.values():
        if info:
            provided |= info["defined"]

    print("Ресурсы assets: %d файлов, %s" % (len(assets), mb(assets_size)))
    obb_hint = any(n.endswith(".obb") for n in names)
    if obb_hint:
        print("Внутри есть .obb")
    print()

    verdict = []
    for lib_name, engine, note in ENGINES:
        if any(os.path.basename(n) == lib_name for n in libs):
            print("Движок: %s (%s)" % (engine, note))
            verdict.append((engine, note))

    total_missing = 0
    for n in sorted(libs, key=lambda x: -z.getinfo(x).file_size):
        info = parsed[n]
        base = os.path.basename(n)
        size = z.getinfo(n).file_size
        if not info or info.get("machine") != 40:
            print("%s: не ARM ELF" % base)
            continue
        missing = sorted(s for s in info["undefined"] if s not in known and s not in provided)
        total_missing += len(missing)
        natives = sorted(s for s in info["defined"] if s.startswith("Java_"))
        entry = []
        if "JNI_OnLoad" in info["defined"]:
            entry.append("JNI_OnLoad")
        if "ANativeActivity_onCreate" in info["defined"] or "android_main" in info["defined"]:
            entry.append("NativeActivity")
        if "SDL_main" in info["defined"]:
            entry.append("SDL_main")
        gles3 = sorted(info["undefined"] & GLES3_HINTS)
        print("%s  %s" % (base, mb(size)))
        print("  зависит от: %s" % (", ".join(info["needed"]) or "-"))
        print("  вход: %s; Java-функций: %d" % (", ".join(entry) or "не найден", len(natives)))
        for s in natives[:12]:
            print("    " + s)
        if len(natives) > 12:
            print("    ... ещё %d" % (len(natives) - 12))
        if gles3:
            print("  использует OpenGL ES 3: %s" % ", ".join(gles3[:6]))
        print("  функций системы: %d, из них нет в загрузчике: %d" % (len(info["undefined"]), len(missing)))
        shown = missing if args.all else missing[:15]
        for s in shown:
            print("    " + s)
        if len(missing) > len(shown):
            print("    ... ещё %d (ключ --all покажет все)" % (len(missing) - len(shown)))
        print()

    code_size = sum(z.getinfo(n).file_size for n in libs)
    print("Итого кода: %s" % mb(code_size))
    if verdict and any("не " in note for _, note in verdict):
        print("ВЕРДИКТ: скорее всего не подойдёт (%s)" % verdict[0][0])
    elif code_size > 60 * 1048576:
        print("ВЕРДИКТ: сомнительно, код слишком большой для 512 МБ памяти")
    elif total_missing == 0:
        print("ВЕРДИКТ: все системные функции покрыты, можно пробовать с профилем")
    else:
        print("ВЕРДИКТ: можно пробовать, но в загрузчик нужно добавить %d функций" % total_missing)

    if args.out:
        os.makedirs(args.out, exist_ok=True)
        for n in libs:
            with open(os.path.join(args.out, os.path.basename(n)), "wb") as f:
                f.write(z.read(n))
        for n in assets:
            dst = os.path.join(args.out, n)
            if not os.path.realpath(dst).startswith(os.path.realpath(args.out) + os.sep):
                continue
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, "wb") as f:
                f.write(z.read(n))
        if args.profile:
            with open(args.profile, "rb") as src, open(os.path.join(args.out, "profile.txt"), "wb") as dst:
                dst.write(src.read())
        print("Разложено в", args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
