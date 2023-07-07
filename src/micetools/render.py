import pygame
import pygame.gfxdraw as gfx
from PIL import Image, ImageDraw
import math

# surf = pygame.Surface((1080, 1080))
# surf.fill((0, 0, 0))

surf = Image.new("RGB", (720 * 2, 1080))
draw = ImageDraw.Draw(surf)


def region(rX, rY, rW, rH):
    # # Out of region
    # if (mX < rX || mY < rY || mX > rX + rW || mY > rY + rH) return 0;

    # # i is now a position in a region from (-1,-1) to (1,1)
    # float iX = ((mX - rX) / (rW / 2)) - 1;
    # float iY = ((mY - rY) / (rH / 2)) - 1;

    # float r2 = iX * iX + iY * iY;

    # # Bound to a unit circle
    # if (r2 > 1) return 0;

    mid = (rX + rW // 2, rY + rH // 2)
    scale = min(rW, rH) / 2

    outer = (rX, rY, rX + rW, rY + rH)
    innerR = 298 / 540
    inner = (
        rX + (rW / 2) * (1 - innerR),
        rY + (rH / 2) * (1 - innerR),
        rX + rW - (rW / 2) * (1 - innerR),
        rY + rH - (rW / 2) * (1 - innerR),
    )
    centreR = 121 / 540
    centre = (
        rX + (rW / 2) * (1 - centreR),
        rY + (rH / 2) * (1 - centreR),
        rX + rW - (rW / 2) * (1 - centreR),
        rY + rH - (rW / 2) * (1 - centreR),
    )

    draw.pieslice(outer, 0, 45, fill=(0, 255, 0))
    draw.pieslice(outer, 45, 90, fill=(127, 255, 127))
    draw.pieslice(outer, 90, 135, fill=(0, 255, 0))
    draw.pieslice(outer, 135, 180, fill=(127, 255, 127))
    draw.pieslice(outer, 180, 225, fill=(0, 255, 0))
    draw.pieslice(outer, 225, 270, fill=(127, 255, 127))
    draw.pieslice(outer, 270, 315, fill=(0, 255, 0))
    draw.pieslice(outer, 315, 360, fill=(127, 255, 127))

    draw.pieslice(inner, 0, 45, fill=(255, 0, 0))
    draw.pieslice(inner, 45, 90, fill=(255, 127, 127))
    draw.pieslice(inner, 90, 135, fill=(255, 0, 0))
    draw.pieslice(inner, 135, 180, fill=(255, 127, 127))
    draw.pieslice(inner, 180, 225, fill=(255, 0, 0))
    draw.pieslice(inner, 225, 270, fill=(255, 127, 127))
    draw.pieslice(inner, 270, 315, fill=(255, 0, 0))
    draw.pieslice(inner, 315, 360, fill=(255, 127, 127))

    draw.pieslice(centre, 0, 360, (0, 0, 255))

    # gfx.arc(s, *mid, 500, 0, 90, (0, 255, 0))

    # pygame.draw.arc(s, (0, 255, 0), (0, 0, 500, 500), 0, math.radians(90), 100)

    # pygame.draw.circle(s, (255, 0, 0), mid, (121/540) * scale)


    # # Centre button
    # if (r2 < (121.0f / 540.0f) * (121.0f / 540.0f)) return BUTTON_C;

    # WORD button = 0;

    # # Four quadrants
    # if (iX > 0 && iY < 0) button = -iY > iX ? BUTTON_A1 : BUTTON_A2;
    # if (iX > 0 && iY > 0) button = iX > iY ? BUTTON_A3 : BUTTON_A4;
    # if (iX < 0 && iY > 0) button = iY > -iX ? BUTTON_A5 : BUTTON_A6;
    # if (iX < 0 && iY < 0) button = -iX > -iY ? BUTTON_A7 : BUTTON_A8;

    # # Inner ring of buttons
    # if (r2 < (298.0f / 540.0f) * (298.0f / 540.0f))
    #     button = (button & 0xff00) | ((button & 0xff) << 1);

    # return button;

region(0, 360, 720, 720)
region(720, 360, 720, 720)
surf.save("out.png")
# pygame.image.save(surf, "out.png")
