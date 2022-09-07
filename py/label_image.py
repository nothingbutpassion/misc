import sys
import os
import cv2
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *

class ImageWindow(QWidget):
    bbox_created = pyqtSignal(list)

    def __init__(self):
        super().__init__()
        self.img = None
        self.bboxes = []
        self.bbox_index = -1
        self.mouse_pressed = False
        self.first_point = None
        self.second_point = None

    def set_file(self, file):
        img = cv2.imread(file, cv2.IMREAD_COLOR)
        self.img = QImage(img, img.shape[1], img.shape[0], img.shape[1]*img.shape[2], QImage.Format_BGR888)
        self.resize(QSize(img.shape[1], img.shape[0]))
        self.bbox_index = -1
        self.update()

    def set_bboxes(self, bboxes):
        self.bboxes = bboxes
        self.update()

    def select_bbox(self, index):
        self.bbox_index = index
        self.update()

    def check_bbox(self, bbox):
        if bbox and len(bbox) == 4:
            x1, y1, x2, y2 = bbox
            if (0 <= x1 and x1 < x2 and x2 < self.img.width() and
                0 <= y1 and y1 < y2 and y2 < self.img.height()):
                return True
        print(f"invalid bbox: {bbox}")
        return False

    def mousePressEvent(self, event):
        self.mouse_pressed = True
        self.first_point = (event.x(), event.y())

    def mouseMoveEvent(self, event):
        self.second_point = (event.x(), event.y())
        self.update()

    def mouseReleaseEvent(self, event):
        self.second_point = (event.x(), event.y())
        self.mouse_pressed = False
        x1, y1 = self.first_point
        x2, y2 = self.second_point
        if x1 > x2:
            x1, x2 = x2, x1
        if y1 > y2:
            y1, y2, = y2, y1
        rw, rh = self.img.width()/self.width(), self.img.height()/self.height()
        bbox = [rw*x1, rh*y1, rw*x2, rh*y2]
        if self.check_bbox(bbox):
            self.bbox_created.emit(bbox)
            print(f"bbox created: {bbox}")
        self.update()

    def paintEvent(self, event):
        if self.img is not None:
            p = QPainter()
            p.begin(self)
            p.setRenderHint(QPainter.Antialiasing)
            self.draw(p)
            p.end()

    def draw(self, p):
        p.drawImage(self.rect(), self.img)
        old_pen = p.pen()
        selected_pen = QPen(Qt.green)
        rw, rh = self.width()/self.img.width(), self.height()/self.img.height()
        for i, b in enumerate(self.bboxes):
            x1, y1, x2, y2 = rw*b[0], rh*b[1], rw*b[2], rh*b[3]
            if i == self.bbox_index and not self.mouse_pressed:
                p.setPen(selected_pen)
                p.drawRect(QRectF(x1, y1, x2 - x1, y2 - y1))
                p.setPen(old_pen)
            else:
                p.drawRect(QRectF(x1, y1, x2-x1, y2-y1))
        if self.mouse_pressed and self.first_point is not None and self.second_point is not None:
            x1, y1 = self.first_point
            x2, y2 = self.second_point
            if x1 > x2:
                x1, x2 = x2, x1
            if y1 > y2:
                y1, y2 = y2, y1
            p.setPen(selected_pen)
            p.drawRect(QRectF(x1, y1, x2-x1, y2-y1))
            p.setPen(old_pen)


class BBoxWindow(QListView):
    current_changed = pyqtSignal(int)
    bbox_updated = pyqtSignal(int, list)
    bbox_deleted = pyqtSignal(int)
    bbox_adjusted = pyqtSignal(int, str, bool)

    def __init__(self):
        super().__init__()
        self.model = QStringListModel()
        self.model.dataChanged.connect(self.data_change)
        self.setModel(self.model)

    def get_bboxes(self):
        return [self.to_bbox(s) for s in self.model.stringList()]

    def set_bboxes(self, bboxes):
        strlist = ["%6.1f, %6.1f, %6.1f, %6.1f"%(b[0], b[1], b[2], b[3]) for b in bboxes]
        self.model.setStringList(strlist)

    def to_bbox(self, txt):
        bbox = []
        try:
            cs = [float(s.strip()) for s in txt.split(',')]
            assert(len(cs) == 4)
            bbox = [cs[0], cs[1], cs[2], cs[3]]
        except Exception as e:
            print(f"exceptoin: {e}")
        return bbox

    def data_change(self, begin_index, end_index):
        assert(begin_index.row() == end_index.row())
        bbox_index = begin_index.row()
        bbox = self.to_bbox(self.model.stringList()[bbox_index])
        self.bbox_updated.emit(bbox_index, bbox)
        print(f"bbox updated: index={bbox_index}, bbox={bbox}")

    def set_current(self, row):
        self.setCurrentIndex(self.model.index(row))

    def get_current(self):
        return self.currentIndex().row()

    def currentChanged(self, cur_index, pre_index):
        print(f"bbox current changed: cur_index={cur_index.row()}, pre_index={pre_index.row()}")
        self.current_changed.emit(cur_index.row())

    def keyPressEvent(self, event):
        bbox_index = self.get_current()
        key = event.key()
        ctrl = True if event.modifiers() == Qt.ControlModifier else False
        if key == Qt.Key_Delete:
            self.bbox_deleted.emit(bbox_index)
        elif key == Qt.Key_Up:
            self.bbox_adjusted.emit(bbox_index, "up", ctrl)
        elif key == Qt.Key_Down:
            self.bbox_adjusted.emit(bbox_index, "down", ctrl)
        elif key == Qt.Key_Left:
            self.bbox_adjusted.emit(bbox_index, "left", ctrl)
        elif key == Qt.Key_Right:
            self.bbox_adjusted.emit(bbox_index, "right", ctrl)
        else:
            super().keyPressEvent(event)

class FileWindow(QListView):
    current_changed = pyqtSignal(int)

    def __init__(self):
        super().__init__()
        self.model = QStringListModel()
        self.setModel(self.model)
        self.setEditTriggers(QAbstractItemView.NoEditTriggers)

    def set_files(self, files):
        self.model.setStringList(files)

    def set_current(self, row):
        self.setCurrentIndex(self.model.index(row))

    def get_current(self):
        return self.currentIndex().row()

    def currentChanged(self, cur_index, pre_index):
        print(f"file current changed: cur_index={cur_index.row()}, pre_index={pre_index.row()}")
        self.current_changed.emit(cur_index.row())


class MainWindow(QWidget):
    def __init__(self, data):
        super().__init__()
        self.save_path = None
        self.data = data
        self.file_index = -1
        self.bbox_index = -1
        self.init_ui()

    def open(self):
        dir = QFileDialog.getExistingDirectory()
        if dir and self.data.open(dir):
            self.save_path = None
            self.bbox_index = -1
            self.file_index = 0
            self.img_wnd.set_file(self.data.get_file(self.file_index))
            self.img_wnd.set_bboxes(self.data.get_bboxes(self.file_index))
            self.file_wnd.set_files(self.data.get_files())
            self.file_wnd.set_current(self.file_index)
            print(f"{dir} is opened")

    def save(self):
        if self.save_path is not None:
            filename = self.save_path
        else:
            filename, _ = QFileDialog.getSaveFileName()
        if filename and self.data.save(filename):
            print(f"{filename} is saved")

    def load(self):
        filename, type = QFileDialog.getOpenFileName()
        if not filename:
            return
        if self.data.load(filename):
            self.save_path = filename
            self.bbox_index = -1
            self.file_index = 0
            self.img_wnd.set_file(self.data.get_file(self.file_index))
            self.img_wnd.set_bboxes(self.data.get_bboxes(self.file_index))
            self.file_wnd.set_files(self.data.get_files())
            self.file_wnd.set_current(self.file_index)
            print(f"{filename} is loaded")

    def prev(self):
        if 0 <= self.file_index and self.file_index < self.data.get_count():
            self.file_index = self.data.get_count() - 1 if self.file_index == 0 else self.file_index - 1
            self.file_wnd.set_current(self.file_index)

    def next(self):
        if 0 <= self.file_index and self.file_index < self.data.get_count():
            self.file_index = 0 if self.file_index == self.data.get_count() - 1 else self.file_index + 1
            self.file_wnd.set_current(self.file_index)

    def zoom_in(self):
        self.img_wnd.resize(self.img_wnd.size()*1.1)

    def zoom_out(self):
        s = self.img_wnd.size()
        self.img_wnd.resize(self.img_wnd.size()*0.9)

    def select_file(self, cur_index):
        self.file_index = cur_index
        self.img_wnd.set_file(self.data.get_file(self.file_index))
        self.img_wnd.set_bboxes(self.data.get_bboxes(self.file_index))
        self.bbox_wnd.set_bboxes(self.data.get_bboxes(self.file_index))

    def select_bbox(self, cur_index):
        self.bbox_index = cur_index
        self.img_wnd.select_bbox(self.bbox_index)

    def update_bbox(self, index, bbox):
        bboxes = self.data.get_bboxes(self.file_index)
        if self.img_wnd.check_bbox(bbox):
            bboxes[index] = bbox
        else:
            self.bbox_wnd.set_bboxes(bboxes)
        self.img_wnd.set_bboxes(bboxes)

    def create_bbox(self, bbox):
        bboxes = self.data.get_bboxes(self.file_index)
        bboxes.append(bbox)
        self.bbox_index = len(bboxes) - 1
        self.bbox_wnd.set_bboxes(bboxes)
        self.bbox_wnd.set_current(self.bbox_index)
        self.img_wnd.set_bboxes(bboxes)
        self.img_wnd.select_bbox(self.bbox_index)

    def delete_bbox(self, index):
        bboxes = self.data.get_bboxes(self.file_index)
        del bboxes[index]
        self.bbox_index = -1
        self.bbox_wnd.set_bboxes(bboxes)
        self.img_wnd.set_bboxes(bboxes)
        self.img_wnd.select_bbox(self.bbox_index)

    def adjust_bbox(self, index, direction, ctrl):
        bboxes = self.data.get_bboxes(self.file_index)
        x1, y1, x2, y2 = bboxes[index]
        delta = -1 if ctrl else 1;
        if direction == "left":
            x1 -= delta
        elif direction == "right":
            x2 += delta
        elif direction == "up":
            y1 -= delta
        elif direction == "down":
            y2 += delta
        if self.img_wnd.check_bbox([x1, y1, x2, y2]):
            bboxes[index] = [x1, y1, x2, y2]
            self.img_wnd.set_bboxes(bboxes)
            self.bbox_wnd.set_bboxes(bboxes)

    def init_ui(self):
        self.open_btn = QPushButton("Open")
        self.open_btn.clicked.connect(self.open)
        self.save_btn = QPushButton("Save")
        self.save_btn.clicked.connect(self.save)
        self.load_btn = QPushButton("Load")
        self.load_btn.clicked.connect(self.load)
        self.prev_btn = QPushButton("<< Prev")
        self.prev_btn.clicked.connect(self.prev)
        self.next_btn = QPushButton("Next>>")
        self.next_btn.clicked.connect(self.next)
        self.zoom_in_btn = QPushButton("Zoom in")
        self.zoom_in_btn.clicked.connect(self.zoom_in)
        self.zoom_out_btn = QPushButton("Zoom out")
        self.zoom_out_btn.clicked.connect(self.zoom_out)
        self.img_wnd  = ImageWindow()
        self.img_wnd.bbox_created.connect(self.create_bbox)
        self.bbox_wnd = BBoxWindow()
        self.bbox_wnd.current_changed.connect(self.select_bbox)
        self.bbox_wnd.bbox_updated.connect(self.update_bbox)
        self.bbox_wnd.bbox_deleted.connect(self.delete_bbox)
        self.bbox_wnd.bbox_adjusted.connect(self.adjust_bbox)
        self.file_wnd = FileWindow()
        self.file_wnd.current_changed.connect(self.select_file)
        top = QHBoxLayout()
        top.addWidget(self.open_btn, 0, Qt.AlignLeft)
        top.addWidget(self.save_btn, 0, Qt.AlignLeft)
        top.addWidget(self.load_btn, 0, Qt.AlignLeft)
        top.addWidget(self.prev_btn, 0, Qt.AlignLeft)
        top.addWidget(self.next_btn, 0, Qt.AlignLeft)
        top.addWidget(self.zoom_in_btn, 0, Qt.AlignLeft)
        top.addWidget(self.zoom_out_btn, 0, Qt.AlignLeft)
        top.addStretch(1)
        right = QSplitter(Qt.Vertical)
        right.addWidget(self.bbox_wnd)
        right.addWidget(self.file_wnd)
        right.setStretchFactor(1, 1)
        left = QScrollArea()
        left.setBackgroundRole(QPalette.Dark)
        left.setWidget(self.img_wnd)
        bottom = QSplitter(Qt.Horizontal)
        bottom.addWidget(left)
        bottom.addWidget(right)
        bottom.setStretchFactor(0, 1)
        vbox = QVBoxLayout()
        vbox.addLayout(top, 0)
        vbox.addWidget(bottom, 1)
        self.setLayout(vbox)
        self.setWindowTitle("Lable image for DMS (www.hangsheng.com.cn)")
        self.setGeometry(200, 200, 1080, 720)

class Dataset(object):
    def __init__(self):
        self.dir = "."
        self.images = []

    def open(self, dir):
        files = [f for f in os.listdir(dir) if os.path.splitext(f)[1].lower() in (".jpg", ".jpeg", ".png", ".jfif")]
        img_files = [f for f in files if cv2.imread(f"{dir}/{f}") is not None]
        if len(img_files) > 0:
            self.dir = dir
            self.images = [{"file":f,"boxes":[]} for f in img_files]
            print(f"{len(self.images)}  files found in {dir}")
            return True
        return False

    def save(self, path):
        if len(self.images) > 0:
            lines = [self.dir]
            for im in self.images:
                file = im["file"]
                bboxes = " ".join(["%.1f,%.1f,%.1f,%.1f"%(b[0], b[1], b[2], b[3]) for b in im["boxes"]])
                line = ": ".join([file, bboxes]) if len(bboxes) > 0 else file
                lines.append("\n" + line)
            with open(path, "w") as f:
                f.writelines(lines)
            return True
        return False

    def load(self, path):
        try:
            lines = open(path, "r").readlines()
            assert(len(lines) > 1)
            dir = lines[0].strip()
            assert(os.path.isdir(dir))
            images = []
            for line in lines[1:]:
                ls = line.split(": ")
                file = ls[0].strip()
                assert (cv2.imread(dir+"/"+file) is not None)
                bboxes = []
                if len(ls) > 1:
                    bs = ls[1].strip().split(" ")
                    for s in bs:
                        bbox = [float(b.strip()) for b in s.split(',')]
                        assert(len(bbox) == 4)
                        bboxes.append(bbox)
                images.append({"file":file, "boxes":bboxes})
            assert(len(images) > 0)
            self.dir = dir
            self.images = images
            print(f"load {path} ok")
            return True
        except Exception as e:
            print(f"load {path} failed: {e}")
        return False

    def get_count(self):
        return len(self.images)

    def get_files(self):
        return [img["file"] for img in self.images]

    def get_file(self, index):
        if 0 <= index and index < len(self.images):
            return self.dir + "/" + self.images[index]["file"]
        return None

    def get_bboxes(self, index):
        if 0 <= index and index < len(self.images):
            return self.images[index]["boxes"]
        return []

    def set_bboxes(self, index, bboxes):
        if 0 <= index and index < len(self.images):
            self.images[index]["boxes"] = bboxes


if __name__ == "__main__":
    app = QApplication(sys.argv)
    data = Dataset()
    window = MainWindow(data)
    window.show()
    sys.exit(app.exec())