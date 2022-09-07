import sys
import os
import cv2
import numpy as np

def show_nv12(img):
    bgr = cv2.cvtColor(img, cv2.COLOR_YUV2BGR_NV12)
    cv2.imshow("nv12", bgr)
    cv2.waitKey()


def show_bgr(img):
    cv2.imshow("bgr", img)
    cv2.waitKey()

def i420_to_nv12(img):
    h, w = img.shape[:2]
    h = h//6
    y = img[:4*h]
    u = img[4*h:5*h]
    v = img[5*h:]
    uv = cv2.merge([u,v])
    uv = uv.reshape(-1, w)
    img[4*h:]= uv
    return img

def ycbcr_to_rgb(y, cb, cr):
    r = y + 1.402   * (cr-128)
    g = y - 0.34414 * (cb-128) -  0.71414 * (cr-128)
    b = y + 1.772   * (cb-128)
    if b < 255: b += 1
    return int(r), int(g), int(b)

def rgb_to_ycbcr(r, g, b):
    assert(255>=r)
    assert(255>=g)
    assert(255>=b)
    y  = 0.299*r + 0.587*g    + 0.114*b
    cb = 128     - 0.168736*r - 0.331364*g + 0.5*b
    cr = 128     + 0.5*r      - 0.418688*g - 0.081312*b
    return int(y), int(cb), int(cr)

def draw_nv12_rect(img, r, c):
    pt1 = (r[0], r[1])
    pt2 = (r[0]+r[2]-1, r[1]+r[3]-1)
    h, w = img.shape[:2]
    h = h//6
    y = img[:4*h]
    uv = img[4*h:]
    uv = uv.reshape(-1, w//2, 2)
    r, g, b = c
    cy, cu, cv = rgb_to_ycbcr(r, g, b)
    y = cv2.rectangle(y, pt1, pt2, (cy,), 1, cv2.LINE_AA)
    pt1, pt2 = (pt1[0]//2, pt1[1]//2), (pt2[0]//2, pt2[1]//2)
    uv = cv2.rectangle(uv, pt1, pt2, (cu,cv), 1, cv2.LINE_AA)
    uv = uv.reshape(-1, w)
    img[:4*h] = y
    img[4*h:] = uv

def draw_nv12_circle(img, pt, c):
    h, w = img.shape[:2]
    h = h//6
    y = img[:4*h]
    uv = img[4*h:]
    uv = uv.reshape(-1, w//2, 2)
    r, g, b = c
    cy, cu, cv = rgb_to_ycbcr(r, g, b)
    y = cv2.circle(y, pt, 8, (cy,), -1, cv2.LINE_AA)
    pt = (pt[0]//2, pt[1]//2)
    uv = cv2.circle(uv, pt, 4, (cu,cv), -1, cv2.LINE_AA)
    uv = uv.reshape(-1, w)
    img[:4*h] = y
    img[4*h:] = uv

def draw_nv12_line(img, pt1, pt2, c):
    h, w = img.shape[:2]
    h = h//6
    y = img[:4*h]
    uv = img[4*h:]
    uv = uv.reshape(-1, w//2, 2)
    r, g, b = c
    cy, cu, cv = rgb_to_ycbcr(r, g, b)
    y = cv2.line(y, pt1, pt2, (cy,), 2, cv2.LINE_AA)
    pt1, pt2 = (pt1[0]//2, pt1[1]//2), (pt2[0]//2, pt2[1]//2)
    uv = cv2.line(uv, pt1, pt2, (cu,cv), 2, cv2.LINE_AA)
    uv = uv.reshape(-1, w)
    img[:4*h] = y
    img[4*h:] = uv


def main(imgfile):
    bgr = cv2.imread(imgfile, cv2.IMREAD_COLOR)
    bgr = cv2.resize(bgr, (256,256))
    # show_bgr(bgr)
    i420 = cv2.cvtColor(bgr, cv2.COLOR_BGR2YUV_I420)
    nv12 = i420_to_nv12(i420)
    draw_nv12_rect(nv12, (64,64,64,64), (0, 0, 255))
    draw_nv12_circle(nv12, (96, 96), (255, 0, 0))
    draw_nv12_line(nv12, (64, 64), (128, 128), (0, 255, 0))
    show_nv12(nv12)

if __name__ == "__main__":
    if len(sys.argv) != 2 or not os.path.isfile(sys.argv[1]):
        print(f"Usage: {sys.argv[0]} <image-file>")
        sys.exit(-1)
    main(sys.argv[1])

