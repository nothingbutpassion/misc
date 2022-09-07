import os
import cv2
import dlib
import numpy as np

face_landmark_path = os.path.dirname(os.path.abspath(__file__)) + '/shape_predictor_68_face_landmarks.dat'

#K = [6.5308391993466671e+002, 0.0, 3.1950000000000000e+002,
#     0.0, 6.5308391993466671e+002, 2.3950000000000000e+002,
#     0.0, 0.0, 1.0]
#D = [7.0834633684407095e-002, 6.9140193737175351e-002, 0.0, 0.0, -1.3073460323689292e+000]

# apprximate fx and fy by image width, cx by half image width, cy by half image height
cx = 320
cy = 240
fx = cx / np.tan(60/2 * np.pi / 180)
fy = fx

K = [fx, 0.0, cx,
     0.0, fy, cy,
     0.0, 0.0, 1.0]
D = [0.0, 0.0, 0.0, 0.0, 0.0]

cam_matrix = np.array(K).reshape(3, 3).astype(np.float32)
dist_coeffs = np.array(D).reshape(5, 1).astype(np.float32)

object_pts = np.float32([
    [6.825897, 6.760612, 4.402142],
    [1.330353, 7.122144, 6.903745],
    [-1.330353, 7.122144, 6.903745],
    [-6.825897, 6.760612, 4.402142],
    [5.311432, 5.485328, 3.987654],
    [1.789930, 5.393625, 4.413414],
    [-1.789930, 5.393625, 4.413414],
    [-5.311432, 5.485328, 3.987654],
    [2.005628, 1.409845, 6.165652],
    [-2.005628, 1.409845, 6.165652],
    [2.774015, -2.080775, 5.048531],
    [-2.774015, -2.080775, 5.048531],
    [0.000000, -3.116408, 6.097667],
    [0.000000, -7.415691, 4.070434]
    ])

axis_pts = np.float32([
    [0.0, 0.0, 0.0],
    [10.0, 0.0, 0.0],
    [0.0, 10.0, 0.0],
    [0.0, 0.0, 10.0]
    ])

line_pairs = [[0, 1], [1, 2], [2, 3], [3, 0],
              [4, 5], [5, 6], [6, 7], [7, 4],
              [0, 4], [1, 5], [2, 6], [3, 7]]

def shape_to_np(shape):
    return np.array([[p.x, p.y] for p in shape.parts()], np.float32)
        
def get_head_pose(image, image_pts):
    _, rotation_vec, translation_vec = cv2.solvePnP(object_pts, image_pts, cam_matrix, dist_coeffs)
    # calculate euler angle
    rotation_mat, _ = cv2.Rodrigues(rotation_vec)
    pose_mat = cv2.hconcat((rotation_mat, translation_vec))
    _, _, _, _, _, _, euler_angle = cv2.decomposeProjectionMatrix(pose_mat)

    cv2.putText(image, "X: " + "{:7.2f}".format(euler_angle[0, 0]), (20, 20), 
        cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255), thickness=2)
    cv2.putText(image, "Y: " + "{:7.2f}".format(euler_angle[1, 0]), (20, 50), 
        cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), thickness=2)
    cv2.putText(image, "Z: " + "{:7.2f}".format(euler_angle[2, 0]), (20, 80), 
        cv2.FONT_HERSHEY_SIMPLEX, 0.75, (255, 0, 0), thickness=2)
    
    return rotation_vec, translation_vec

def reproject_axis(image, rotation_vec, translation_vec, cam_matrix, dist_coeffs):
    reprojectsrc = 0.5*axis_pts + (object_pts[4] + object_pts[5] + object_pts[6] + object_pts[7])/4 + np.float32([0, 0, 2])
    reprojectdst, _ = cv2.projectPoints(reprojectsrc, rotation_vec, translation_vec, cam_matrix, dist_coeffs)
    reprojectdst = tuple(map(tuple, reprojectdst.reshape(-1, 2)))
    cv2.line(image, reprojectdst[0], reprojectdst[1], (0, 0, 255), 2, cv2.LINE_AA)
    cv2.line(image, reprojectdst[0], reprojectdst[2], (0, 255, 0), 2, cv2.LINE_AA)
    cv2.line(image, reprojectdst[0], reprojectdst[3], (255, 0, 0), 2, cv2.LINE_AA)

def object_points_grad(pts_true, pts_pred, rotation_vec, translation_vec, cam_matrix, dist_coeffs):
    delta=1e-4
    f = np.float32([np.sqrt(d[0]*d[0]+d[1]*d[1]) for d in pts_true - pts_pred])
    g = np.zeros((f.shape[0], 3), dtype=np.float32)
    for i in range(3):
        pts = object_pts.copy()
        pts[:,i] += delta
        pred = cv2.projectPoints(pts, rotation_vec, translation_vec, cam_matrix, dist_coeffs)[0].reshape(-1, 2)
        d = np.float32([np.sqrt(d[0]*d[0]+d[1]*d[1]) for d in pts_true - pred])
        g[:,i] = (d - f)/delta
    return g

def camera_matrix_grad(pts_true, pts_pred):
    fx=cam_matrix[0,0]
    fy=cam_matrix[1,1]
    cx=cam_matrix[0,2]
    cy=cam_matrix[1,2]
    grad_fx =np.average((pts_pred[:,0] - pts_true[:,0])*(pts_pred[:,0]-cx)/fx)
    grad_fy =np.average((pts_pred[:,1] - pts_true[:,1])*(pts_pred[:,1]-cy)/fy)
    grad_cx = np.average(pts_pred[:,0] - pts_true[:,0])
    grad_cy = np.average(pts_pred[:,1] - pts_true[:,1])
    return grad_fx, grad_fy, grad_cx, grad_cy


def reproject_points(image, object_pts, pts_true, rotation_vec, translation_vec, cam_matrix, dist_coeffs):
    pts_pred = cv2.projectPoints(object_pts, rotation_vec, translation_vec, cam_matrix, dist_coeffs)[0].reshape(-1, 2)
    for (x, y) in pts_true:
        cv2.circle(image, (x, y), 1, (0, 0, 255), 2)
    project_err = np.average(abs(pts_true-pts_pred))
    print("reproject error: %.2f" % project_err)

    gfx, gfy, gcx, gcy = camera_matrix_grad(pts_true, pts_pred)
    gm = np.float32([
        [gfx, 0,   gcx],
        [0,   gfy, gcy],
        [0,    0,  0  ]])
    pts_fixed = cv2.projectPoints(object_pts, rotation_vec, translation_vec, cam_matrix - 0.01*gm, dist_coeffs)[0].reshape(-1, 2)
    fixed_err = np.average(abs(pts_true-pts_fixed))
    print("fixed camera error: %.2f" % fixed_err)
    if fixed_err < project_err:
        cam_matrix -=  0.01*gm
        print("fx=%f, fy=%f, cx=%f, cy=%f" % (cam_matrix[0,0], cam_matrix[1,1], cam_matrix[0,2], cam_matrix[1,2]))

    g = object_points_grad(pts_true, pts_pred, rotation_vec, translation_vec, cam_matrix, dist_coeffs)
    pts_fixed = cv2.projectPoints(object_pts - 0.01*g, rotation_vec, translation_vec, cam_matrix, dist_coeffs)[0].reshape(-1, 2)
    fixed_err = np.average(abs(pts_true-pts_fixed))
    print("fixed object error: %.2f" % fixed_err)
    if fixed_err < project_err:
        object_pts -=  0.01*g
    for (x, y) in pts_pred:
        cv2.circle(image, (x, y), 1, (0, 255, 0), 2)
    for (x, y) in pts_fixed:
        cv2.circle(image, (x, y), 1, (255, 0, 0), 2)
      
def main():
    # return
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Unable to connect to camera.")
        return
    detector = dlib.get_frontal_face_detector()
    predictor = dlib.shape_predictor(face_landmark_path)
    while cap.isOpened():
        ret, image = cap.read()
        if ret:
            face_rects = detector(image, 0)
            if len(face_rects) > 0:
                shape = predictor(image, face_rects[0])
                shape = shape_to_np(shape)
                image_pts = np.float32([shape[17], shape[21], shape[22], shape[26], shape[36],
                        shape[39], shape[42], shape[45], shape[31], shape[35],
                        shape[48], shape[54], shape[57], shape[8]])

                rotation_vec, translation_vec = get_head_pose(image, image_pts)
                reproject_points(image, object_pts, image_pts, rotation_vec, translation_vec, cam_matrix, dist_coeffs)
                reproject_axis(image, rotation_vec, translation_vec, cam_matrix, dist_coeffs)

            cv2.imshow("demo", image)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break


if __name__ == '__main__':
    main()
