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

def get_init_image(image_file, target_width, target_height):
    img = cv2.imread(image_file, cv2.IMREAD_COLOR)
    assert img is not None, f"can't load {image_file}"
    iw, ih = img.shape[1::-1]
    tw, th = target_width, target_height
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
    # corner points of projector
    corners = np.array([(0, 0), (img_w, 0), (img_w, img_h), (0, img_h)])
    screen_r1 = [] # left  projection points in screen
    screen_r2 = [] # right projection points in screen
    for u, v in corners:
        x, z, w = H31 @ [u, v, 1]
        screen_r1.append([x/w, z/w])
        x, z, w = H32 @ [u, v, 1]
        screen_r2.append([x/w, z/w])
    return screen_r1, screen_r2

def adjust_screen_regions(screen_r1, screen_r2, image_roi):
    p1, p2, p3, p4 = screen_r1  # left quad in screen
    p5, p6, p7, p8 = screen_r2  # right quad in screen

    # adjust top points
    p1[1] = p2[1] = p5[1] = p6[1] = min(p1[1], p2[1], p5[1], p6[1])
    # adjust bottom points
    p3[1]= p4[1] = p7[1] = p8[1] = max(p3[1], p4[1], p7[1], p8[1])
    # adjust left region
    p1[0] = p4[0] = max(p1[0], p4[0])
    p2[0] = p3[0] = min(p2[0], p3[0])
    # adjust right region
    p5[0] = p8[0] = max(p5[0], p8[0])
    p6[0] = p7[0] = min(p6[0], p7[0])

    # screen roi:[left,  top,   width,       height]
    screen_roi = [p1[0], p1[1], p6[0]-p1[0], p1[1]-p4[1]]

    # adjust screen roi based on image aspect
    x, z, w, h = screen_roi
    _, _, iw, ih = image_roi
    if iw/ih < w/h:
        tw = h*iw/ih
        th = h
    else:
        tw = w
        th = w*ih/iw
    tx = x + 0.5*(w - tw)
    tz = z - 0.5*(h - th)
    screen_roi = [tx, tz, tw, th]

    # FIXME: screen_r1, screen_r2 would be out of screen_roi
    return screen_r1, screen_r2, screen_roi

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

def get_image_regions(Hi3, screen_r1, screen_r2, img_w, img_h):
    # left region of image
    image_r1 = []
    for x, z in screen_r1:
        u, v, w = Hi3 @ [x, z, 1]
        u = min(max(u/w, 0), img_w)
        v = min(max(v/w, 0), img_h)  
        image_r1.append([u, v])

    # right region of image
    image_r2 = []
    for x, z in screen_r2:
        u, v, w = Hi3 @ [x, z, 1]
        u = min(max(u/w, 0), img_w)
        v = min(max(v/w, 0), img_h)
        image_r2.append([u, v])

    # overlap rect
    p3 = image_r1[2]
    p5 = image_r2[0]
    overlap = [p5[0], p5[1], p3[0]-p5[0], p3[1]-p5[1]]   
    return image_r1, image_r2, overlap

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

def generate_image_masks(img_w, img_h, overlap_roi):
    m1 = np.ones((img_h, img_w, 1))
    m2 = np.ones((img_h, img_w, 1))    
    x, y, w, h = map(round, overlap_roi)
    if w > 0 and h > 0:
        m1[y:y+h,x:x+w] = np.linspace(1, 0, w).reshape(w, 1)
        m2[y:y+h,x:x+w] = 1.0 - m1[y:y+h,x:x+w]
    return m1, m2

def generate_projector_images(init_img, mask1, mask2, H1i, H2i, img_w, img_h):
    masked1 = np.array(init_img * mask1, dtype=np.uint8)
    masked2 = np.array(init_img * mask2, dtype=np.uint8)
    img1 = cv2.warpPerspective(masked1, H1i, dsize=(img_w, img_h))
    img2 = cv2.warpPerspective(masked2, H2i, dsize=(img_w, img_h))
    return img1, img2

def print_points(points, prefix=""):
    s = f"{prefix}"
    for x, y in points:
        s = s + f" ({float(x):.3}, {float(y):.3})"
    print(s)

def print_values(values, prefix=""):
    s = f"{prefix}"
    for v in values:
        s = s + f" {float(v):.3}"
    print(s)

def imshow(win_name, image, fx=0.5, fy=0.5):
    image = cv2.resize(image, dsize=None, fx=fx, fy=fx)
    cv2.imshow(win_name, image)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <image-file>")
        sys.exit(-1)
    init_img, image_roi = get_init_image(sys.argv[1], 2*IMG_W, IMG_H)
    K = get_camera_matrix(IMG_W, IMG_H, FOV_H, FOV_V)
    T13, T23 = screen_projector_transforms(D1, D2, ALPHA)
    H13, H23 = screen_projector_homographies(K, T13, T23)
    H31, H32 = np.linalg.inv(H13), np.linalg.inv(H23)

    screen_r1, screen_r2 = get_screen_regions(H31, H32, IMG_W, IMG_H)
    print_points(screen_r1, "screen_r1")
    print_points(screen_r2, "screen_r2")

    screen_r1, screen_r2, screen_roi = adjust_screen_regions(screen_r1, screen_r2, image_roi)
    print_points(screen_r1, "screen_r1")
    print_points(screen_r2, "screen_r2")
    print_values(screen_roi, "screen_roi")
    
    Hi3 = screen_image_homography(screen_roi, image_roi)
    H3i = np.linalg.inv(Hi3)

    Hi1 = Hi3 @ H31     # left  projector -> left  region of image
    Hi2 = Hi3 @ H32     # right projector -> right region of image

    H1i = H13 @ H3i     # left  region of image -> left  projector
    H2i = H23 @ H3i     # right region of image -> right projector

    # get left/right/overlap region of init image
    image_r1, image_r2, overlap_roi = get_image_regions(Hi3, screen_r1, screen_r2, 2*IMG_W, IMG_H)
    print_points(image_r1, "image_r1")
    print_points(image_r2, "image_r2")
    print_values(overlap_roi, "overlap_roi")

    # get left/right projector regions
    projector_r1, projector_r2 = get_projector_regions(H1i, H2i, image_r1, image_r2)
    print_points(projector_r1, "projector_r1")
    print_points(projector_r2, "projector_r2")

    mask1, mask2 = generate_image_masks(2*IMG_W, IMG_H, overlap_roi)
    image1, image2 = generate_projector_images(init_img, mask1, mask2, H1i, H2i, IMG_W, IMG_H)
    imshow("left  projector image", image1)
    imshow("right projector image", image2)
    cv2.waitKey()
```
