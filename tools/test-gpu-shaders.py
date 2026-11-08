#!/usr/bin/env python3
"""Check the actual Xenos shader containers embedded by the title build.

Uses xsdk's scalar microcode interpreter, not the desktop GLSL compiler. This
checks arithmetic/lowering; texture hardware, rasterization and timing still
require the console. Usage: test-gpu-shaders.py XSDK_ROOT generated_header
"""
import math
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(sys.argv[1]) / "xenos-toolchain"))
from xdis.simulate import run  # noqa: E402

text = Path(sys.argv[2]).read_text()


def shader(name):
    body = re.search(rf"{name}_container\[\] = \{{(.*?)\}};", text, re.S)[1]
    words = [int(w, 16) for w in re.findall(r"0x[0-9A-Fa-f]+", body)]
    blob = struct.pack(f">{len(words)}I", *words)
    constants = {}
    table = re.search(rf"{name}_literals\[\] = \{{(.*?)\n\}};", text, re.S)
    if table:
        for reg, values in re.findall(r"\{ (\d+), \{ (.*?) \} \}", table[1]):
            constants[int(reg)] = [float(v.strip().removesuffix("f")) for v in values.split(",")]
    return blob, constants


def rgb(values):
    return [round(min(1, max(0, v)) * 255) for v in values[:3]]


def original(x, y, phase, color):
    u, v = (x / 383 - .51) * 2, (y / 215 - .49) * 1.7
    a, b = u + .14 * math.sin(v * 3 + phase), v + .11 * math.cos(phase * .73)
    radius = math.sqrt(a * a * .83 + b * b)
    band = (radius - .65 - .12 * math.sin((x / 383 - .51) * 6.2 + phase * .6)) * 4.2
    ribbon = math.exp(-band * band)
    wave = math.sin(u * 3.4) * math.cos(v * 2.8 + phase * .8) + math.cos(u * 3.4) * math.sin(v * 2.8 + phase * .8)
    light = .69 + ribbon * .57 + wave * .08 + math.sin(radius * 9 - phase) * .045
    return [int(min(255, max(0, c * light + ribbon * r))) for c, r in zip(color, [18, 25, 5])]


blob, literals = shader("swirl_frag")
samples = 0
for color in ([222, 91, 15], [218, 151, 57], [47, 178, 47], [37, 150, 220], [144, 91, 212]):
    for frame in (0, 1, 60, 150, 1113, 23520):
        phase = frame / 30 * .8
        for x, y in ((0, 0), (383, 215), (192, 108), (66, 54), (322, 150), (100, 164), (300, 54), (13, 191)):
            constants = dict(literals, **{})
            constants[0], constants[1] = color + [1], [phase, 0, 0, 0]
            got = rgb(run(blob, {0: [(x + .5) / 384, (y + .5) / 216, 0, 0]}, constants).exports[0])
            want = original(x, y, phase, color)
            assert max(abs(a - b) for a, b in zip(got, want)) <= 1, (x, y, frame, got, want)
            samples += 1
print(f"PASS: {samples} compiled Xenos swirl samples match supplied equations within one RGB level")


def texel(x, y):
    return [(x * 13 + y * 17) % 256, (x * 3 + y * 23) % 256, (x * 29 + y * 7) % 256, 255]


def fetch(_slot, uv):
    x, y = min(383, max(0, int(uv[0] * 384))), min(215, max(0, int(uv[1] * 216)))
    return [v / 255 for v in texel(x, y)]


blob, literals = shader("background_frag")
for x in (0, 1, 137, 639, 640, 993, 1278, 1279):
    for y in (0, 1, 71, 359, 610, 718, 719):
        px, py = x * 383 * 256 // 1279, y * 215 * 256 // 719
        ix, iy, fx, fy = px // 256, py // 256, px % 256, py % 256
        a, b, c, d = texel(ix, iy), texel(min(383, ix + 1), iy), texel(ix, min(215, iy + 1)), texel(min(383, ix + 1), min(215, iy + 1))
        top = [(av * (256 - fx) + bv * fx) // 256 for av, bv in zip(a, b)]
        bottom = [(av * (256 - fx) + bv * fx) // 256 for av, bv in zip(c, d)]
        want = [(av * (256 - fy) + bv * fy) // 256 for av, bv in zip(top, bottom)][:3]
        got = rgb(run(blob, {0: [(x + .5) / 1280, (y + .5) / 720, 0, 0]}, literals, texture_fetch=fetch).exports[0])
        assert max(abs(a - b) for a, b in zip(got, want)) <= 2, (x, y, got, want)
print("PASS: compiled Xenos point fetches and separable 720p background filtering")

blob, literals = shader("ui_frag")
for alpha in (0, 1, 32, 127, 128, 254, 255):
    for opacity in (0, 69, 101, 255):
        got = run(blob, {0: [.5, .5, 0, 0], 1: [.2, .4, .6, opacity / 255]}, literals,
                  texture_fetch=lambda _s, _uv: [1, 1, 1, alpha / 255]).exports[0]
        assert max(abs(a - b) for a, b in zip(got[:3], [.2, .4, .6])) < 1e-6
        assert round(got[3] * 255) == alpha * opacity // 255, (alpha, opacity, got)
print("PASS: compiled Xenos glyph tint and integer alpha coverage")

blob, literals = shader("copy_frag")
constants = dict(literals)
constants[0] = [.375, 0, 0, 0]
got = run(blob, {0: [.5, .5, 0, 0]}, constants, texture_fetch=lambda _s, _uv: [.2, .4, .6, 1]).exports[0]
assert max(abs(a - b) for a, b in zip(got, [.2, .4, .6, .375])) < 1e-6
print("PASS: compiled Xenos content fade shader")

blob, literals = shader("cap_frag")
for height in (50, 51, 60, 67):
    constants = dict(literals)
    constants[0] = [128, 68, height, 0]
    for y in (0, 1, height // 2, height - 1):
        def cap_fetch(_s, uv):
            row = min(67, max(0, int(uv[1] * 68)))
            return [v / 255 for v in (row * 17 % 256, row * 7 % 256, row * 29 % 256, row * 13 % 256)]
        y0, y1 = y * 68 // height, ((y + 1) * 68 + height - 1) // height
        rows = [[round(v * 255) for v in cap_fetch(0, [.5, (i + .5) / 68])] for i in range(y0, y1)]
        alpha = sum(p[3] for p in rows)
        want = [sum(p[k] * p[3] for p in rows) // alpha if alpha else 0 for k in range(3)] + [alpha // len(rows)]
        uv = [.5, (.5 + y * 68 / height) / 68, 0, 0]
        out = run(blob, {0: uv}, constants, texture_fetch=cap_fetch).exports[0]
        got = rgb(out) + [round(out[3] * 255)]
        assert max(abs(a - b) for a, b in zip(got, want)) <= 1, (height, y, got, want)
print("PASS: compiled Xenos cap box filter preserves alpha-aware area samples")

# Video uses three independent linear R8 textures. Validate actual compiled
# Xenos arithmetic/sampler slots rather than only the desktop GLSL source.
blob, literals = shader("video_frag")
checks = 0
for hd in (False, True):
    for full in (False, True):
        kr, kb = (.2126, .0722) if hd else (.299, .114)
        kg = 1-kr-kb
        ys, cs = (1., 1.) if full else (255/219, 255/224)
        constants = dict(literals)
        constants[0] = [ys, 0, 2*(1-kr)*cs, 0]
        constants[1] = [ys, -2*kb*(1-kb)*cs/kg, -2*kr*(1-kr)*cs/kg, 0]
        constants[2] = [ys, 2*(1-kb)*cs, 0, 0]
        constants[3] = [0 if full else 16/255, 128/255, 0, 0]
        for y, u, v in ((16,128,128), (235,128,128), (128,80,176), (80,176,80), (0,128,128), (255,128,128)):
            for xy in ((0,0), (.5,.5), (1,1)):
                fetched=[]
                def plane_fetch(slot, uv):
                    assert slot in (0,1,2), slot
                    assert max(abs(a-b) for a,b in zip(uv[:2],xy)) < 1e-6, (slot, uv, xy)
                    fetched.append(slot)
                    return [(y,u,v)[slot]/255,0,0,1]
                got=run(blob,{0:[*xy,0,0],1:[1,1,1,1]},constants,texture_fetch=plane_fetch).exports[0]
                luma=y/255 if full else (y-16)/219
                chroma=1/255 if full else 1/224
                uu,vv=(u-128)*chroma,(v-128)*chroma
                want=[luma+2*(1-kr)*vv,luma-2*(kb*(1-kb)*uu+kr*(1-kr)*vv)/kg,luma+2*(1-kb)*uu,1]
                want=[min(1,max(0,a)) for a in want]
                assert sorted(fetched)==[0,1,2], fetched
                assert max(abs(a-b) for a,b in zip(got,want)) < 2/255,(hd,full,y,u,v,xy,got,want)
                checks+=1
print(f"PASS: {checks} compiled Xenos video samples; three Y/U/V sampler slots, BT601/709 limited/full, boundaries and RGB")
