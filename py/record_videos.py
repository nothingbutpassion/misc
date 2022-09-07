import sys
import os
import cv2
import threading
from pathlib import Path
from PyQt5.QtWidgets import QApplication, QHBoxLayout, QWidget, QMessageBox, QPushButton, QLabel, QComboBox, QLineEdit
from PyQt5.QtCore import Qt

record_name = "record"
record_cmd = None

def init_recorders():
	recorders = []
	for i in range(4):
		c = cv2.VideoCapture(i)
		if not c.isOpened():
			return recorders
		size = int(c.get(cv2.CAP_PROP_FRAME_WIDTH)), int(c.get(cv2.CAP_PROP_FRAME_HEIGHT))
		fps = c.get(cv2.CAP_PROP_FPS)
		recorders.append((i, c, size, fps, None))
		print(f"init camera {i} ok")
	return recorders

def exit(recorders):
	for i, c, size, fps, writer in recorders:
		if writer is not None:
			writer.release()
			print(f"recording camera {i} finished")
		c.release()
		print(f"release camera {i} finished")

def start(recorders):
	global record_cmd, record_name
	for i, c, size, fps, writer in recorders:
		if writer is None:
			format = cv2.VideoWriter.fourcc('m', 'p', '4', 'v')
			outfile = record_name + f"_{i}.mp4"
			writer = cv2.VideoWriter(outfile, format, fps, size, True)
			recorders[i] = (i, c, size, fps, writer)
			print(f"start recording camera {i} to {outfile}")

def stop(recorders):
	global record_cmd
	for i, c, size, fps, writer in recorders:
		if writer is not None:
			writer.release()
			recorders[i] = (i, c, size, fps, None)
			print(f"recording camera {i} finished")


def record():
	global record_cmd, record_name
	recorders = init_recorders()
	while True:
		if record_cmd == "exit":
			exit(recorders)
			return
		if record_cmd == "start":
			stop(recorders)
			start(recorders)
		elif record_cmd == "stop":
			stop(recorders)
		record_cmd = None
		for i, c, size, fps, writer in recorders:
			ok, img = c.read()
			if ok:
				if writer is not None:
					writer.write(img)
				cv2.imshow(f"camera {i}", img)
			else:
				print(f"can't read frame from camera {i}")
		cv2.waitKey(3)


class MainWindow(QWidget):
	def __init__(self, outdir):
		super().__init__()
		self.outdir = outdir
		self.init_ui()
	def init_ui(self):
		self.person_lable = QLabel("Person:")
		self.person_id = QLineEdit("0000")
		self.action_cbx = QComboBox()
		for action in ["Smoking", "Calling", "Blinking", "Distracting"]:
			self.action_cbx.addItem(action)
		self.start_btn = QPushButton("Start")
		self.start_btn.clicked.connect(self.start)
		self.stop_btn = QPushButton("Stop")
		self.stop_btn.clicked.connect(self.stop)
		top = QHBoxLayout()
		top.addWidget(self.person_lable, 0, Qt.AlignLeft)
		top.addWidget(self.person_id, 0, Qt.AlignLeft)
		top.addWidget(self.action_cbx, 0, Qt.AlignLeft)
		top.addWidget(self.start_btn, 0, Qt.AlignLeft)
		top.addWidget(self.stop_btn, 0, Qt.AlignLeft)
		self.setLayout(top)
		self.setWindowTitle(" DMS Recorder")
		self.setGeometry(320, 320, 450, 50)
		self.setFixedHeight(50)
	def start(self):
		global record_cmd, record_name
		prefix = Path(self.outdir).as_posix() + "/" + self.person_id.text()	+ "_" +  self.action_cbx.currentText()
		filepath = Path(prefix + "_0.mp4")
		if filepath.exists():
			reply = QMessageBox.information(self, 'Warning',
											f'{filepath.absolute()} already exists, overwrite it!',
											QMessageBox.Yes | QMessageBox.No,
											QMessageBox.No)
			if reply == QMessageBox.Yes:
				record_name = prefix
				record_cmd = "start"
		else:
			record_name = prefix
			record_cmd = "start"
	def stop(self):
		global record_cmd
		record_cmd = "stop"
	def closeEvent(self, event):
		global record_cmd
		record_cmd = "exit"

def run_app(argv, outdir):
	app = QApplication(argv)
	window = MainWindow(outdir)
	window.show()
	app.exec()

if __name__ == "__main__":
	outdir = sys.argv[1] if len(sys.argv) == 2 else os.curdir
	ui_thread = threading.Thread(target=run_app, args=(sys.argv, outdir))
	ui_thread.start()
	record()
	ui_thread.join()