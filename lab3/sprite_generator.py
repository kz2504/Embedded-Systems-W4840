from PIL import Image

# ---------- settings ----------
IMG_PATH = "ball.png"
SPRITE_W = 32
SPRITE_H = 32

OUT_FILE = "ball_rgb_lut.txt"
OUT_PREVIEW = "ball_preview.png"

ARRAY_NAME_R = "BALL_R"
ARRAY_NAME_G = "BALL_G"
ARRAY_NAME_B = "BALL_B"
ARRAY_NAME_MASK = "BALL_MASK"
# -----------------------------

img = Image.open(IMG_PATH).convert("RGBA")

if img.size != (SPRITE_W, SPRITE_H):
    raise ValueError(f"Expected image size {(SPRITE_W, SPRITE_H)}, got {img.size}")

pixels = img.load()

ball_r = []
ball_g = []
ball_b = []
ball_mask = []

for y in range(SPRITE_H):
    row_r = []
    row_g = []
    row_b = []
    row_m = []
    for x in range(SPRITE_W):
        r, g, b, a = pixels[x, y]
        if a == 0:
            row_r.append(0)
            row_g.append(0)
            row_b.append(0)
            row_m.append(0)
        else:
            row_r.append(r)
            row_g.append(g)
            row_b.append(b)
            row_m.append(1)
    ball_r.append(row_r)
    ball_g.append(row_g)
    ball_b.append(row_b)
    ball_mask.append(row_m)

with open(OUT_FILE, "w") as f:
    f.write(f"localparam logic {ARRAY_NAME_MASK} [0:{SPRITE_H-1}][0:{SPRITE_W-1}] = '{{\n")
    for y in range(SPRITE_H):
        entries = ", ".join("1'b1" if v else "1'b0" for v in ball_mask[y])
        comma = "," if y != SPRITE_H - 1 else ""
        f.write(f"    '{{{entries}}}{comma}\n")
    f.write("};\n\n")

    for name, data in [(ARRAY_NAME_R, ball_r), (ARRAY_NAME_G, ball_g), (ARRAY_NAME_B, ball_b)]:
        f.write(f"localparam logic [7:0] {name} [0:{SPRITE_H-1}][0:{SPRITE_W-1}] = '{{\n")
        for y in range(SPRITE_H):
            entries = ", ".join(f"8'h{val:02X}" for val in data[y])
            comma = "," if y != SPRITE_H - 1 else ""
            f.write(f"    '{{{entries}}}{comma}\n")
        f.write("};\n\n")

print(f"Wrote to {OUT_FILE}")

# preview from the same exact data
preview = Image.new("RGBA", (SPRITE_W, SPRITE_H))
for y in range(SPRITE_H):
    for x in range(SPRITE_W):
        if ball_mask[y][x]:
            preview.putpixel((x, y), (ball_r[y][x], ball_g[y][x], ball_b[y][x], 255))
        else:
            preview.putpixel((x, y), (0, 0, 0, 0))

preview_big = preview.resize((SPRITE_W * 16, SPRITE_H * 16), Image.Resampling.NEAREST)
preview_big.save(OUT_PREVIEW)
print(f"Wrote preview image to {OUT_PREVIEW}")

preview_big.show()