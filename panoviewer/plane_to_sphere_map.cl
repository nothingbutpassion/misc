
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) ((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat2(addr, x, y) 		rmat32fc2(addr, x, y)
#define wmat2(addr, x, y) 		wmat32fc2(addr, x, y)

/*
Point3d cross(Point3d p1, Point3d p2) {
	return Point3d(p1.y*p2.z - p1.z*p2.y, p1.z*p2.x - p1.x*p2.z, p1.x*p2.y - p1.y*p2.x);
}
double dot(Point3d p1, Point3d p2) {
	return p1.x*p2.x + p1.y*p2.y + p1.z*p2.z;
}
Point2d orthogonal_to_sphere(Point3d p) {
	double r = sqrt(dot(p, p));
	double theta = acos(p.z / r);
	double phi = atan2(p.y, p.x);
	phi = phi < 0 ? phi + 2 * PI : phi;
	return Point2d(phi, theta);
}
Point2d plane_to_sphere(double theta, double phi, Point2d p) {
	Point3d n = Point3d(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
	Point3d y = Point3d(sin(theta + PI / 2)*cos(phi), sin(theta + PI / 2)*sin(phi), cos(theta + PI / 2));
	Point3d x = cross(n, y);
	return orthogonal_to_sphere(n + x*p.x + y*p.y);
}
Point2d plane_to_sphere_map(double theta, double phi, Point2d fov, Size src_size, Size dst_size, Point2d dst_p) {
	double x_size = 2 * tan(fov.x / 2);
	double y_size = 2 * tan(fov.y / 2);
	double x = x_size * dst_p.x / dst_size.width;
	double y = y_size * dst_p.y / dst_size.height;
	x -= x_size / 2;
	y -= y_size / 2;
	Point2d pano_angle = plane_to_sphere(theta, phi, Point2d(x, y));
	return Point2d(src_size.width*pano_angle.x / (2 * PI), src_size.height*pano_angle.y / PI);
}
void sphere_projection(const Mat& pano, Mat& plane, double theta, double phi, double fov_x, double fov_y) {
	Mat map(plane.size(), CV_32FC2);
	for (int y = 0; y < map.rows; ++y) {
		for (int x = 0; x < map.cols; ++x) {
			Point2d p = plane_to_sphere_map(theta, phi, Point2d(fov_x, fov_y), pano.size(), plane.size(), Point2d(x, y));
			map.at<Point2f>(y, x) = Point2f(p.x, p.y);
		}
	}
	remap(pano, plane, map, Mat(), CV_INTER_CUBIC, BORDER_REPLICATE);
}
*/

__kernel void plane_to_sphere_map(
	__global float2* map, int map_step, int map_offset, int map_rows, int map_cols,
	int pano_rows, int pano_cols, float theta, float phi, float fov_x, float fov_y)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	
	if (x < map_cols && y < map_rows) {
		float x_size = 2 * tan(fov_x / 2);
		float y_size = 2 * tan(fov_y / 2);
		float x0 = x_size * x/map_cols - x_size/2;
		float y0 = y_size * y/map_rows - y_size/2;
		
		float3 N = (float3)(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
		float3 Y = (float3)(sin(theta + M_PI/2)*cos(phi), sin(theta + M_PI/2)*sin(phi), cos(theta + M_PI/2));
		float3 X = cross(N, Y);
		
		float3 p = N + X*x0 + Y*y0;
		float  p_r = length(p);
		float  p_theta = acos(p.z/p_r);
		float  p_phi = atan2(p.y, p.x);
		p_phi = p_phi < 0 ? p_phi + 2*M_PI : p_phi;

		wmat2(map, x, y) = (float2)(pano_cols*p_phi/(2*M_PI), pano_rows*p_theta/M_PI);
	}
	
}
