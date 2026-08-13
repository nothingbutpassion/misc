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
IMG_W = 1920                    # projector image width
IMG_H = 1080                    # projector image height
FOV_H = math.radians(30.0)      # projector horizontal fov
FOV_V = math.radians(18.0)      # projector vertical fov
D1 = 2                          # distance between 2 projectors
D2 = 6                          # distance from projectors-center to screen
ALPHA = math.radians(30.0)      # screen angle relative to projectors

class Projector:
    def __init__(self, w, h, fov_h, fov_v):
        self.w = w
        self.h = h
        self.fov_h = fov_h
        self.fov_v = fov_v
        self.K = self.camera_matrix(self.w, self.h, self.fov_h, self.fov_v)
        self.Ki = self.project_matrix(self.w, self.h, self.fov_h, self.fov_v)

    def camera_matrix(self, w, h, fov_h, fov_v):
        cx, cy = 0.5*w, 0.5*h
        fx, fy  = 0.5*w/math.tan(0.5*fov_h), 0.5*h/math.tan(0.5*fov_v)
        return np.array([
            [fx, 0,  cx],
            [0,  fy, cy],
            [0,  0,  1 ]
        ])

    def project_matrix(self, w, h, fov_h, fov_v):
        cx, cy = 0.5*w, 0.5*h
        fx, fy  = 0.5*w/math.tan(0.5*fov_h), 0.5*h/math.tan(0.5*fov_v)
        return np.array([
            [1/fx, 0,    -cx/fx],
            [0,    1/fy, -cy/fy],
            [0,    0,    1     ]
        ])

    def xyz2uv(self, x, y, z):
        u, v, w = self.K @ [x, y, z]
        return u/w, v/w

    def uv2pxyz(self, u, v, plain):
        a, b, c, d = plain
        x, y, z = self.Ki @ [u, v, 1]
        assert abs(a*x + b*y + c*z) > 1e-7, f"light ray {x,y,z} is parallel to plane {a,b,c,d}"
        s = -d/(a*x+b*y+c*z)
        return s*x, s*y, s*z

def get_init_image(img_file):
    img = cv2.imread(img_file, cv2.IMREAD_COLOR)
    assert img is not None, f"Can't load {img_file}"
    iw, ih = img.shape[1::-1]
    tw, th = 2*IMG_W, IMG_H
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

def get_transforms(d1, d2, alpha):
    # frame0 -> frame1
    M10 = np.identity(4)
    M10[:3, 3] = np.array([d1/2, 0, 0]) 
    M10[:3,:3] = np.array([
        [1, 0, 0 ],
        [0, 0, -1],
        [0, 1, 0 ]
    ])
    # frame0 -> frame2
    M20 = np.identity(4)
    M20[:3, 3] = np.array([-d1/2, 0, 0])
    M20[:3,:3] = np.array([
        [1, 0, 0 ],
        [0, 0, -1],
        [0, 1, 0 ]
    ])
    # frame0 -> frame3
    M30 = np.identity(4)
    M30[:3, 3] = np.array([-d2*math.sin(alpha), -d2*math.cos(alpha), 0])
    M30[:3,:3] = np.array([
        [math.cos(-alpha), -math.sin(-alpha), 0],
        [math.sin(-alpha),  math.cos(-alpha), 0],
        [0,                 0,                1]
    ])
    return M10, M20, M30

def project_corners(projector, corners, plane3, M3x):
    pass


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <image-file>")
        sys.exit(-1)
    init_img, init_roi = get_init_image(sys.argv[1])

    M10, M20, M30 = get_transforms(D1, D2, ALPHA)
    M01, M02, M03 = np.linalg.inv(M10), np.linalg.inv(M20), np.linalg.inv(M30)
    M31 = M30 @ M01
    M13 = M10 @ M03
    M32 = M30 @ M02
    M23 = M20 @ M03
    plane3 = [0, 1, 0, 0]
    plane1 = plane3 @ M31
    plane2 = plane3 @ M32

    projector1 = Projector(IMG_W, IMG_H, FOV_H, FOV_V)
    projector2 = Projector(IMG_W, IMG_H, FOV_H, FOV_V)

    corners = np.array([
        [0,     0    ], 
        [IMG_W, 0    ], 
        [IMG_W, IMG_H],
        [0,     IMG_H]
    ])

    # compute left projection region
    xyz1 = [projector1.uv2pxyz(u, v, plane1) for u, v in corners]
    xyzw3 = [M31 @ [x,y,z,1] for x, y, z in xyz1]
    p1, p2, p3, p4 = [[x/w, y/w, z/w] for x, y, z, w in xyzw3]

    # compute left projection region
    xyz2 = [projector2.uv2pxyz(u, v, plane2) for u, v in corners]
    xyzw3 = [M32 @ [x,y,z,1] for x, y, z in xyz2]
    p5, p6, p7, p8 = [[x/w, y/w, z/w] for x, y, z, w in xyzw3]

    z_max = min(p1[2], p2[2], p5[2], p6[2])
    p1[2], p2[2], p5[2], p6[2] = z_max, z_max, z_max, z_max

    z_min = max(p3[2], p4[2], p7[2], p8[2])
    p3[2], p4[2], p7[2], p8[2] = z_min, z_min, z_min, z_min
    
    kp1 = [p1, p2, p3, p4, p5, p6]
    kp2 = [p5, p6, p7, p8, p2, p3]

    xyzw1 = [M13 @ [x, y, z, 1] for x, y, z in kp1]
    uv1 = [projector1.xyz2uv(x/w, y/w, z/w) for x, y, z, w in xyzw1]

    xyzw2 = [M23 @ [x, y, z, 1] for x, y, z in kp2]
    uv2 = [projector1.xyz2uv(x/w, y/w, z/w) for x, y, z, w in xyzw2]
```
