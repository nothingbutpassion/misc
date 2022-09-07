import sys
import os
import cv2
from multiprocessing import Process, Queue
from pathlib import Path
from PyQt5.QtWidgets import QWidget, QApplication, QLabel, QHBoxLayout, QLineEdit, QComboBox, QMessageBox, QPushButton
from PyQt5.QtCore import Qt

def record(i, q):
	c = cv2.VideoCapture(i)
	if not c.isOpened():
		print(f"can't open camera {i}")
		return
	print(f"init camera {i} ok")
	cv2.namedWindow(f"camera {i}", cv2.WINDOW_NORMAL)
	writer = None
	while True:
		if q.qsize() > 0:
			cmds = q.get()
			cmds = cmds.split()
			cmd = cmds[0]
			if cmd == "start" and len(cmds) == 2:
				outfile = f"{cmds[1]}_{i}.mp4"
				format = cv2.VideoWriter.fourcc('m', 'p', '4', 'v')
				fps = c.get(cv2.CAP_PROP_FPS)
				size = int(c.get(cv2.CAP_PROP_FRAME_WIDTH)), int(c.get(cv2.CAP_PROP_FRAME_HEIGHT))
				if writer is not None:
					writer.release()
					print(f"recording camera {i} finished")
				writer = cv2.VideoWriter(outfile, format, fps, size, True)
				if not writer.isOpened():
					print(f"can't create {outfile}")
					writer = None
					break
				print(f"start recording camera {i} to {outfile}")
			elif cmd == "stop":
				if writer is not None:
					writer.release()
					writer = None
					print(f"recording camera {i} finished")
			elif cmd == "exit":
				if writer is not None:
					writer.release()
					print(f"recording camera {i} finished")
				break
		ok, img = c.read()
		if not ok:
			print(f"read frame from camera {i} failed")
			break
		if writer is not None:
			writer.write(img)
		cv2.imshow(f"camera {i}", img)
		if cv2.waitKey(3) == ord('q'):
			break
	c.release()
	print(f"camera {i} released")

def find_cameras():
	cameras = []
	for i in range(4):
		c = cv2.VideoCapture(i)
		if not c.isOpened():
			continue
		c.release()
		cameras.append(i)
	return cameras

class MainWindow(QWidget):
	def __init__(self, outdir, cameras):
		super().__init__()
		self.outdir = outdir
		self.init_ui()
		self.record_processes = []
		for i in cameras:
			q = Queue()
			p = Process(target=record, args=(i, q))
			p.start()
			self.record_processes.append((p, q))
	def closeEvent(self, event):
		self.send_cmds("exit")
		for p, q in self.record_processes:
			p.join()
			q.close()
	def init_ui(self):
		self.person_label = QLabel("Person:")
		self.person_text = QLineEdit("0000")
		self.action_cbox = QComboBox(self)
		for action in ["Smoke", "Call", "Blink", "Distract", "Yawn"]:
			self.action_cbox.addItem(action)
		self.start_btn = QPushButton("Start")
		self.start_btn.clicked.connect(self.start)
		self.stop_btn = QPushButton("Stop")
		self.stop_btn.clicked.connect(self.stop)
		top = QHBoxLayout()
		top.addWidget(self.person_label, 0, Qt.AlignLeft)
		top.addWidget(self.person_text, 0, Qt.AlignLeft)
		top.addWidget(self.action_cbox, 0, Qt.AlignLeft)
		top.addWidget(self.start_btn, 0, Qt.AlignLeft)
		top.addWidget(self.stop_btn, 0, Qt.AlignLeft)
		top.addStretch(1)
		self.setLayout(top)
		self.setWindowTitle("DMS Recorder")
		self.setGeometry(320, 320, 450, 50)
		self.setFixedHeight(50)

	def start(self):
		file_prefix = Path(self.outdir).as_posix() + "/" + self.person_text.text() + "_" + self.action_cbox.currentText().lower()
		record_file = file_prefix + "_0.mp4"
		if Path(record_file).exists():
			reply = QMessageBox.question(self, 'Warning', f"{record_file} exists, overwrite it?",
										 QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
			if reply == QMessageBox.Yes:
				self.send_cmds(f"start {file_prefix}")
		else:
			self.send_cmds(f"start {file_prefix}")

	def stop(self):
		self.send_cmds("stop")

	def send_cmds(self, cmds):
		for p, q in self.record_processes:
			if p.is_alive():
				q.put(cmds)

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print(f"Usage: {sys.argv[0]} [out-dir]")
		sys.exit(-1)
	outdir = sys.argv[1] if len(sys.argv) > 1 else os.curdir
	cameras = find_cameras()
	if len(cameras) == 0:
		print("no video capture devices found")
		sys.exit(-1)
	app = QApplication(sys.argv)
	wnd = MainWindow(outdir, cameras)
	wnd.show()
	sys.exit(app.exec())

