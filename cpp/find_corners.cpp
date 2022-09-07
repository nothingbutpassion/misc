#include <stack>
#include <algorithm>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "find_corners.h"
#include "avm_calib.h"
#include "log.h"

using namespace std;
using namespace cv;

struct Quad {
	int count = 0;
	int group_id = -1;
	Point2f corners[4];
	Quad* neighbors[4] = { nullptr, nullptr, nullptr, nullptr };
};

void generateQuads(const Mat& image, vector<Quad>& quads) {
	std::vector<std::vector<Point>> contours;
	std::vector<Vec4i> hierarchy;
	findContours(image, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
	if (contours.empty()) {
		LOGE("findContours returns no contours");
		return;
	}
	vector<Quad> cadidates;
	std::vector<int> contour_child_counter(contours.size(), 0);
	int boardIdx = -1;
	for (int idx = (int)(contours.size() - 1); idx >= 0; --idx) {
		// holes only (no child contours and with parent)
		int parentIdx = hierarchy[idx][3];
		if (hierarchy[idx][2] != -1 || parentIdx == -1)
			continue;
		// area shouldn't be too small
		const std::vector<Point>& contour = contours[idx];
		if (contourArea(contour) < 121)
			continue;
		std::vector<Point> approx_contour;
		for (float approx_level = 1.f; approx_level < 6.f; approx_level += 1.f)
		{
			approxPolyDP(contour, approx_contour, approx_level, true);
			if (approx_contour.size() == 4)
				break;
			// we call this again on its own output, because sometimes
			// approxPoly() does not simplify as much as it should.
			std::vector<Point> approx_contour_tmp;
			std::swap(approx_contour, approx_contour_tmp);
			approxPolyDP(approx_contour_tmp, approx_contour, approx_level, true);
			if (approx_contour.size() == 4)
				break;
		}
		// reject non-quadrangles
		if (approx_contour.size() != 4)
			continue;
		if (!cv::isContourConvex(approx_contour))
			continue;
		// filter with geometric constraints
		cv::Point pt[4];
		for (int i = 0; i < 4; ++i)
			pt[i] = approx_contour[i];
		double p = cv::arcLength(approx_contour, true);
		double area = cv::contourArea(approx_contour, false);
		double d1 = sqrt(normL2Sqr<double>(pt[0] - pt[2]));
		double d2 = sqrt(normL2Sqr<double>(pt[1] - pt[3]));
		double d3 = sqrt(normL2Sqr<double>(pt[0] - pt[1]));
		double d4 = sqrt(normL2Sqr<double>(pt[1] - pt[2]));
		double d5 = sqrt(normL2Sqr<double>(pt[2] - pt[3]));
		double d6 = sqrt(normL2Sqr<double>(pt[3] - pt[0]));
		if (!(5 * d3 > d4 && 5 * d4 > d3 && d3 * d4 < 1.5 * area && d1 > 0.15 * p && d2 > 0.15 * p))
			continue;
		if (!(1.5 * d3 > d5 && 1.5 * d5 > d3 && 1.5 * d4 > d6 && 1.5 * d6 > d4))
			continue;
		contour_child_counter[parentIdx]++;
		if (boardIdx != parentIdx && (boardIdx < 0 || contour_child_counter[boardIdx] < contour_child_counter[parentIdx]))
			boardIdx = parentIdx;
		Quad quad;
		for (int i = 0; i < 4; ++i)
			quad.corners[i] = Point2f(pt[i].x, pt[i].y);
		quad.group_id = parentIdx;
		cadidates.push_back(quad);
	}
	for (auto& q : cadidates) {
		if (q.group_id == boardIdx) {
			q.group_id = -1;
			quads.push_back(q);
		}
	}
}

vector<Quad> orderQuads(vector<Quad>& quads) {
	vector<Quad> result;
	for (auto& q : quads) {
		bool sorted = false;
		for (int i = 0; i < 4; ++i) {
			Point2f p0 = q.corners[0];
			Point2f p1 = q.corners[1];
			Point2f p2 = q.corners[2];
			Point2f p3 = q.corners[3];
			Point2f c = 0.25*(p0 + p1 + p2 + p3);
			if (p0.x < p1.x && p0.y < c.y && p1.y < c.y && p2.x > p3.x && p2.y > c.y && p3.y > c.y) {
				sorted = true;
				break;
			}
			q.corners[0] = p1;
			q.corners[1] = p2;
			q.corners[2] = p3;
			q.corners[3] = p0;
		}
		if (sorted)
			result.push_back(q);
	}
	return result;
}

void findQuadNeighbors(vector<Quad>& quads) {
	for (int idx = 0; idx < quads.size(); ++idx) {
		Quad& cur_quad = quads[idx];
		// for each corner of this quadrangle
		for (int i = 0; i < 4; i++) {
			if (cur_quad.neighbors[i])
				continue;
			float min_dist = 1111111111.f;
			int closest_corner_idx = -1;
			Quad* closest_quad = nullptr;
			cv::Point2f pt = cur_quad.corners[i];
			// find the closest corner in all other quadrangles
			for (int k = 0; k < quads.size(); k++) {
				if (k == idx)
					continue;
				Quad& q_k = quads[k];
				for (int j = 0; j < 4; j++) {
					if (q_k.neighbors[j])
						continue;
					float dist = normL2Sqr<float>(pt - q_k.corners[j]);
					if (dist < min_dist && dist < 121) {
						closest_corner_idx = j;
						closest_quad = &q_k;
						min_dist = dist;
					}
				}
			}
			// we found a matching corner point?
			if (closest_corner_idx >= 0) {
				if (cur_quad.count >= 4 || closest_quad->count >= 4)
					continue;
				// If another point from our current quad is closer to the found corner
				// than the current one, then we don't count this one after all.
				// This is necessary to support small squares where otherwise the wrong
				// corner will get matched to closest_quad;
				Point2f& closest_corner = closest_quad->corners[closest_corner_idx];
				int j = 0;
				for (; j < 4; j++) {
					if (cur_quad.neighbors[j] == closest_quad)
						break;
					if (normL2Sqr<float>(closest_corner - cur_quad.corners[j]) < min_dist)
						break;
				}
				if (j < 4)
					continue;
				// Check that each corner is a neighbor of different quads
				for (j = 0; j < closest_quad->count; j++) {
					if (closest_quad->neighbors[j] == &cur_quad)
						break;
				}
				if (j < closest_quad->count)
					continue;
				// check whether the closest corner to closest_corner
				// is different from cur_quad->corners[i]->pt
				for (j = 0; j < quads.size(); j++) {
					Quad* q = &quads[i];
					if (j == idx || q == closest_quad)
						continue;
					int k = 0;
					for (; k < 4; k++) {
						if (!q->neighbors[k]) {
							if (normL2Sqr<float>(closest_corner - q->corners[k]) < min_dist)
								break;
						}
					}
					if (k < 4)
						break;
				}
				if (j < quads.size())
					continue;
				// We've found one more corner - remember it
				closest_corner = (pt + closest_corner) * 0.5f;
				cur_quad.count++;
				cur_quad.neighbors[i] = closest_quad;
				cur_quad.corners[i] = closest_corner;
				closest_quad->count++;
				closest_quad->neighbors[closest_corner_idx] = &cur_quad;
			}
		}
	}
}

void findConnectedQuads(vector<Quad>& quads, vector<Quad*>& out_group, int group_id) {
	out_group.clear();
	std::stack<Quad*> stack;
	int i = 0;
	for (; i < quads.size(); i++) {
		Quad* q = (Quad*)&quads[i];
		// Scan the array for a first unlabeled quad
		if (q->count <= 0 || q->group_id >= 0)
			continue;
		// Recursively find a group of connected quads
		stack.push(q);
		out_group.push_back(q);
		q->group_id = group_id;
		while (!stack.empty()) {
			q = stack.top();
			stack.pop();
			for (int k = 0; k < 4; k++) {
				Quad* neighbor = q->neighbors[k];
				if (neighbor && neighbor->count > 0 && neighbor->group_id < 0){
					stack.push(neighbor);
					out_group.push_back(neighbor);
					neighbor->group_id = group_id;
				}
			}
		}
		break;
	}
}

void mergeQuads(vector<Quad>& quads, const vector<Quad>& last_quads) {
	for (auto& q1 : quads) {
		for (auto q2 : last_quads) {
			int i = 0;
			for (i = 0; i < 4; ++i) {
				vector<Point2f> contour(q2.corners, q2.corners + 4);
				// It returns positive (inside), negative (outside), or zero (on an edge) value, correspondingly.
				// When measureDist = false, the return value is + 1, -1, and 0, respectively.
				double r = pointPolygonTest(contour, q1.corners[i], false);
				if (r < 0)
					break;
			}
			if (i < 4)
				continue;
			for (int i = 0; i < 4; ++i)
				q1.corners[i] = q2.corners[i];
			break;
		}
	}
}

bool getCorners(const std::string& lable, vector<vector<Quad*>>& groups, vector<Point2f>& corners) {
	sort(groups.begin(), groups.end(), [](const vector<Quad*>& g1, const vector<Quad*>& g2) {
		return g1[0]->corners[0].x < g2[0]->corners[0].x;
	});
	vector<Point2f> top;
	vector<Point2f> middle;
	vector<Point2f> bottom;
	if (lable == "left" || lable == "right") {
		for (int i = 0; i < 5; ++i) {
			auto& g = groups[i];
			// FIXME: exclude some exceptional ones if there're more than 2 quads
			if (g.size() != 2)
				return false;
			sort(g.begin(), g.end(), [](const Quad* q1, const Quad* q2) {
				return q1->corners[0].y < q2->corners[0].y;
			});
			if (i == 0 || i == 4) {
				middle.push_back(g[0]->corners[3]);
			} else {
				middle.push_back(g[0]->corners[2]);
				top.push_back(g[0]->corners[1]);
				bottom.push_back(g[1]->corners[3]);
			}
		}
	} else /*if (lable == "front" || lable == "rear")*/ {
		for (auto& g : groups) {
			sort(g.begin(), g.end(), [](const Quad* q1, const Quad* q2) {
				return q1->corners[0].y < q2->corners[0].y;
			});
		}
		vector<Quad*>& g1 = groups[0];
		vector<Quad*>& g2 = groups[1];
		vector<Quad*>& g3 = groups[2];
		if (g1.size() != 2 || g2.size() != 4 || g3.size() != 2)
			return false;
		sort(g2.begin(), g2.begin()+2, [](const Quad* q1, const Quad* q2) {
			return q1->corners[0].x < q2->corners[0].x;
		});
		sort(g2.begin()+2, g2.begin()+4, [](const Quad* q1, const Quad* q2) {
			return q1->corners[0].x < q2->corners[0].x;
		});
		middle = vector<Point2f>{
			g1[0]->corners[2],
			g2[0]->corners[2],
			g2[1]->corners[3],
			g2[1]->corners[2],
			g3[0]->corners[2]
		};
		top = vector<Point2f>{
			g2[0]->corners[1],
			g2[1]->corners[0],
			g2[1]->corners[1]
		};
		bottom = vector<Point2f>{
			g2[2]->corners[3],
			g2[2]->corners[2],
			g2[3]->corners[3]
		};
	}
	corners.clear();
	corners.insert(corners.end(), middle.begin(), middle.end());
	corners.insert(corners.end(), top.begin(), top.end());
	corners.insert(corners.end(), bottom.begin(), bottom.end());
	return true;
}

void showQuads(const Mat& image, const std::string& lable, const vector<Quad>& quads) {
	Mat img;
	image.copyTo(img);
	int i = 0;
	for (auto q : quads) {
		vector<vector<Point>> contours = { vector<Point>(q.corners, q.corners + 4) };
		drawContours(img, contours, -1, CV_RGB(255, 0, 0), 1, LINE_AA);
		putText(img, to_string(i++), q.corners[0], FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(0, 255, 0), 2);
	}
	imshow(lable, img);
	waitKey(0);
}

void drawCorners(const Mat& image, const std::string& lable, const vector<Point2f>& corners) {
	Mat img;
	image.copyTo(img);
	int i = 0;
	for (auto c : corners) {
		drawMarker(img, c, CV_RGB(255, 0, 0));
		putText(img, to_string(i++), Point(c.x + 4, c.y - 4), FONT_HERSHEY_COMPLEX, 0.7, CV_RGB(0, 255, 0), 2);
	}
	imshow(lable, img);
	waitKey(0);
}

bool findBoardCorners(const Mat& image, const std::string& lable, vector<Point2f>& corners) {
	Mat gray;
	cvtColor(image, gray, COLOR_BGR2GRAY);
	// FIXME: extract roi based on camera pose
	int x = image.cols / 16;
	int y = 3 * image.rows / 8;
	int w = 7 * image.cols / 8;
	int h = 3 * image.rows / 8;
	Mat equalized;
	Mat binary;
	equalizeHist(gray(Rect(x, y, w, h)), equalized);
	threshold(equalized, binary, 128, 255, THRESH_BINARY);
	vector<Quad> last_quads;
	for (int dilations = 0; dilations < 5;  ++dilations) {
		dilate(binary, binary, Mat(), Point(-1, -1), 1);
		rectangle(binary, Point(0, 0), Point(binary.cols - 1, binary.rows - 1), Scalar(255, 255, 255), 3, LINE_8);
		vector<Quad> quads;
		generateQuads(binary, quads);
		quads = orderQuads(quads);
		if (last_quads.size() > 0)
			mergeQuads(quads, last_quads);
		last_quads = quads;
		if ((lable == "left" || lable == "right") && quads.size() < 10)
			continue;
		if ((lable == "front" || lable == "rear") && quads.size() < 8)
			continue;
		findQuadNeighbors(quads);
		const int max_groups = quads.size();
		vector<vector<Quad*>> groups;
		for (int group_id = 0; group_id < max_groups; group_id++) {
			vector<Quad*> group;
			findConnectedQuads(quads, group, group_id);
			if (group.size() < 2)
				continue;
			groups.push_back(group);
		}
		if ((lable == "left" || lable == "right") && groups.size() != 5)
			continue;
		if ((lable == "front" || lable == "rear") && groups.size() != 3)
			continue;
		if (getCorners(lable, groups, corners)) {
			cornerSubPix(equalized, corners, Size(9, 9), Size(-1, -1), 
				TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 30, 0.1));
			for (auto& c : corners) {
				c.x += x;
				c.y += y;
			}
			//drawCorners(image, lable, corners);
			return true;
		}
	}
	return false;
}


bool findCorners(int calibType, int cameraId, const Mat& image, vector<Point2f>& corners) {
	if (calibType != FACTORY_CALIB)
		return false;
	if (cameraId < 0 || cameraId > 3)
		return false;
	string lables[4] = {
		"front", "rear", "left", "right"
	};
	return findBoardCorners(image, lables[cameraId], corners);
}

