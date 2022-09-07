#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace std;

/**
 * @brief Solve linear equations ax=b using Gauss-Jordan elimination 
 * 
 * @param a  mxm matrix, changed in this function
 * @param b  mxn matrix, save the solve x
 * 
 * @return true is if a is a non-singular matrix
 */
bool gauss_jordan_solve(float* a, int astep, float* b, int bstep, int m, int n) {
    const float epsilon = 1e-11;
    for (int j=0; j < m; ++j) {
        // NOTES:
        // a(i,j) is a[i*astep+j]
        // b(i,j) is b[i*bstep+j]

        // select the max value in a(:,j)
        int k = j;
        for (int i=k+1; i < m; ++i) {
            if (fabs(a[i*astep+j]) > fabs(a[k*astep+j]))
                k = i;
        }
        // NOTES:
        // the max vaule a(k,j) is near to 0, maybe overflow
        if (fabs(a[k*astep+j]) < epsilon)
            return false;

        // swap a(k,:) and a(j,:)
        if (k != j) {
            for (int i=0; i < m; ++i)
                swap(a[j*astep+i], a[k*astep+i]);
            for (int i=0; i < n; ++i)
                swap(b[j*bstep+i], b[k*bstep+i]); 
        }
        // Now a(j,j) is the max value
        for (k=0; k < m; ++k) {
            if (k != j) {
                // alpha = -a(k,j)/a(j,j)
                // a(k,:) +=  a(j,:)*alpha
                float alpha = -a[k*astep+j]/a[j*astep+j];
                for (int i=0; i < m; ++i)
                    a[k*astep+i] += a[j*astep+i]*alpha;
                for (int i=0; i < n; ++i)
                    b[k*bstep+i] += b[j*bstep+i]*alpha; 
            }
        }
    }
    // Got the solve
    for (int j=0; j< m; ++j) {
        // b(j,:) = b(j,:)/a(j,j)
        for (int i=0; i < n; ++i)
            b[j*bstep+i] /= a[j*astep+j];
    }
    return true;
}

/**
 * @brief Solve linear equations ax=b using Gauss elimination 
 * 
 * @param a  mxm matrix, changed in this function
 * @param b  mxn matrix, save the solve x
 * 
 * @return true is if a is a non-singular matrix
 */
bool gauss_solve(float* a, int astep, float* b, int bstep, int m, int n) {
    const float epsilon = 1e-11;
    for (int j=0; j < m; ++j) {
        // NOTES:
        // a(i,j) is a[i*astep+j]
        // b(i,j) is b[i*bstep+j]

        // select the max value in a(:,j)
        int k = j;
        for (int i=k+1; i < m; ++i) {
            if (fabs(a[i*astep+j]) > fabs(a[k*astep+j]))
                k = i;
        }
        // NOTES:
        // the max vaule a(k,j) is near to 0, maybe overflow
        if (fabs(a[k*astep+j]) < epsilon)
            return false;

        // swap a(k,:) and a(j,:)
        if (k != j) {
            for (int i=j; i < m; ++i)
                swap(a[j*astep+i], a[k*astep+i]);
            for (int i=0; i < n; ++i)
                swap(b[j*bstep+i], b[k*bstep+i]); 
        }
        // Now a(j,j) is the max value
        for (k=j+1; k < m; ++k) {
            // alpha = -a(k,j)/a(j,j)
            // a(k,:) +=  a(j,:)*alpha
            float alpha = -a[k*astep+j]/a[j*astep+j];
            for (int i=j; i < m; ++i)
                a[k*astep+i] += a[j*astep+i]*alpha;
            for (int i=0; i < n; ++i)
                b[k*bstep+i] += b[j*bstep+i]*alpha; 
        }
    }
    // Got the solve
    for (int j=m-1; j >= 0; --j) {
        for (int k=0; k < n; ++k) {
            for (int i=j+1; i < m; ++i) {
                b[j*bstep+k] -= a[j*astep+i]*b[i*bstep+k];
            }
            b[j*bstep+k] /= a[j*astep+j];
        }
    }
    return true;
}

/**
 * @brief Calculates eigenvalues and eigenvectors of a symmetric matrix by using jacobi algorithm
 *		  a * v.row(i).t = e(i) * v.row(i).t 
 *		  wherein
 *		  v.row(i) is the ith row of v, it's a row vector (1xn) 
 *		  v.row(i).t is the transpose, it's a cols vector (nx1)
 *		  e(i) is the ith eigen values
 *
 * @note For more details about jacobi algorithm, refer to
 *       http://fourier.eng.hmc.edu/e176/lectures/ch1/node1.html
 *    
 * @param a	nxn symmetric matrix
 * @param v return nxn eigen vectors
 * @param e return n eigen values
 * 
 */
void jacobi_eigen(float* a, int astep, float* v, int vstep, float* e, int n) {
	
	const float epsilon = 1e-11;
	const int iterations = n*n*30;

	// init eigen vectors as identity matrix
	for (int r = 0; r < n; ++r)
		for (int c = 0; c < n; c++)
			v[vstep*r + c] = (r == c) ? 1 : 0;
	
	// repeate jacobi algorithm
	for (int it = 0; it < iterations; ++it) {
		// find the max absolute value of off-diagonal elements, the pivot (i,j)
		// NOTES:
		// beacuse a is symmetric matrix, we only consider upper-triangle part, so always has i < j
		float mv = 0;
		int i = 0, j = 1;
		for (int r = 0; r < n; ++r)
			for (int c = r+1; c < n; c++)
				if (mv < fabs(a[astep*r + c]))
					mv = fabs(a[astep*r + c]), i = r, j = c;

		// precision is met, stop iteration
		if (mv < epsilon)
			break;

		// do transform for a
		// w = (a(j,j) - a(i,i)) / (2*a(i,j)) = cot(2*theta)
		// t = tan(theta), s=sin(theta) c=cos(theta)
		// a(i,i) = a(i,i) - t*a(i,j)
		// a(j,j) = a(j,j) + t*a(i,j)
		// a(i,j) = a(j,i) = 0
		float w = 0.5*(a[astep*j + j] - a[astep*i + i])/a[astep*i + j];
		float t = -w + (w > 0 ? sqrt(1 + w*w) : -sqrt(1 + w*w));
		float s = t / sqrt(1 + t*t);
		float c = 1 / sqrt(1 + t*t);
		a[astep*i + i] -= t*a[astep*i + j];
		a[astep*j + j] += t*a[astep*i + j];
		a[astep*i + j] = a[astep*j + i] = 0;
		// a(i,k) = a(k,i) = c*a(i,k)-s*a(j,k)
		// a(j,k) = a(k,j) = c*a(j,k)+s*a(i,k)
		for (int k = 0; k < n; ++k)
			if (k != i && k != j) {
				// ith row/cols
				float v0 = c*a[astep*i + k] - s*a[astep*j + k];
				float v1 = c*a[astep*j + k] + s*a[astep*i + k];
				a[astep*i + k] = a[astep*k + i] = v0;
				a[astep*j + k] = a[astep*k + j] = v1;
			}
		
		// do transform for v
		for (int k = 0; k < n; ++k) {
			float v0 = c*v[astep*i + k] - s*v[vstep*j + k];
			float v1 = c*v[astep*j + k] + s*v[vstep*i + k];
			v[astep*i + k] = v0;
			v[astep*j + k] = v1;
		}
	}

	// now, the diagonal elements of a are the eigen values 
	for (int k = 0; k < n; ++k)
		e[k] = a[astep*k + k];

	// sort the eigen values and eigen vectors
	for (int k = 0; k < n - 1; ++k) {
		int m = k;
		for (int i = k + 1; i < n; ++i)
			if (e[m] < e[i])
				m = i;
		if (m != k) {
			// switch the k-th and m-th eigen value
			swap(e[k], e[m]);
			// switch the k-th row and m-th row
			for (int i = 0; i < n; ++i)
				swap(v[vstep*k + i], v[vstep*m + i]);
		}
	}		
}


void test_jacobi_eigen() {
    printf("%s()\n", __func__);
	const int n = 3;
	float a[] = {
		1, 2, 3,
		2, 2, 1,
		3, 1, 3
	};
	float v[n*n];
	float e[n];
	jacobi_eigen(a, n, v, n, e, n);
	for (int i = 0; i < n; ++i) {
		printf("eigen value: %10.6f  ", e[i]);
		printf("eigen vector: [");
		for (int j = 0; j < n; ++j)
			printf(" %10.6f", v[n * i + j]);
		printf(" ]\n");
	}
}

void test_solve_linear_equations() {
    printf("%s()\n", __func__);
    float a[3][3] = {
        {4, -9, 2},
        {2, -4, 6},
        {1, -1, 3}
    };
    float b[3][2] = {
        {5, 3},
        {3, 4},
        {4, 5}
    };
    gauss_solve((float*)a, 3, (float*)b, 2, 3, 2);

    float c[3][3] = {
        {4, -9, 2},
        {2, -4, 6},
        {1, -1, 3}
    };
    float d[3][2] = {
        {5, 3},
        {3, 4},
        {4, 5}
    };
    gauss_jordan_solve((float*)c, 3, (float*)d, 2, 3, 2);
    
    printf("ground_truth_solve: {%.2f,%.2f,%.2f,%.2f,%.2f,%.2f}\n", 6.95, 2.5, -0.15, 7.4, 3.0, 0.2);
    printf("gauss_solve: {%.2f,%.2f,%.2f,%.2f,%.2f,%.2f}\n", b[0][0], b[1][0], b[2][0], b[0][1], b[1][1], b[2][1]);
    printf("gauss_jordan_solve: {%.2f,%.2f,%.2f,%.2f,%.2f,%.2f}\n", d[0][0], d[1][0], d[2][0], d[0][1], d[1][1], d[2][1]);
}

int main()
{
    test_jacobi_eigen();
    test_solve_linear_equations();
    return  0;
}