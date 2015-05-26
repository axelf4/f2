/** Vector math library with SIMD acceleration.
	\file vmath.h */

	// TODO have cmake set the variables __SSE__, __SSE2__, __SSE3, __SSE4_1__, __SSE4_2__

#ifndef VMATH_H
#define VMATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <string.h>
#include <assert.h>

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
#endif
#elif defined __GNUC__
#define ALIGN(i)  __attribute__ ((aligned (i)))
#endif
	// if __ARM_NEON__ or __SSE__ or __ALTIVEC__
#define VMATH_INLINE inline /**< Inlining. */

#ifndef MATRIX_ORDER
	/** A value of either 'ROW_MAJOR' or 'COLUMN_MAJOR' that specifies the memory layout for matrices of type ::MAT.
	For ROW_MAJOR the order would be:
	> 00 01 02 03\n
	> 04 05 06 07\n
	> 08 09 10 11\n
	> 12 13 14 15

	And for COLUMN_MAJOR:
	> 00 04 08 12\n
	> 01 05 09 13\n
	> 02 06 10 14\n
	> 03 07 11 15 */
#define MATRIX_ORDER ROW_MAJOR
#elif MATRIX_ORDER != ROW_MAJOR && MATRIX_ORDER != COLUMN_MAJOR
#error "MATRIX_ORDER is not properly defined"
#endif

	// Doxygen won't generate documentation for macros that aren't defined
#ifdef __DOXYGEN__
#define NO_SIMD_INTRINSICS /**< If defined, compiled code won't be SIMD accelerated even if availible. */
#endif

#ifdef NO_SIMD_INTRINSICS
#undef __SSE__
#undef __SSE2__
#undef __SSE3__
#undef __SSE4_1__
#undef __SSE4_2__
#else
#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(z, y, x, w) (((z) << 6) | ((y) << 4) | ((x) << 2) | (w))
#endif
#define _mm_replicate_x_ps(v) _mm_shuffle_ps((v), (v), 0x00)
#define _mm_replicate_y_ps(v) _mm_shuffle_ps((v), (v), 0x55)
#define _mm_replicate_z_ps(v) _mm_shuffle_ps((v), (v), 0xAA)
#define _mm_replicate_w_ps(v) _mm_shuffle_ps((v), (v), 0xFF)
#define _mm_madd_ps(a, b, c) _mm_add_ps(_mm_mul_ps((a), (b)), (c)) /** (a * b + c) */
#endif

#define PI 3.141592654f /**< An optimal approximation of the constant pi. */
#define NULL_VECTOR VectorReplicate(0) /**< A null vector (or zero vector) whose length is zero, with the components [0, 0, 0, 0] (*0*). */

#if MATRIX_ORDER == ROW_MAJOR
#define M_00 0 /**< XX. */
#define M_01 4 /**< XY. */
#define M_02 8 /**< XZ. */
#define M_03 12 /**< XW. */
#define M_10 1 /**< YX. */
#define M_11 5 /**< YY. */
#define M_12 9 /**< YZ. */
#define M_13 13 /**< YW. */
#define M_20 2 /**< ZX. */
#define M_21 6 /**< ZY. */
#define M_22 10 /**< ZZ. */
#define M_23 14 /**< ZW. */
#define M_30 3 /**< WX. */
#define M_31 7 /**< WY. */
#define M_32 11 /**< WZ. */
#define M_33 15 /**< WW. */
#else
#define M_00 0 /**< XX. */
#define M_01 1 /**< XY. */
#define M_02 2 /**< XZ. */
#define M_03 3 /**< XW. */
#define M_10 4 /**< YX. */
#define M_11 5 /**< YY. */
#define M_12 6 /**< YZ. */
#define M_13 7 /**< YW. */
#define M_20 8 /**< ZX. */
#define M_21 9 /**< ZY. */
#define M_22 10 /**< ZZ. */
#define M_23 11 /**< ZW. */
#define M_30 12 /**< WX. */
#define M_31 13 /**< WY. */
#define M_32 14 /**< WZ. */
#define M_33 15 /**< WW. */
#endif

	/* VECTOR */

	/** A type representing a four-dimensional vector. */
	typedef
#ifdef __SSE__
		__m128
#else
	struct { float v[4]; /**< A 4 elements long float vector containing the components. */ }
#endif
	VEC;

	/** Stores a representation of the vector \a _A in the float array \a _V and returns \a _V.
	@def VectorGet(_V, _A)
	@param[out] _V A 4 elements long 16-byte aligned float array to store in.
	@param[in] _A The vector to be stored.

	This method is to be used when the data of a ::VEC needs to be converted to a more general, usable format. */
#ifdef __SSE__
#define VectorGet(_V, _A) (_mm_store_ps((_V), (_A)), (_V))
#else
#define VectorGet(_V, _A) (memcpy((_V), (_A).v, sizeof(float) * 4), (_V))
#endif

	/** Returns a ::VEC, whoose components are solely \a v.
		@param v The value to use for the components. */
	VMATH_INLINE VEC VectorReplicate(float v) {
#ifdef __SSE__
		return(_mm_set1_ps(v));
#else
		VEC result = { v, v, v, v };
		return(result);
#endif
	}

	/** Returns a ::VEC consisting of the components \a x, \a y, \a z and \a w.
		@param x The x component.
		@param y The y component.
		@param z The z component.
		@param w The w component. */
	VMATH_INLINE VEC VectorSet(float x, float y, float z, float w) {
#ifdef __SSE__
		return(_mm_setr_ps(x, y, z, w));
#else
		VEC v = { x, y, z, w };
		return(v);
#endif
	}

	/** Loads and returns a ::VEC from the float array \a v.
		@param v The float array to load up. */
	VMATH_INLINE VEC VectorLoad(float *v) {
#ifdef __SSE__
		return(_mm_load_ps(v));
#else
		VEC result = { v[0], v[1], v[2], v[3] };
		return(result);
#endif
	}

	/** Adds the two vectors \a a and \a b (a + b).
		@param a The first vector to add.
		@param b The second vector to add.
		@return The sum of the two vectors. */
	VMATH_INLINE VEC VectorAdd(VEC a, VEC b) {
#ifdef __SSE__
		return(_mm_add_ps(a, b));
#else
		VEC v = { a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3] };
		return(v);
#endif
	}

	/** Subtracts the vector \a b from \a a (a - b).
		@param a The vector to be subtracted.
		@param b The vector to subtract.
		@return The difference between the two vectors. */
	VMATH_INLINE VEC VectorSubtract(VEC a, VEC b) {
#ifdef __SSE__
		return(_mm_sub_ps(a, b));
#else
		VEC v = { a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3] };
		return(v);
#endif
	}

	/** Multiplies the two vectors \a a and \a b (a * b).
		@param a The first vector to multiply.
		@param b The second vector to multiply.
		@return The product of sum of the two vectors. */
	VMATH_INLINE VEC VectorMultiply(VEC a, VEC b) {
#ifdef __SSE__
		return(_mm_mul_ps(a, b));
#else
		VEC v = { a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3] };
		return(v);
#endif
	}

	/** Divides the vector \a a with \a b (a / b).
		@param a The dividend.
		@param b The divisor.
		@return The quotient of the two vectors. */
	VMATH_INLINE VEC VectorDivide(VEC a, VEC b) {
#ifdef __SSE__
		return(_mm_div_ps(a, b));
#else
		VEC v = { a.v[0] / b.v[0], a.v[1] / b.v[1], a.v[2] / b.v[2], a.v[3] / b.v[3] };
		return(v);
#endif
	}

	/** Returns the length or magnitude or norm of the vector \a a (||a||). */
	VMATH_INLINE float VectorLength(VEC a) {
#ifdef __SSE4_1__
		return(_mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(a, a, 0x71))));
#elif defined(__SSE__)

#else
		return((float)sqrt(a.v[0] * a.v[0] + a.v[1] * a.v[1] + a.v[2] * a.v[2]));
#endif
	}

	/** Returns the normalized vector \a a (â). */
	VMATH_INLINE VEC VectorNormalize(VEC a) {
#ifdef __SSE4_1__
		return(_mm_mul_ps(a, _mm_rsqrt_ps(_mm_dp_ps(a, a, 0x7F /* 0x77 */))));
#elif defined(__SSE__)
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
		return(_mm_mul_ps(vec0, a));
#else
		float length = VectorLength(a);
		VEC v = { a.v[0] / length, a.v[1] / length, a.v[2] / length, a.v[3] / length };
		return v;
#endif
	}

	/** Returns the dot product, a.k.a. the scalar product, of the two vectors \a a and \a b (a · b). */
	VMATH_INLINE float VectorDot(VEC a, VEC b) {
#if defined(__SSE4_1__)
		return(_mm_cvtss_f32(_mm_dp_ps(a, b, 0x71)));
#elif defined(__SSE3__)
		__m128 r1 = _mm_mul_ps(a, b), r2 = _mm_hadd_ps(r1, r1), r3 = _mm_hadd_ps(r2, r2);
		return(_mm_cvtss_f32(r3));
#elif defined(__SSE__)
		__m128 m = _mm_mul_ps(a, b), t = _mm_add_ps(m, _mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 3, 0, 1)));
		return(_mm_cvtss_f32(_mm_add_ps(t, _mm_shuffle_ps(t, t, _MM_SHUFFLE(1, 0, 3, 2)))));
#else
		return(a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2]);
#endif
	}

	/** Returns the cross product, a.k.a. the vector product, of the two vectors \a a and \a b (a × b). */
	VMATH_INLINE VEC VectorCross(VEC a, VEC b) {
#ifdef __SSE__
		return(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2))), _mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1)))));
#else
		VEC v = { a.v[1] * b.v[2] - a.v[2] * b.v[1],
			a.v[2] * b.v[0] - a.v[0] * b.v[2],
			a.v[0] * b.v[1] - a.v[1] * b.v[0],
			0 };
		return v;
#endif
	}

	/** Compare the elements in \a a and \a b and return a bit mask where \c 1 corresponds to equality.
		@param a The vector to compare
		@param b The vector to compare
		@return The result as a bit mask */
	VMATH_INLINE int VectorEqual(VEC a, VEC b) {
#ifdef __SSE__
		return _mm_movemask_ps(_mm_cmpeq_ps(a, b));
#else
		return (a.v[0] == b.v[0] ? 1 : 0) | (a.v[1] == b.v[1] ? 1 << 1 : 0) | (a.v[2] == b.v[2] ? 1 << 2 : 0) | (a.v[3] == b.v[3] ? 1 << 3 : 0);
#endif
	}

	/** Constructs a new ::VEC from the Euler angles \a pitch, \a yaw and \a roll in radians.
		@param pitch The pitch in radians.
		@param yaw The yaw in radians.
		@param roll The roll in radians.
		@return A new ::VEC from the supplied angles. */
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

#ifdef __SSE__
		return(_mm_setr_ps((chy_shp * chr) + (shy_chp * shr), (shy_chp * chr) - (chy_shp * shr), (chy_chp * shr) - (shy_shp * chr), (chy_chp * chr) + (shy_shp * shr)));
#else
		VEC v = { (chy_shp * chr) + (shy_chp * shr), // cos(yaw/2) * sin(pitch/2) * cos(roll/2) + sin(yaw/2) * cos(pitch/2) * sin(roll/2)
			(shy_chp * chr) - (chy_shp * shr), // sin(yaw/2) * cos(pitch/2) * cos(roll/2) - cos(yaw/2) * sin(pitch/2) * sin(roll/2)
			(chy_chp * shr) - (shy_shp * chr), // cos(yaw/2) * cos(pitch/2) * sin(roll/2) - sin(yaw/2) * sin(pitch/2) * cos(roll/2)
			(chy_chp * chr) + (shy_shp * shr) }; // cos(yaw/2) * cos(pitch/2) * cos(roll/2) + sin(yaw/2) * sin(pitch/2) * sin(roll/2)
		return v;
#endif
	}

	/* MATRIX */

	/** A type representing a 4x4 matrix. */
	typedef struct {
#ifdef __SSE__
		__m128 row0, /**< The first row/column. */
			row1, /**< The second row/column. */
			row2, /**< The third row/column. */
			row3; /**< The fourth row/column. */
#else
		float m[16]; /**< A 16 elements long float vector containing the components. */
#endif
	} MAT;

	/** Stores a representation of the matrix \a _A in the float array \a _V and returns \a _V.
	@def MatrixGet(_V, _A)
	@param[out] _V A 16 elements long 16-byte aligned float array to store in.
	@param[in] _A The matrix to be stored. Note: not a pointer as the rest of the matrix functions.

	This method is to be used when the data of a ::MAT needs to be converted to a more general, usable format. */
#ifdef __SSE__
#define MatrixGet(_V, _A) (_mm_store_ps((_V), (_A).row0), _mm_store_ps((_V) + 4, (_A).row1), _mm_store_ps((_V) + 8, (_A).row2), _mm_store_ps((_V) + 12, (_A).row3), (_V))
#else
#define MatrixGet(_V, _A) (memcpy((_V), (_A).m, sizeof(float) * 16), (_V))
#endif

	/** Returns a new ::MAT from the specified components. */
	VMATH_INLINE MAT MatrixSet(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33) {
#ifdef __SSE__
		MAT m = { _mm_setr_ps(m00, m01, m02, m03), _mm_setr_ps(m10, m11, m12, m13), _mm_setr_ps(m20, m21, m22, m23), _mm_setr_ps(m30, m31, m32, m33) };
#else
		MAT m = { m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33 };
#endif
		return m;
	}

	/** Loads and returns a ::MAT from the float array \a v.
		@param v The float array to load up. */
	VMATH_INLINE MAT MatrixLoad(float *v) {
#ifdef __SSE__
		MAT m = { _mm_load_ps(v), _mm_load_ps(v + 4), _mm_load_ps(v + 8), _mm_load_ps(v + 12) };
#else
		MAT m = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15] };
#endif
		return m;
	}

	/** Returns the identity matrix (I). */
	VMATH_INLINE MAT MatrixIdentity() {
#ifdef __SSE__
		MAT m = { _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_setr_ps(0, 0, 0, 1) };
#else
		MAT m = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
#endif
		return m;
	}

	/** Returns a perspective matrix.
		@param fov The field of vision in degrees.
		@param aspect The aspect ratio of the screen.
		@param zNear The near coordinate of the z-plane.
		@param zFar The far coordinate of the z-plane. */
	VMATH_INLINE MAT MatrixPerspective(float fov, float aspect, float zNear, float zFar) {
		const float h = 1.0f / tan(fov * PI / 360);
#ifdef __SSE__
		MAT m = { _mm_setr_ps(h / aspect, 0, 0, 0), _mm_setr_ps(0, h, 0, 0), _mm_setr_ps(0, 0, (zNear + zFar) / (zNear - zFar), -1), _mm_setr_ps(0, 0, 2 * (zNear * zFar) / (zNear - zFar), 0) };
#else
		MAT m = { h / aspect, 0, 0, 0, 0, h, 0, 0, 0, 0, (zNear + zFar) / (zNear - zFar), -1, 0, 0, 2 * (zNear * zFar) / (zNear - zFar), 0 };
#endif
		return m;
	}

	/** Multiplies the two matrices \a a and \a b (a * b).
		@param a The first matrix to multiply.
		@param b The second matrix to multiply.
		@return The product of the two matrices.
		@warning The two matrices must be distinct, the result will be incorrect if \a a or \a b are equal. */
	VMATH_INLINE MAT MatrixMultiply(MAT *a, MAT *b) {
#ifdef __SSE__
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

		MAT m = { row0, row1, row2, row3 };
		return m;
#else
		MAT result;
		result.m[M_00] = a->m[M_00] * b->m[M_00] + a->m[M_01] * b->m[M_10] + a->m[M_02] * b->m[M_20] + a->m[M_03] * b->m[M_30];
		result.m[M_01] = a->m[M_00] * b->m[M_01] + a->m[M_01] * b->m[M_11] + a->m[M_02] * b->m[M_21] + a->m[M_03] * b->m[M_31];
		result.m[M_02] = a->m[M_00] * b->m[M_02] + a->m[M_01] * b->m[M_12] + a->m[M_02] * b->m[M_22] + a->m[M_03] * b->m[M_32];
		result.m[M_03] = a->m[M_00] * b->m[M_03] + a->m[M_01] * b->m[M_13] + a->m[M_02] * b->m[M_23] + a->m[M_03] * b->m[M_33];
		result.m[M_10] = a->m[M_10] * b->m[M_00] + a->m[M_11] * b->m[M_10] + a->m[M_12] * b->m[M_20] + a->m[M_13] * b->m[M_30];
		result.m[M_11] = a->m[M_10] * b->m[M_01] + a->m[M_11] * b->m[M_11] + a->m[M_12] * b->m[M_21] + a->m[M_13] * b->m[M_31];
		result.m[M_12] = a->m[M_10] * b->m[M_02] + a->m[M_11] * b->m[M_12] + a->m[M_12] * b->m[M_22] + a->m[M_13] * b->m[M_32];
		result.m[M_13] = a->m[M_10] * b->m[M_03] + a->m[M_11] * b->m[M_13] + a->m[M_12] * b->m[M_23] + a->m[M_13] * b->m[M_33];
		result.m[M_20] = a->m[M_20] * b->m[M_00] + a->m[M_21] * b->m[M_10] + a->m[M_22] * b->m[M_20] + a->m[M_23] * b->m[M_30];
		result.m[M_21] = a->m[M_20] * b->m[M_01] + a->m[M_21] * b->m[M_11] + a->m[M_22] * b->m[M_21] + a->m[M_23] * b->m[M_31];
		result.m[M_22] = a->m[M_20] * b->m[M_02] + a->m[M_21] * b->m[M_12] + a->m[M_22] * b->m[M_22] + a->m[M_23] * b->m[M_32];
		result.m[M_23] = a->m[M_20] * b->m[M_03] + a->m[M_21] * b->m[M_13] + a->m[M_22] * b->m[M_23] + a->m[M_23] * b->m[M_33];
		result.m[M_30] = a->m[M_30] * b->m[M_00] + a->m[M_31] * b->m[M_10] + a->m[M_32] * b->m[M_20] + a->m[M_33] * b->m[M_30];
		result.m[M_31] = a->m[M_30] * b->m[M_01] + a->m[M_31] * b->m[M_11] + a->m[M_32] * b->m[M_21] + a->m[M_33] * b->m[M_31];
		result.m[M_32] = a->m[M_30] * b->m[M_02] + a->m[M_31] * b->m[M_12] + a->m[M_32] * b->m[M_22] + a->m[M_33] * b->m[M_32];
		result.m[M_33] = a->m[M_30] * b->m[M_03] + a->m[M_31] * b->m[M_13] + a->m[M_32] * b->m[M_23] + a->m[M_33] * b->m[M_33];
		return result;
#endif
	}

	/** Transposes the matrix \a a (a<sup>T</sup>). */
	VMATH_INLINE MAT MatrixTranspose(MAT *a) {
#ifdef __SSE4_1__
		__m128 tmp0 = _mm_unpacklo_ps(a->row0, a->row1), tmp2 = _mm_unpacklo_ps(a->row2, a->row3), tmp1 = _mm_unpackhi_ps(a->row0, a->row1), tmp3 = _mm_unpackhi_ps(a->row2, a->row3);
		MAT m = { _mm_movelh_ps(tmp0, tmp2), _mm_movehl_ps(tmp2, tmp0), _mm_movelh_ps(tmp1, tmp3), _mm_movehl_ps(tmp3, tmp1) };
		return m;
#else
		// A B C D    A E I M
		// E F G H    B F J N
		// I J K L    C G K O
		// M N O P    D H L P
		MAT m;
		m.m[0] = a->m[0];  // A . . .
		m.m[1] = a->m[4];  // A E . .
		m.m[2] = a->m[8];  // A E I .
		m.m[3] = a->m[12]; // A E I M
		m.m[4] = a->m[1];  // B . . .
		m.m[5] = a->m[5];  // B F . .
		m.m[6] = a->m[9];  // B F J .
		m.m[7] = a->m[13]; // B F J N
		m.m[8] = a->m[2];  // C . . .
		m.m[9] = a->m[6];  // C G . .
		m.m[10] = a->m[10]; // C G K .
		m.m[11] = a->m[14]; // C G K O
		m.m[12] = a->m[3];  // D . . .
		m.m[13] = a->m[7];  // D H . .
		m.m[14] = a->m[11]; // D H L .
		m.m[15] = a->m[15]; // D H L P
		return m;
#endif
	}

	/** Inverses the matrix \a a using Cramer's rule (a<sup>-1</sup>). */
	VMATH_INLINE MAT MatrixInverse(MAT *a) {
#ifdef __SSE__
		__m128 minor0, minor1, minor2, minor3,
			row0, row1, row2, row3,
			det, tmp1;
#if MATRIX_ORDER == ROW_MAJOR
		MAT tmp = MatrixTranspose(a);
		a = &tmp;
#endif
		row0 = a->row0;
		row1 = _mm_shuffle_ps(a->row1, a->row1, 0x4E);
		row2 = a->row2;
		row3 = _mm_shuffle_ps(a->row3, a->row3, 0x4E);
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
		MAT m = { _mm_mul_ps(det, minor0), _mm_mul_ps(det, minor1), _mm_mul_ps(det, minor2), _mm_mul_ps(det, minor3) };
		return m;
#else
		double inv[16], det;
		inv[0] = a->m[5] * a->m[10] * a->m[15] - a->m[5] * a->m[11] * a->m[14] - a->m[9] * a->m[6] * a->m[15] + a->m[9] * a->m[7] * a->m[14] + a->m[13] * a->m[6] * a->m[11] - a->m[13] * a->m[7] * a->m[10];
		inv[4] = -a->m[4] * a->m[10] * a->m[15] + a->m[4] * a->m[11] * a->m[14] + a->m[8] * a->m[6] * a->m[15] - a->m[8] * a->m[7] * a->m[14] - a->m[12] * a->m[6] * a->m[11] + a->m[12] * a->m[7] * a->m[10];
		inv[8] = a->m[4] * a->m[9] * a->m[15] - a->m[4] * a->m[11] * a->m[13] - a->m[8] * a->m[5] * a->m[15] + a->m[8] * a->m[7] * a->m[13] + a->m[12] * a->m[5] * a->m[11] - a->m[12] * a->m[7] * a->m[9];
		inv[12] = -a->m[4] * a->m[9] * a->m[14] + a->m[4] * a->m[10] * a->m[13] + a->m[8] * a->m[5] * a->m[14] - a->m[8] * a->m[6] * a->m[13] - a->m[12] * a->m[5] * a->m[10] + a->m[12] * a->m[6] * a->m[9];
		inv[1] = -a->m[1] * a->m[10] * a->m[15] + a->m[1] * a->m[11] * a->m[14] + a->m[9] * a->m[2] * a->m[15] - a->m[9] * a->m[3] * a->m[14] - a->m[13] * a->m[2] * a->m[11] + a->m[13] * a->m[3] * a->m[10];
		inv[5] = a->m[0] * a->m[10] * a->m[15] - a->m[0] * a->m[11] * a->m[14] - a->m[8] * a->m[2] * a->m[15] + a->m[8] * a->m[3] * a->m[14] + a->m[12] * a->m[2] * a->m[11] - a->m[12] * a->m[3] * a->m[10];
		inv[9] = -a->m[0] * a->m[9] * a->m[15] + a->m[0] * a->m[11] * a->m[13] + a->m[8] * a->m[1] * a->m[15] - a->m[8] * a->m[3] * a->m[13] - a->m[12] * a->m[1] * a->m[11] + a->m[12] * a->m[3] * a->m[9];
		inv[13] = a->m[0] * a->m[9] * a->m[14] - a->m[0] * a->m[10] * a->m[13] - a->m[8] * a->m[1] * a->m[14] + a->m[8] * a->m[2] * a->m[13] + a->m[12] * a->m[1] * a->m[10] - a->m[12] * a->m[2] * a->m[9];
		inv[2] = a->m[1] * a->m[6] * a->m[15] - a->m[1] * a->m[7] * a->m[14] - a->m[5] * a->m[2] * a->m[15] + a->m[5] * a->m[3] * a->m[14] + a->m[13] * a->m[2] * a->m[7] - a->m[13] * a->m[3] * a->m[6];
		inv[6] = -a->m[0] * a->m[6] * a->m[15] + a->m[0] * a->m[7] * a->m[14] + a->m[4] * a->m[2] * a->m[15] - a->m[4] * a->m[3] * a->m[14] - a->m[12] * a->m[2] * a->m[7] + a->m[12] * a->m[3] * a->m[6];
		inv[10] = a->m[0] * a->m[5] * a->m[15] - a->m[0] * a->m[7] * a->m[13] - a->m[4] * a->m[1] * a->m[15] + a->m[4] * a->m[3] * a->m[13] + a->m[12] * a->m[1] * a->m[7] - a->m[12] * a->m[3] * a->m[5];
		inv[14] = -a->m[0] * a->m[5] * a->m[14] + a->m[0] * a->m[6] * a->m[13] + a->m[4] * a->m[1] * a->m[14] - a->m[4] * a->m[2] * a->m[13] - a->m[12] * a->m[1] * a->m[6] + a->m[12] * a->m[2] * a->m[5];
		inv[3] = -a->m[1] * a->m[6] * a->m[11] + a->m[1] * a->m[7] * a->m[10] + a->m[5] * a->m[2] * a->m[11] - a->m[5] * a->m[3] * a->m[10] - a->m[9] * a->m[2] * a->m[7] + a->m[9] * a->m[3] * a->m[6];
		inv[7] = a->m[0] * a->m[6] * a->m[11] - a->m[0] * a->m[7] * a->m[10] - a->m[4] * a->m[2] * a->m[11] + a->m[4] * a->m[3] * a->m[10] + a->m[8] * a->m[2] * a->m[7] - a->m[8] * a->m[3] * a->m[6];
		inv[11] = -a->m[0] * a->m[5] * a->m[11] + a->m[0] * a->m[7] * a->m[9] + a->m[4] * a->m[1] * a->m[11] - a->m[4] * a->m[3] * a->m[9] - a->m[8] * a->m[1] * a->m[7] + a->m[8] * a->m[3] * a->m[5];
		inv[15] = a->m[0] * a->m[5] * a->m[10] - a->m[0] * a->m[6] * a->m[9] - a->m[4] * a->m[1] * a->m[10] + a->m[4] * a->m[2] * a->m[9] + a->m[8] * a->m[1] * a->m[6] - a->m[8] * a->m[2] * a->m[5];

		det = a->m[0] * inv[0] + a->m[1] * inv[4] + a->m[2] * inv[8] + a->m[3] * inv[12];
		if (det == 0) return *a; // assert(det == 0 && "Non-invertible matrix");
		det = 1.0 / det;

		MAT m;
		for (int i = 0; i < 16; i++) m.m[i] = inv[i] * det;
		return m;
#endif
	}

	/** Builds a translation matrix from the specified offsets.
		@param x The translation along the x-axis
		@param y The translation along the y-axis
		@param z The translation along the z-axis
		@return The translation matrix */
	VMATH_INLINE MAT MatrixTranslation(float x, float y, float z) {
#ifdef __SSE__
		MAT m = { _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_setr_ps(x, y, z, 1) };
#else
		MAT m = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1 };
#endif
		return m;
	}

	/** Builds a translation matrix from a vector.
		@param v 3D vector describing the translations along the x-axis, y-axis, and z-axis
		@return The translation matrix */
	VMATH_INLINE MAT MatrixTranslationFromVector(VEC v) {
#ifdef __SSE__
		__m128 t = _mm_move_ss(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 1, 0, 3)), _mm_set1_ps(1));
		MAT m = { _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_shuffle_ps(t, t, _MM_SHUFFLE(0, 3, 2, 1)) };
#else
		MAT m = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, v.v[0], v.v[1], v.v[2], v.v[3] };
#endif
		return m;
	}

	/** Builds a matrix that scales along the x-axis, y-axis, and z-axis.
		@param x Scaling factor along the x-axis
		@param y Scaling factor along the x-axis
		@param z Scaling factor along the x-axis
		@return The scaling	matrix */
	VMATH_INLINE MAT MatrixScaling(float x, float y, float z) {
#ifdef __SSE__
		MAT m = { _mm_setr_ps(x, 0, 0, 0), _mm_setr_ps(0, y, 0, 0), _mm_setr_ps(0, 0, z, 0), _mm_setr_ps(0, 0, 0, 1) };
#else
		MAT m = { x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1 };
#endif
		return m;
	}

	/** Builds a rotation matrix from the quaternion \a a.
		@param a Quaternion defining the rotation.
		@return The rotation matrix */
	VMATH_INLINE MAT MatrixRotationQuaternion(VEC a) {
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
#ifdef __SSE__
		MAT m = { _mm_setr_ps(1 - 2 * (qyy + qzz), 2 * (qxy + qwz), 2 * (qxz - qwy), 0),
			_mm_setr_ps(2 * (qxy - qwz), 1 - 2 * (qxx + qzz), 2 * (qyz + qwx), 0),
			_mm_setr_ps(2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx + qyy), 0),
			_mm_setr_ps(0, 0, 0, 1) };
#else
		MAT m = { 1 - 2 * (qyy + qzz), 2 * (qxy + qwz), 2 * (qxz - qwy), 0, 2 * (qxy - qwz), 1 - 2 * (qxx + qzz), 2 * (qyz + qwx), 0, 2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx + qyy), 0, 0, 0, 0, 1 };
#endif
		return m;
	}

	/** Compare the elements in \a a and \a b and return a bit mask where \c 1 corresponds to equality.
		@param a The vector to compare
		@param b The vector to compare
		@return The result as a bit mask */
	VMATH_INLINE int MatrixEqual(MAT *a, MAT *b) {
#ifdef __SSE__
		return _mm_movemask_ps(_mm_cmpeq_ps(a->row0, b->row0)) | (_mm_movemask_ps(_mm_cmpeq_ps(a->row1, b->row1)) << 4) | (_mm_movemask_ps(_mm_cmpeq_ps(a->row2, b->row2)) << 8) | (_mm_movemask_ps(_mm_cmpeq_ps(a->row3, b->row3)) << 12);
#else
		return (a.m[0] == b.m[0] ? 1 : 0) | (a.m[1] == b.m[1] ? 1 << 1 : 0) | (a.m[2] == b.m[2] ? 1 << 2 : 0) | (a.m[3] == b.m[3] ? 1 << 3 : 0)
			| (a.m[4] == b.m[4] ? 1 << 4 : 0) | (a.m[5] == b.m[5] ? 1 << 5 : 0) | (a.m[6] == b.m[6] ? 1 << 6 : 0) | (a.m[7] == b.m[7] ? 1 << 7 : 0)
			| (a.m[8] == b.m[8] ? 1 << 8 : 0) | (a.m[9] == b.m[9] ? 1 << 9 : 0) | (a.m[10] == b.m[10] ? 1 << 10 : 0) | (a.m[11] == b.m[11] ? 1 << 11 : 0)
			| (a.m[12] == b.m[12] ? 1 << 12 : 0) | (a.m[13] == b.m[13] ? 1 << 13 : 0) | (a.m[14] == b.m[14] ? 1 << 14 : 0) | (a.m[15] == b.m[15] ? 1 << 15 : 0);
#endif
	}

#ifdef __cplusplus
}
#endif

#endif