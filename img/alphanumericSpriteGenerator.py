from PIL import Image, ImageDraw, ImageFont
import string

cell = 32
cols = 16
rows = 4

width = cols * cell
height = rows * cell

chars = string.ascii_lowercase + string.ascii_uppercase + string.digits

img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

font = ImageFont.truetype("DejaVuSans.ttf", 24)

for i, ch in enumerate(chars):
    col = i % cols
    row = i // cols
    x = col * cell
    y = row * cell

    bbox = draw.textbbox((0, 0), ch, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]

    draw.text(
        (x + (cell - tw)//2, y + (cell - th)//2),
        ch,
        font=font,
        fill=(255, 255, 255, 255)
    )

img.save("source/font_32.png")