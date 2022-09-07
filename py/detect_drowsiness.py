from collections import OrderedDict
import numpy as np
import argparse
import dlib
import cv2
import os

# define a dictionary that maps the indexes of the facial
# landmarks to specific face regions

#For dlib’s 68-point facial landmark detector:
FACIAL_LANDMARKS_68_IDXS = OrderedDict([
	("mouth", (48, 68)),
	("inner_mouth", (60, 68)),
	("right_eyebrow", (17, 22)),
	("left_eyebrow", (22, 27)),
	("right_eye", (36, 42)),
	("left_eye", (42, 48)),
	("nose", (27, 36)),
	("jaw", (0, 17))
])

EYE_AR_THRESH = 0.15
EYE_AR_CONSEC_FRAMES = 5
# initialize the frame counter as well as a boolean used to
# indicate if the alarm is going off
COUNTER = 0
MAX_EAR = 0.2

def euclidean_dist(ptA, ptB):
	# compute and return the euclidean distance between the two
	# points
	return np.linalg.norm(ptA - ptB)

def eye_aspect_ratio(eye):
	# compute the euclidean distances between the two sets of
	# vertical eye landmarks (x, y)-coordinates
	A = euclidean_dist(eye[1], eye[5])
	B = euclidean_dist(eye[2], eye[4])
	# compute the euclidean distance between the horizontal
	# eye landmark (x, y)-coordinates
	C = euclidean_dist(eye[0], eye[3])
	# compute the eye aspect ratio
	ear = (A + B) / (2.0 * C)
	# return the eye aspect ratio
	return ear

def shape_to_np(shape, dtype="int"):
	# initialize the list of (x, y)-coordinates
	coords = np.zeros((shape.num_parts, 2), dtype=dtype)

	# loop over all facial landmarks and convert them
	# to a 2-tuple of (x, y)-coordinates
	for i in range(0, shape.num_parts):
		coords[i] = (shape.part(i).x, shape.part(i).y)

	# return the list of (x, y)-coordinates
	return coords

if __name__ == "__main__":
    detector = dlib.get_frontal_face_detector()
    predictor = dlib.shape_predictor(
        os.path.dirname(os.path.abspath(__file__)) + "/shape_predictor_68_face_landmarks.dat")
    vc = cv2.VideoCapture(0)
    while vc.isOpened():
        ret, img = vc.read()
        if not ret:
            break
        boxes = detector(img)
        if len(boxes) == 0:
            continue
        rect = boxes[0]
        shape = predictor(img, rect)
        shape = shape_to_np(shape)
        (lStart, lEnd) = FACIAL_LANDMARKS_68_IDXS["left_eye"]
        (rStart, rEnd) = FACIAL_LANDMARKS_68_IDXS["right_eye"]
        leftEye = shape[lStart:lEnd]
        rightEye = shape[rStart:rEnd]
        leftEAR = eye_aspect_ratio(leftEye)
        rightEAR = eye_aspect_ratio(rightEye)
        ear = (leftEAR + rightEAR) / 2.0
        MAX_EAR = max(MAX_EAR, ear)
        # check to see if the eye aspect ratio is below the blink
        # threshold, and if so, increment the blink frame counter
        # if ear < EYE_AR_THRESH:
        if ear/MAX_EAR < 0.4 or ear < EYE_AR_THRESH:
            COUNTER += 1
            # if the eyes were closed for a sufficient number of
            # frames, then sound the alarm
            if COUNTER >= EYE_AR_CONSEC_FRAMES:
                # draw an alarm on the frame
                cv2.putText(img, "DROWSINESS ALERT!", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            else:
                cv2.putText(img, f"COUNTER: {COUNTER}", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        # otherwise, the eye aspect ratio is not below the blink
        # threshold, so reset the counter and alarm
        else:
            COUNTER = 0
        cv2.putText(img, "EAR: {:.3f} RAR: {:.3f}".format(ear, ear/MAX_EAR), (300, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        # show the frame
        cv2.imshow("Frame", img)
        key = cv2.waitKey(1) & 0xFF
        # if the `q` key was pressed, break from the loop
        if key == ord("q"):
            break