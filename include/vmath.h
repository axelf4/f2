#ifndef VMATH_H
#define VMATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#if defined(_MSC_VER)
	/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
	/* GCC-compatible compiler, targeting x86/x86-64 */
#include <x86intrin.h>
#endif

	/*<mmintrin.h>  MMX

	<xmmintrin.h> SSE

	<emmintrin.h> SSE2

	<pmmintrin.h> SSE3

	<tmmintrin.h> SSSE3

	<smmintrin.h> SSE4.1

	<nmmintrin.h> SSE4.2

	<ammintrin.h> SSE4A

	<wmmintrin.h> AES

	<immintrin.h> AVX*/

#ifdef _WIN32
#if defined(__MINGW32__) || defined(__CYGWIN__) || (defined (_MSC_VER) && _MSC_VER < 1300)
#define ATTRIBUTE_ALIGNED(i)
#else
#define ALIGN(i) __declspec(align(i))
#define VMATH_INLINE inline
// #include <emmintrin.h>
// #include <smmintrin.h>
#endif
#elif defined __GNUC__
#define ALIGN(i)  __attribute__((aligned(i)))
#endif
	// if __ARM_NEON__ or __SSE__
#ifdef USE_DOUBLE_PRECISION
	typedef double scalar;
#else
	typedef float scalar;
#endif
	// Wether to use row or column major matrices
#define MATRIX_ROW_MAJOR
	// Wether using SSE/SSE2 // USE_SSE
#define _SSE_INTRINSICS_

#ifdef _SSE_INTRINSICS_
#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(z, y, x, w) (((z) << 6) | ((y) << 4) | ((x) << 2) | (w))
#endif
#define _mm_replicate_x_ps(v) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(0, 0, 0, 0))
#define _mm_replicate_y_ps(v) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(1, 1, 1, 1))
#define _mm_replicate_z_ps(v) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(2, 2, 2, 2))
#define _mm_replicate_w_ps(v) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(3, 3, 3, 3))
#define _mm_madd_ps(a, b, c) _mm_add_ps(_mm_mul_ps((a), (b)), (c))
#endif

#define PI 3.141592654f
	// Loop unrolling
#define ROUND_DOWN(x, s) ((x) & ~((s)-1))

#define M00 0
#define M01 4
#define M02 8
#define M03 12
#define M10 1
#define M11 5
#define M12 9
#define M13 13
#define M20 2
#define M21 6
#define M22 10
#define M23 14
#define M30 3
#define M31 7
#define M32 11
#define M33 15

	/* VECTOR */

	typedef
#ifdef _SSE_INTRINSICS_
		__m128
#else
	union { float v[4]; struct { float x, y, z, w; } }
#endif
	VEC;

	VMATH_INLINE VEC VectorReplicate(float v) {
#ifdef _SSE_INTRINSICS_
		return(_mm_set1_ps(v));
#else
		return((VEC) { { v, v, v, v } });
#endif
	}

	VMATH_INLINE VEC VectorSet(float x, float y, float z, float w) {
#ifdef _SSE_INTRINSICS_
		return(_mm_setr_ps(x, y, z, w));
#else
		return((VEC) { w, z, y, x });
#endif
	}

	VMATH_INLINE VEC VectorLoad(float *v) {
#ifdef _SSE_INTRINSICS_
		return(_mm_load_ps(v));
#else
		return((VEC) { v[0], v[1], v[2], v[3] });
#endif
	}

#ifdef _SSE_INTRINSICS_
#define VectorGet(_V, _A) (_mm_store_ps((_V), (_A)), (_V))
#else
#define VectorGet(_V, _A) (memcpy((_V), (_A).v, sizeof(float) * 4), (_V))
#endif

	VMATH_INLINE VEC VectorAdd(VEC a, VEC b) {
#ifdef _SSE_INTRINSICS_
		return(_mm_add_ps(a, b));
#else
		return((VEC) { a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3] });
#endif
	}

	VMATH_INLINE VEC VectorSubtract(VEC a, VEC b) {
#ifdef _SSE_INTRINSICS_
		return(_mm_sub_ps(a, b));
#else
		return((VEC) { a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3] });
#endif
	}

	VMATH_INLINE VEC VectorMultiply(VEC a, VEC b) {
#ifdef _SSE_INTRINSICS_
		return(_mm_mul_ps(a, b));
#else
		return((VEC) { a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3] });
#endif
	}

	VMATH_INLINE VEC VectorDivide(VEC a, VEC b) {
#ifdef _SSE_INTRINSICS_
		return(_mm_div_ps(a, b));
#else
		return((VEC) { a.v[0] / b.v[0], a.v[1] / b.v[1], a.v[2] / b.v[2], a.v[3] / b.v[3] });
#endif
	}

	VMATH_INLINE float VectorLength(VEC v) {
		// Using SSE 4
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v, v, 0x71)));
	}

	VMATH_INLINE VEC VectorNormalize(VEC v) {
#define SSE4
#ifdef SSE4
		// return _mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0x77)));
		return _mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0x7F)));
#else
		__m128 vec0, vec1;
		// vec0 = _mm_and_ps(v, vector4::vector4_clearW);
		vec0 = _mm_mul_ps(vec0, vec0);
		vec1 = vec0;
		vec0 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(2, 1, 0, 3));
		vec1 = _mm_add_ps(vec0, vec1);
		vec0 = vec1;
		vec1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(1, 0, 3, 2));
		vec0 = _mm_add_ps(vec0, vec1);
		vec0 = _mm_rsqrt_ps(vec0);
		return(_mm_mul_ps(vec0, v));
#endif
	}

	VMATH_INLINE VEC VectorCross(VEC a, VEC b) {
		return _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2))), _mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1))));
	}

	VMATH_INLINE VEC QuaternionRotationRollPitchYaw(float pitch, float yaw, float roll) {
		// Assuming the angles are in radians.
		const float hr = roll * 0.5f;
		const float shr = (float)sin(hr);
		const float chr = (float)cos(hr);
		const float hp = pitch * 0.5f;
		const float shp = (float)sin(hp);
		const float chp = (float)cos(hp);
		const float hy = yaw * 0.5f;
		const float shy = (float)sin(hy);
		const float chy = (float)cos(hy);
		const float chy_shp = chy * shp;
		const float shy_chp = shy * chp;
		const float chy_chp = chy * chp;
		const float shy_shp = shy * shp;

		return(_mm_setr_ps(
			(chy_shp * chr) + (shy_chp * shr), // cos(yaw/2) * sin(pitch/2) * cos(roll/2) + sin(yaw/2) * cos(pitch/2) * sin(roll/2)
			(shy_chp * chr) - (chy_shp * shr), // sin(yaw/2) * cos(pitch/2) * cos(roll/2) - cos(yaw/2) * sin(pitch/2) * sin(roll/2)
			(chy_chp * shr) - (shy_shp * chr), // cos(yaw/2) * cos(pitch/2) * sin(roll/2) - sin(yaw/2) * sin(pitch/2) * cos(roll/2)
			(chy_chp * chr) + (shy_shp * shr) // cos(yaw/2) * cos(pitch/2) * cos(roll/2) + sin(yaw/2) * sin(pitch/2) * sin(roll/2)
			));
	}

	/* MATRIX */

	typedef struct {
#ifdef _SSE_INTRINSICS_
		__m128 row0, row1, row2, row3;
#else
		float m[16];
#endif
	} MAT;

	VMATH_INLINE MAT MatrixSet(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33) {
#ifdef _SSE_INTRINSICS_
		return(MAT{ _mm_setr_ps(m00, m01, m02, m03), _mm_setr_ps(m10, m11, m12, m13), _mm_setr_ps(m20, m21, m22, m23), _mm_setr_ps(m30, m31, m32, m33) });
#endif
	}

	VMATH_INLINE MAT MatrixLoad(float *v) {
		return(MAT{ _mm_load_ps(v), _mm_load_ps(v + 4), _mm_load_ps(v + 8), _mm_load_ps(v + 12) });
	}

	VMATH_INLINE MAT MatrixIdentity() {
#ifdef _SSE_INTRINSICS_
		return(MAT{ _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_setr_ps(0, 0, 0, 1) });
#else
		return 0;
#endif
	}

	VMATH_INLINE MAT MatrixPerspective(float fov, float aspect, float zNear, float zFar) {
#ifdef _SSE_INTRINSICS_
		const float h = 1.0f / tan(fov * PI / 360);
		return(MAT{ _mm_setr_ps(h / aspect, 0, 0, 0), _mm_setr_ps(0, h, 0, 0), _mm_setr_ps(0, 0, (zNear + zFar) / (zNear - zFar), -1), _mm_setr_ps(0, 0, 2 * (zNear * zFar) / (zNear - zFar), 0) });
#endif
		return MatrixIdentity();
	}

#ifdef _SSE_INTRINSICS_
#define MatrixGet(_V, _A) (_mm_store_ps((_V), (_A)->row0), _mm_store_ps((_V) + 4, (_A)->row1), _mm_store_ps((_V) + 8, (_A)->row2), _mm_store_ps((_V) + 12, (_A)->row3), (_V))
#else
#define MatrixGet(_V, _A) (memcpy((_V), (_A)->m, sizeof(float) * 16), (_V))
#endif

	VMATH_INLINE MAT MatrixMultiply(MAT *a, MAT *b) {
		/// Performs a matrix-matrix multiplication and stores the product in result.
		/// Corresponds to the mathematical statement reuslt = matrix1 * matrix2
		/// @warning The three matrices must be distinct, the result will be incorrect if result is the same object as matrix1 or matrix2.
#ifdef _SSE_INTRINSICS_
		/*__m128 row1 = _mm_load_ps(&B[0]);
		__m128 row2 = _mm_load_ps(&B[4]);
		__m128 row3 = _mm_load_ps(&B[8]);
		__m128 row4 = _mm_load_ps(&B[12]);
		for (int i = 0; i<4; i++) {
		__m128 brod1 = _mm_set1_ps(A[4 * i + 0]);
		__m128 brod2 = _mm_set1_ps(A[4 * i + 1]);
		__m128 brod3 = _mm_set1_ps(A[4 * i + 2]);
		__m128 brod4 = _mm_set1_ps(A[4 * i + 3]);
		__m128 row = _mm_add_ps(
		_mm_add_ps(
		_mm_mul_ps(brod1, row1),
		_mm_mul_ps(brod2, row2)),
		_mm_add_ps(
		_mm_mul_ps(brod3, row3),
		_mm_mul_ps(brod4, row4)));
		_mm_store_ps(&C[4 * i], row);
		}*/

		/*__m128 row0;
		__m128 row1;
		__m128 row2;
		__m128 row3;
		__m128 value0;

		// Compute row 0 of the matrix product:
		value0 = _mm_shuffle_ps(a->m[0], a->m[0], _MM_SHUFFLE(3, 3, 3, 3));
		row0 = _mm_mul_ps(b->m[0], value0);

		value0 = _mm_shuffle_ps(a->m[0], a->m[0], _MM_SHUFFLE(2, 2, 2, 2));
		row0 = _mm_add_ps(row0, _mm_mul_ps(b->m[1], value0));

		value0 = _mm_shuffle_ps(a->m[0], a->m[0], _MM_SHUFFLE(1, 1, 1, 1));
		row0 = _mm_add_ps(row0, _mm_mul_ps(b->m[2], value0));

		value0 = _mm_shuffle_ps(a->m[0], a->m[0], _MM_SHUFFLE(0, 0, 0, 0));
		row0 = _mm_add_ps(row0, _mm_mul_ps(b->m[3], value0));

		// Compute row 1 of the matrix product:
		value0 = _mm_shuffle_ps(a->m[1], a->m[1], _MM_SHUFFLE(3, 3, 3, 3));
		row1 = _mm_mul_ps(b->m[0], value0);

		value0 = _mm_shuffle_ps(a->m[1], a->m[1], _MM_SHUFFLE(2, 2, 2, 2));
		row1 = _mm_add_ps(row1, _mm_mul_ps(b->m[1], value0));

		value0 = _mm_shuffle_ps(a->m[1], a->m[1], _MM_SHUFFLE(1, 1, 1, 1));
		row1 = _mm_add_ps(row1, _mm_mul_ps(b->m[2], value0));

		value0 = _mm_shuffle_ps(a->m[1], a->m[1], _MM_SHUFFLE(0, 0, 0, 0));
		row1 = _mm_add_ps(row1, _mm_mul_ps(b->m[3], value0));

		// Compute row 2 of the matrix product:
		value0 = _mm_shuffle_ps(a->m[2], a->m[2], _MM_SHUFFLE(3, 3, 3, 3));
		row2 = _mm_mul_ps(b->m[0], value0);

		value0 = _mm_shuffle_ps(a->m[2], a->m[2], _MM_SHUFFLE(2, 2, 2, 2));
		row2 = _mm_add_ps(row2, _mm_mul_ps(b->m[1], value0));

		value0 = _mm_shuffle_ps(a->m[2], a->m[2], _MM_SHUFFLE(1, 1, 1, 1));
		row2 = _mm_add_ps(row2, _mm_mul_ps(b->m[2], value0));

		value0 = _mm_shuffle_ps(a->m[2], a->m[2], _MM_SHUFFLE(0, 0, 0, 0));
		row2 = _mm_add_ps(row2, _mm_mul_ps(b->m[3], value0));

		// Compute row 3 of the matrix product:
		value0 = _mm_shuffle_ps(a->m[3], a->m[3], _MM_SHUFFLE(3, 3, 3, 3));
		row3 = _mm_mul_ps(b->m[0], value0);

		value0 = _mm_shuffle_ps(a->m[3], a->m[3], _MM_SHUFFLE(2, 2, 2, 2));
		row3 = _mm_add_ps(row3, _mm_mul_ps(b->m[1], value0));

		value0 = _mm_shuffle_ps(a->m[3], a->m[3], _MM_SHUFFLE(1, 1, 1, 1));
		row3 = _mm_add_ps(row3, _mm_mul_ps(b->m[2], value0));

		value0 = _mm_shuffle_ps(a->m[3], a->m[3], _MM_SHUFFLE(0, 0, 0, 0));
		row3 = _mm_add_ps(row3, _mm_mul_ps(b->m[3], value0));

		// Write results back to memory:
		return(MAT{ { row0, row1, row2, row3 } });*/

		/*const __m128 a = a2->m[0];
		const __m128 b = a2->m[1];
		const __m128 c = a2->m[2];
		const __m128 d = a2->m[3];

		ALIGN(16) float m[16];
		MatrixGet(m, b2);

		__m128 t1, t2;

		MAT result;

		t1 = _mm_set1_ps(m[M00]);
		t2 = _mm_mul_ps(a, t1);
		t1 = _mm_set1_ps(m[M01]);
		t2 = _mm_add_ps(_mm_mul_ps(b, t1), t2);
		t1 = _mm_set1_ps(m[M02]);
		t2 = _mm_add_ps(_mm_mul_ps(c, t1), t2);
		t1 = _mm_set1_ps(m[M03]);
		t2 = _mm_add_ps(_mm_mul_ps(d, t1), t2);

		result.m[0] = t2; // _mm_store_ps(&m[0], t2);

		t1 = _mm_set1_ps(m[M10]);
		t2 = _mm_mul_ps(a, t1);
		t1 = _mm_set1_ps(m[M11]);
		t2 = _mm_add_ps(_mm_mul_ps(b, t1), t2);
		t1 = _mm_set1_ps(m[M12]);
		t2 = _mm_add_ps(_mm_mul_ps(c, t1), t2);
		t1 = _mm_set1_ps(m[M13]);
		t2 = _mm_add_ps(_mm_mul_ps(d, t1), t2);

		result.m[1] = t2; // _mm_store_ps(&m[4], t2);

		t1 = _mm_set1_ps(m[M20]);
		t2 = _mm_mul_ps(a, t1);
		t1 = _mm_set1_ps(m[M21]);
		t2 = _mm_add_ps(_mm_mul_ps(b, t1), t2);
		t1 = _mm_set1_ps(m[M22]);
		t2 = _mm_add_ps(_mm_mul_ps(c, t1), t2);
		t1 = _mm_set1_ps(m[M23]);
		t2 = _mm_add_ps(_mm_mul_ps(d, t1), t2);

		result.m[2] = t2; // _mm_store_ps(&m[8], t2);

		t1 = _mm_set1_ps(m[M30]);
		t2 = _mm_mul_ps(a, t1);
		t1 = _mm_set1_ps(m[M31]);
		t2 = _mm_add_ps(_mm_mul_ps(b, t1), t2);
		t1 = _mm_set1_ps(m[M32]);
		t2 = _mm_add_ps(_mm_mul_ps(c, t1), t2);
		t1 = _mm_set1_ps(m[M33]);
		t2 = _mm_add_ps(_mm_mul_ps(d, t1), t2);

		result.m[3] = t2; // _mm_store_ps(&m[12], t2);

		return result;*/

		__m128 row0, row1, row2, row3;

		row0 = _mm_mul_ps(b->row0, _mm_replicate_x_ps(a->row0));
		row1 = _mm_mul_ps(b->row0, _mm_replicate_x_ps(a->row1));
		row2 = _mm_mul_ps(b->row0, _mm_replicate_x_ps(a->row2));
		row3 = _mm_mul_ps(b->row0, _mm_replicate_x_ps(a->row3));

		row0 = _mm_madd_ps(b->row1, _mm_replicate_y_ps(a->row0), row0);
		row1 = _mm_madd_ps(b->row1, _mm_replicate_y_ps(a->row1), row1);
		row2 = _mm_madd_ps(b->row1, _mm_replicate_y_ps(a->row2), row2);
		row3 = _mm_madd_ps(b->row1, _mm_replicate_y_ps(a->row3), row3);

		row0 = _mm_madd_ps(b->row2, _mm_replicate_z_ps(a->row0), row0);
		row1 = _mm_madd_ps(b->row2, _mm_replicate_z_ps(a->row1), row1);
		row2 = _mm_madd_ps(b->row2, _mm_replicate_z_ps(a->row2), row2);
		row3 = _mm_madd_ps(b->row2, _mm_replicate_z_ps(a->row3), row3);

		row0 = _mm_madd_ps(b->row3, _mm_replicate_w_ps(a->row0), row0);
		row1 = _mm_madd_ps(b->row3, _mm_replicate_w_ps(a->row1), row1);
		row2 = _mm_madd_ps(b->row3, _mm_replicate_w_ps(a->row2), row2);
		row3 = _mm_madd_ps(b->row3, _mm_replicate_w_ps(a->row3), row3);

		return(MAT{ row0, row1, row2, row3 });

		/*// load matrices a and b as column-major order of OpenGL ES
	__m128 ma_col_0 = _mm_load_ps(a);
	__m128 ma_col_1 = _mm_load_ps(a + 4);
	__m128 ma_col_2 = _mm_load_ps(a + 8);
	__m128 ma_col_3 = _mm_load_ps(a + 12);

	__m128 mb_col_0 = _mm_load_ps(b);
	__m128 mb_col_1 = _mm_load_ps(b + 4);
	__m128 mb_col_2 = _mm_load_ps(b + 8);
	__m128 mb_col_3 = _mm_load_ps(b + 12);

	// get ready to store the result
	__m128 result0;
	__m128 result1;
	__m128 result2;
	__m128 result3;

	// result = first column of B x first row of A
	result0 = _mm_mul_ps(mb_col_0, _mm_replicate_x_ps(ma_col_0));
	result1 = _mm_mul_ps(mb_col_0, _mm_replicate_x_ps(ma_col_1));
	result2 = _mm_mul_ps(mb_col_0, _mm_replicate_x_ps(ma_col_2));
	result3 = _mm_mul_ps(mb_col_0, _mm_replicate_x_ps(ma_col_3));

	// result += second column of B x second row of A
	result0 = _mm_madd_ps(mb_col_1, _mm_replicate_y_ps(ma_col_0), result0);
	result1 = _mm_madd_ps(mb_col_1, _mm_replicate_y_ps(ma_col_1), result1);
	result2 = _mm_madd_ps(mb_col_1, _mm_replicate_y_ps(ma_col_2), result2);
	result3 = _mm_madd_ps(mb_col_1, _mm_replicate_y_ps(ma_col_3), result3);

	// result += third column of B x third row of A
	result0 = _mm_madd_ps(mb_col_2, _mm_replicate_z_ps(ma_col_0), result0);
	result1 = _mm_madd_ps(mb_col_2, _mm_replicate_z_ps(ma_col_1), result1);
	result2 = _mm_madd_ps(mb_col_2, _mm_replicate_z_ps(ma_col_2), result2);
	result3 = _mm_madd_ps(mb_col_2, _mm_replicate_z_ps(ma_col_3), result3);

	// result += last column of B x last row of A
	result0 = _mm_madd_ps(mb_col_3, _mm_replicate_w_ps(ma_col_0), result0);
	result1 = _mm_madd_ps(mb_col_3, _mm_replicate_w_ps(ma_col_1), result1);
	result2 = _mm_madd_ps(mb_col_3, _mm_replicate_w_ps(ma_col_2), result2);
	result3 = _mm_madd_ps(mb_col_3, _mm_replicate_w_ps(ma_col_3), result3);

	// store the result to memory
	_mm_store_ps(output, result0);
	_mm_store_ps(output+4, result1);
	_mm_store_ps(output+8, result2);
	_mm_store_ps(output+12, result3);*/
#else
		// FIXME
		MAT result;
		result.m[M03] = (a.m[M03] * b.m[M33]) + (a.m[M02] * b.m[M23]) + (a.m[M01] * b.m[M13]) + (a.m[M00] * b.m[M03]);
		result.m[M02] = (a.m[M03] * b.m[M32]) + (a.m[M02] * b.m[M22]) + (a.m[M01] * b.m[M12]) + (a.m[M00] * b.m[M02]);
		result.m[M01] = (a.m[M03] * b.m[M31]) + (a.m[M02] * b.m[M21]) + (a.m[M01] * b.m[M11]) + (a.m[M00] * b.m[M01]);
		result.m[M00] = (a.m[M03] * b.m[M30]) + (a.m[M02] * b.m[M20]) + (a.m[M01] * b.m[M10]) + (a.m[M00] * b.m[M00]);

		result.m[M13] = (a.m[M13] * b.m[M33]) + (a.m[M12] * b.m[M23]) + (a.m[M11] * b.m[M13]) + (a.m[M10] * b.m[M03]);
		result.m[M12] = (a.m[M13] * b.m[M32]) + (a.m[M12] * b.m[M22]) + (a.m[M11] * b.m[M12]) + (a.m[M10] * b.m[M02]);
		result.m[M11] = (a.m[M13] * b.m[M31]) + (a.m[M12] * b.m[M21]) + (a.m[M11] * b.m[M11]) + (a.m[M10] * b.m[M01]);
		result.m[M10] = (a.m[M13] * b.m[M30]) + (a.m[M12] * b.m[M20]) + (a.m[M11] * b.m[M10]) + (a.m[M10] * b.m[M00]);

		result.m[M23] = (a.m[M23] * b.m[M33]) + (a.m[M22] * b.m[M23]) + (a.m[M21] * b.m[M13]) + (a.m[M20] * b.m[M03]);
		result.m[M22] = (a.m[M23] * b.m[M32]) + (a.m[M22] * b.m[M22]) + (a.m[M21] * b.m[M12]) + (a.m[M20] * b.m[M02]);
		result.m[M21] = (a.m[M23] * b.m[M31]) + (a.m[M22] * b.m[M21]) + (a.m[M21] * b.m[M11]) + (a.m[M20] * b.m[M01]);
		result.m[M20] = (a.m[M23] * b.m[M30]) + (a.m[M22] * b.m[M20]) + (a.m[M21] * b.m[M10]) + (a.m[M20] * b.m[M00]);

		result.m[M33] = (a.m[M33] * b.m[M33]) + (a.m[M32] * b.m[M23]) + (a.m[M31] * b.m[M13]) + (a.m[M30] * b.m[M03]);
		result.m[M32] = (a.m[M33] * b.m[M32]) + (a.m[M32] * b.m[M22]) + (a.m[M31] * b.m[M12]) + (a.m[M30] * b.m[M02]);
		result.m[M31] = (a.m[M33] * b.m[M31]) + (a.m[M32] * b.m[M21]) + (a.m[M31] * b.m[M11]) + (a.m[M30] * b.m[M01]);
		result.m[M30] = (a.m[M33] * b.m[M30]) + (a.m[M32] * b.m[M20]) + (a.m[M31] * b.m[M10]) + (a.m[M30] * b.m[M00]);

		for (int i = 0; i < 16; i += 4)
			for (int j = 0; j < 4; j++)
				r[i + j] = b[i] * a[j] + b[i + 1] * a[j + 4] + b[i + 2] * a[j + 8] + b[i + 3] * a[j + 12];

		return result;
#endif
	}

#include <stdio.h>

	/* Using Cramer's rule */
	VMATH_INLINE MAT MatrixInverse(MAT *a) {
#ifdef _SSE_INTRINSICS_
		__m128 minor0, minor1, minor2, minor3,
			row0, row1, row2, row3,
			det, tmp1;
		ALIGN(128) float src[16];
		MatrixGet(src, a);
		tmp1 = row0 = row1 = row2 = row3 = _mm_set1_ps(0);
		tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src + 4));
		row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src + 8)), (__m64*)(src + 12));

		/*ALIGN(16) float value[4];
		VectorGet(value, row1);
		printf("inverse: vector row1: |%2.0f|%2.0f|%2.0f|%2.0f|\n", value[0], value[0], value[0], value[0]);
		VectorGet(value, a->row1);
		printf("inverse: matrix row1: |%2.0f|%2.0f|%2.0f|%2.0f|\n", value[0], value[0], value[0], value[0]);*/

		row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
		row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
		tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src + 2)), (__m64*)(src + 6));
		row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src + 10)), (__m64*)(src + 14));
		row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
		row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

		/*row0 = a->row0;
		row1 = a->row1;
		row2 = a->row2;
		row3 = a->row3;*/
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row2, row3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor0 = _mm_mul_ps(row1, tmp1);
		minor1 = _mm_mul_ps(row0, tmp1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
		minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
		minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row1, row2);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
		minor3 = _mm_mul_ps(row0, tmp1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
		minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
		minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		row2 = _mm_shuffle_ps(row2, row2, 0x4E);
		minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
		minor2 = _mm_mul_ps(row0, tmp1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
		minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
		minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row0, row1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
		minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
		minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row0, row3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
		minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
		minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row0, row2);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
		minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
		minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
		// -----------------------------------------------
		det = _mm_mul_ps(row0, minor0);
		det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
		det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
		tmp1 = _mm_rcp_ss(det);
		det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
		det = _mm_shuffle_ps(det, det, 0x00);
		minor0 = _mm_mul_ps(det, minor0);
		// _mm_storel_pi((__m64*)(src), minor0);
		// _mm_storeh_pi((__m64*)(src + 2), minor0);
		minor1 = _mm_mul_ps(det, minor1);
		// _mm_storel_pi((__m64*)(src + 4), minor1);
		// _mm_storeh_pi((__m64*)(src + 6), minor1);
		minor2 = _mm_mul_ps(det, minor2);
		// _mm_storel_pi((__m64*)(src + 8), minor2);
		//_mm_storeh_pi((__m64*)(src + 10), minor2);
		minor3 = _mm_mul_ps(det, minor3);
		//_mm_storel_pi((__m64*)(src + 12), minor3);
		//_mm_storeh_pi((__m64*)(src + 14), minor3);
		return(MAT{ minor0, minor1, minor2, minor3 });
#endif
	}

	VMATH_INLINE MAT MatrixTranspose(MAT *a) {
		__m128 tmp0 = _mm_unpacklo_ps(a->row0, a->row1), tmp2 = _mm_unpacklo_ps(a->row2, a->row3), tmp1 = _mm_unpackhi_ps(a->row0, a->row1), tmp3 = _mm_unpackhi_ps(a->row2, a->row3);
		return(MAT{ _mm_movelh_ps(tmp0, tmp2), _mm_movehl_ps(tmp2, tmp0), _mm_movelh_ps(tmp1, tmp3), _mm_movehl_ps(tmp3, tmp1) });
	}

	VMATH_INLINE MAT MatrixTranslate(MAT *a, float x, float y, float z) {
		return MAT{ _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_setr_ps(x, y, z, 1) };
	}

	VMATH_INLINE MAT QuaternionToMatrix(VEC a) {
#ifdef _SSE_INTRINSICS_
		ALIGN(128) float q[4];
		VectorGet(q, a);
		float qxx = q[0] * q[0];
		float qyy = q[1] * q[1];
		float qzz = q[2] * q[2];
		float qxz = q[0] * q[2];
		float qxy = q[0] * q[1];
		float qyz = q[1] * q[2];
		float qwx = q[3] * q[0];
		float qwy = q[3] * q[1];
		float qwz = q[3] * q[2];

		return(MAT{
			_mm_setr_ps(1 - 2 * (qyy + qzz), 2 * (qxy + qwz), 2 * (qxz - qwy), 0),
			_mm_setr_ps(2 * (qxy - qwz), 1 - 2 * (qxx + qzz), 2 * (qyz + qwx), 0),
			_mm_setr_ps(2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx + qyy), 0),
			_mm_setr_ps(0, 0, 0, 1) });
#endif
	}

#ifdef __cplusplus
}
#endif

#endif