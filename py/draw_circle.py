import math
import sys
import os
import numpy as np
import cv2


def draw_circle(img, center, radius, color):
    cx, cy = center
    h, w = img.shape[:2]
    x0, x1 = max(0, cx - radius), min(w-1, cx + radius)
    y0, y1 = max(0, cy - radius), min(h-1, cy + radius)
    x0, x1 = int(np.floor(x0)), int(np.ceil(x1))
    y0, y1 = int(np.floor(y0)), int(np.ceil(y1))
    for y in range(y0, y1+1):
        for x in range(x0, x1+1):
            d = math.sqrt((x-cx)**2 + (y-cy)**2) - radius
            thresh = 0.5
            if d < -thresh:
                img[y, x] = color
            elif d > thresh:
                continue
            else:
                backgroud = img[y, x]
                r = 0.5 - d/(2*thresh)
                img[y, x] = np.round(r*np.array(color) + (1-r)*backgroud)

def test_draw_circle(img=None):
    if img is None:
        img = np.zeros((240, 480, 3), dtype='uint8')
    h, w = img.shape[:2]
    r = min(h, w)//16

    cv2.circle(img, (w//4, h//4),   r, (255, 0, 0), cv2.FILLED, cv2.LINE_AA)
    cv2.circle(img, (w//4, 2*h//4), r, (0, 255, 0), cv2.FILLED, cv2.LINE_AA)
    cv2.circle(img, (w//4, 3*h//4), r, (0, 0, 255), cv2.FILLED, cv2.LINE_AA)

    draw_circle(img, (3*w/4, h/4),   r, (255, 0, 0))
    draw_circle(img, (3*w/4, 2*h/4), r, (0, 255, 0))
    draw_circle(img, (3*w/4, 3*h/4), r, (0, 0, 255))

    cv2.imshow("image", img)
    cv2.waitKey()


if __name__ == "__main__":
    img = None
    if len(sys.argv) > 1:
        img=cv2.imread(sys.argv[1])
    test_draw_circle(img)





