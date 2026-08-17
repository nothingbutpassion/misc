# Python
## pytest
- Install
  ```shell
  pip install pytest
  ```
- Write testcase
  ```py
  # NOTES: function name starts with `test_`
  def test_add() {
    # Use built-in `assert` to check
    assert 1+1 == 2, "Error: 1+1 != 2"
  }
  ```
- Run testcase
  ```shell
  cd <python-testcase-dir>
  pytest -v
  ```
## Save code
```python
import sys
import math
import cv2
import numpy as np

# NOTES: 
# There are 4 coordinate frames:
# frame0: dual-projectors-center frame (origin is ground projection point of the center)
# frame1: left projector frame
# frame2: right projector frame
# frame3: screen coordinate frame
IMG_W = 1920                                        # projector image width
IMG_H = 1080                                        # projector image height
FOV_H = math.radians(30.0)                          # projector horizontal fov        
FOV_V = 2*math.atan(math.tan(FOV_H/2)*IMG_H/IMG_W)  # projector vertical fov    
D1 = 2                                              # distance between 2 projectors
D2 = 6                                              # distance from projectors-center to screen
ALPHA = math.radians(30.0)                          # screen angle relative to projectors

def get_init_image(img_file, img_w, img_h):
    img = cv2.imread(img_file, cv2.IMREAD_COLOR)
    assert img is not None, f"Can't load {img_file}"
    iw, ih = img.shape[1::-1]
    tw, th = img_w, img_h
    if iw/ih < tw/th:
        w = round(iw*th/ih)
        h = th
    else:
        w = tw
        h = round(ih*tw/iw)
    x0 = (tw - w)//2
    y0 = (th - h)//2
    scaled_img = cv2.resize(img, dsize=(w, h), interpolation=cv2.INTER_CUBIC)
    target_img = np.zeros((th, tw, 3), dtype=np.uint8)
    target_img[y0:y0+h,x0:x0+w] = scaled_img
    return target_img, (x0, y0, w, h)

def get_camera_matrix(w, h, fov_h, fov_v):
    cx, cy = 0.5*w, 0.5*h
    fx, fy = 0.5*w/math.tan(0.5*fov_h), 0.5*h/math.tan(0.5*fov_v)
    return np.array([
        [fx, 0,  cx],
        [0,  fy, cy],
        [0,  0,  1 ]
    ])

def screen_projector_transforms(d1, d2, alpha):
    # frame0 (x0, y0, z0, 1) -> frame1 (x1, y1, z1, 1)
    T10 = np.identity(4)
    T10[:3, 3] = np.array([d1/2, 0, 0]) 
    T10[:3,:3] = np.array([
        [1, 0, 0 ],
        [0, 0, -1],
        [0, 1, 0 ]
    ])
    # frame0 -> frame2
    T20 = np.identity(4)
    T20[:3, 3] = np.array([-d1/2, 0, 0])
    T20[:3,:3] = np.array([
        [1, 0, 0 ],
        [0, 0, -1],
        [0, 1, 0 ]
    ])
    # frame3 -> frame0
    T03 = np.identity(4)
    T03[:3, 3] = np.array([0, d2, 0])
    T03[:3,:3] = np.array([
        [math.cos(alpha), -math.sin(alpha), 0],
        [math.sin(alpha),  math.cos(alpha), 0],
        [0,                 0,                1]
    ])
    
    T13 = T10 @ T03 # fram3 -> frame 1
    T23 = T20 @ T03 # fram3 -> frame 2
    return T13, T23

def screen_projector_homographies(K, T13, T23):
    # screen point (x3, z3, 1) -> left projector image point (u1, v1, 1)
    H13 = np.zeros((3, 3))
    H13[:,0] = T13[:3,0]    # 1st column of Rotation matrix
    H13[:,1] = T13[:3,2]    # 3rd column of Roataion matrix
    H13[:,2] = T13[:3,3]    # translation
    H13 = K @ H13
    # screen point (x3, z3, 1) -> left projector image point (u2, v2, 1)
    H23 = np.zeros((3, 3))
    H23[:,0] = T23[:3,0]
    H23[:,1] = T23[:3,2]
    H23[:,2] = T23[:3,3]
    H23 = K @ H23
    return H13, H23

def get_screen_regions(H31, H32, img_w, img_h):
    corners = np.array([(0, 0), (img_w, 0), (img_w, img_h), (0, img_h)])
    r1 = [] # left  projection points in screen
    r2 = [] # right projection points in screen
    for u, v in corners:
        x, z, w = H31 @ [u, v, 1]
        r1.append([x/w, z/w])
        x, z, w = H32 @ [u, v, 1]
        r2.append([x/w, z/w])

    p1, p2, p3, p4 = r1 # left quad in screen
    p5, p6, p7, p8 = r2 # right quad in screen

    # adjust top points
    z_max = min(p1[1], p2[1], p5[1], p6[1])
    p1[1] = p2[1] = p5[1] = p6[1] = z_max
    # adjust top points
    z_min = max(p3[1], p4[1], p7[1], p8[1])
    p3[1]= p4[1] = p7[1] = p8[1] = z_min
    assert z_min < z_max

    # adjust left points
    x_min = max(p1[0], p4[0])
    p1[0] = p4[0] = x_min
    # adjust right points
    x_max = min(p6[0], p7[0])
    p6[0] = p7[0] = x_max
    assert x_min < x_max

    # roi: [left, top, width, height]
    roi = [x_min, z_max, x_max-x_min, z_max-z_min] 
    return roi, r1, r2

def adjust_screen_regions(screen_roi, r1, r2, image_roi):
    _, _, iw, ih = image_roi
    x, z, w, h = screen_roi
    ia = iw/ih
    a = w/h
    # adjust size of screen roi based on image aspect
    if ia < a:
        tw = h*ia
        th = h
    else:
        tw = w
        th = w/ia
    # adjust top-left of screen roi
    tx = x + 0.5*(w - tw)
    tz = z - 0.5*(h - th)
    screen_roi = [tx, tz, tw, th]

    # adjust left projection region
    p1, p2, p3, p4 = r1
    p1[0] = p4[0] = max(p1[0], p4[0], tx)
    p2[0] = p3[0] = min(p2[0], p3[0], tx+tw)

    # adjust right projection region
    p5, p6, p7, p8 = r2
    p5[0] = p8[0] = max(p5[0], p8[0], tx)
    p6[0] = p7[0] = min(p6[0], p7[0], tx+tw)
    return screen_roi, r1, r2

def screen_image_homography(screen_roi, image_roi):
    u, v, iw, ih = image_roi
    x, z, w, h = screen_roi
    assert abs(iw/w - ih/h) < 1e-5
    sx, sy = iw/w, ih/h
    # get homography: screen (x, z, 1) -> image (u, v, 1)
    H = np.array([
      [sx,  0,  -sx*x + u],
      [0, -sy,   sy*z + v],
      [0,   0,   1       ]
    ])
    return H

def get_image_regions(Hi3, screen_r1, screen_r2):
    # left region of image
    image_r1 = []
    for x, z in screen_r1:
        u, v, w = Hi3 @ [x, z, 1]
        image_r1.append([u/w, v/w])
    # right region of image
    image_r2 = []
    for x, z in screen_r2:
        u, v, w = Hi3 @ [x, z, 1]
        image_r2.append([u/w, v/w])
    return image_r1, image_r2

def get_projector_regions(H1i, H2i, image_r1, image_r2):
    # effective region in left projector
    projector_r1 = []
    for u1, v1 in image_r1:
        u, v, w = H1i @ [u1, v1, 1]
        projector_r1.append([u/w, v/w])
    # effective region in right projector
    projector_r2 = []
    for u2, v2 in image_r2:
        u, v, w = H2i @ [u2, v2, 1]
        projector_r2.append([u/w, v/w])
    return projector_r1, projector_r2

def print_points(points, prefix=""):
    s = f"{prefix}"
    for x, y in points:
        s = s + f" ({x:.3}, {y:.3})"
    print(s)

def print_values(values, prefix=""):
    s = f"{prefix}"
    for v in values:
        s = s + f" {v:.3}"
    print(s)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <image-file>")
        sys.exit(-1)
    init_img, image_roi = get_init_image(sys.argv[1], 2*IMG_W, IMG_H)
    K = get_camera_matrix(IMG_W, IMG_H, FOV_H, FOV_V)
    T13, T23 = screen_projector_transforms(D1, D2, ALPHA)
    H13, H23 = screen_projector_homographies(K, T13, T23)
    H31, H32 = np.linalg.inv(H13), np.linalg.inv(H23)

    screen_roi, screen_r1, screen_r2 = get_screen_regions(H31, H32, IMG_W, IMG_H)
    print_values(screen_roi, "roi")
    print_points(screen_r1, "r1")
    print_points(screen_r2, "r2")

    screen_roi, screen_r1, screen_r2 = adjust_screen_regions(screen_roi, screen_r1, screen_r2, image_roi)
    print_values(screen_roi, "screen_roi")
    print_points(screen_r1, "screen_r1")
    print_points(screen_r2, "screen_r2")
    
    Hi3 = screen_image_homography(screen_roi, image_roi)
    image_r1, image_r2 = get_image_regions(Hi3, screen_r1, screen_r2)
    print_points(image_r1, "image_r1")
    print_points(image_r2, "image_r2")

    Hi1 = Hi3 @ H31             # left  projector -> left  part of image
    Hi2 = Hi3 @ H32             # right projector -> right part of image
    H1i = np.linalg.inv(Hi1)    # left  part of image -> left  projector
    H2i = np.linalg.inv(Hi2)    # right part of image -> right projector
    projector_r1, projector_r2 = get_projector_regions(H1i, H2i, image_r1, image_r2)
    print_points(projector_r1, "projector_r1")
    print_points(projector_r2, "projector_r2")

    # The follows code is only for debug!
    src1 = np.array(image_r1)
    dst1 = np.array(projector_r1)
    H1, mask1 = cv2.findHomography(src1, dst1)
    image1 = cv2.warpPerspective(init_img, H1, dsize=(IMG_W, IMG_H))
    image1 = cv2.resize(image1, dsize=None, fx=0.25, fy=0.25)
    cv2.imshow("project image1", image1)
    src2 = np.array(image_r2)
    dst2 = np.array(projector_r2)
    H2, mask2 = cv2.findHomography(src2, dst2)
    image2 = cv2.warpPerspective(init_img, H2, dsize=(IMG_W, IMG_H))
    image2 = cv2.resize(image2, dsize=None, fx=0.25, fy=0.25)
    cv2.imshow("project image2", image2)
    cv2.waitKey()
```
