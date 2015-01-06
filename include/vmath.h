/** Vector math library with SIMD acceleration.
	\file vmath.h */

// TODO rename __SSE__ to SIMD_INTRINSICS
// TODO make cmake set the variables __SSE__, __SSE2__, __SSE3, __SSE4__

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
#endif
#elif defined __GNUC__
#define ALIGN(i)  __attribute__ ((aligned (i)))
#endif
	// if __ARM_NEON__ or __SSE__ or __ALTIVEC__
#define VMATH_INLINE inline /**< Inlining. */
	/** A scalar that can be be used for multiplying vectors. */
	typedef
#ifdef USE_DOUBLE_PRECISION
		double
#else
		float
#endif
		scalar;
	/** \def ROW_MAJOR_MATRIX
	\brief Whether ::MAT is row-major order in memory. */
	/** \def COLUMN_MAJOR_MATRIX
	\brief Whether ::MAT is column-major order in memory. */
#if !defined(ROW_MAJOR_MATRIX) && !defined(COLUMN_MAJOR_MATRIX)
#define ROW_MAJOR_MATRIX
#endif
#ifdef __DOXYGEN__
#define COLUMN_MAJOR_MATRIX
#define NO_SIMD_INTRINSICS /**< If defined compiled code won't be SIMD accelerated, even if availible. */
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

#define M00 0 /**< XX. */
#define M01 4 /**< XY. */
#define M02 8 /**< XZ. */
#define M03 12 /**< XW. */
#define M10 1 /**< YX. */
#define M11 5 /**< YY. */
#define M12 9 /**< YZ. */
#define M13 13 /**< YW. */
#define M20 2 /**< ZX. */
#define M21 6 /**< ZY. */
#define M22 10 /**< ZZ. */
#define M23 14 /**< ZW. */
#define M30 3 /**< WX. */
#define M31 7 /**< WY. */
#define M32 11 /**< WZ. */
#define M33 15 /**< WW. */

	/* VECTOR */

	/** A type representing a four-dimensional vector. */
	typedef
#ifdef __SSE__
		__m128
#else
	union {
		float v[4]; /**< A 4 elements long float vector containing the components. */
		struct {
			float x, /**< The x component of the vector. */
				y, /**< The y component of the vector. */
				z, /**< The z component of the vector. */
				w; /**< The w component of the vector. */
		}
	}
#endif
	VEC;

	/** Returns a ::VEC, whoose components are solely \a v.
		@param v The value to use for the components. */
	VMATH_INLINE VEC VectorReplicate(float v) {
#ifdef __SSE__
		return(_mm_set1_ps(v));
#else
		VEC v = { v, v, v, v };
		return(v);
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
		VEC v = { w, z, y, x };
		return(v);
#endif
	}

	/** Loads and returns a ::VEC from the float array \a v.
		@param v The float array to load up. */
	VMATH_INLINE VEC VectorLoad(float *v) {
#ifdef __SSE__
		return(_mm_load_ps(v));
#else
		VEC v = { v[0], v[1], v[2], v[3] };
		return(v);
#endif
	}

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

	/** Returns the scalar length of the vector \a v (||a||). */
	VMATH_INLINE float VectorLength(VEC v) {
#ifdef __SSE4_1__
		return(_mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v, v, 0x71))));
#else
		return((float) sqrt(v.v[0] * v.v[0] + v.v[1] * v.v[1] + v.v[2] * v.v[2] + v.v[3] * v.v[3]));
#endif
	}

	/** Normalizes the vector \a v (â). */
	VMATH_INLINE VEC VectorNormalize(VEC v) {
#ifdef __SSE4_1__
		return(_mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0x7F /* 0x77 */))));
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
		return(_mm_mul_ps(vec0, v));
#else
		float length = VectorLength(v);
		VEC v = { v.v[0] / length, v.v[1] / length, v.v[2] / length, v.v[3] / length };
		return v;
#endif
	}
	
	/** Returns the dot product, a.k.a. the scalar product, of the two vectors \a a and \a b (a . b). */
	VMATH_INLINE float VectorDot(VEC a, VEC b) {
#if defined(__SSE4_1__)
		return(_mm_cvtss_f32(_mm_dp_ps(a, b, 0x71)));
#elif defined(__SSE3__)
		__m128 r1 = _mm_mul_ps(a, b), r2 = _mm_hadd_ps(r1, r1), r3 = _mm_hadd_ps(r2, r2);
		return(_mm_cvtss_f32(r3));
#elif defined(__SSE__)
		__m128 m = _mm_mul_ps(v1, v2), t = _mm_add_ps(m, _mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 3, 0, 1)));
		return(_mm_cvtss_f32(_mm_add_ps(t, _mm_shuffle_ps(t, t, _MM_SHUFFLE(1, 0, 3, 2)))));
#else
		return(a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2] + a.v[3] * b.v[3]);
#endif
	}

	/** Returns the cross product, a.k.a. the vector product, of the two vectors \a a and \a b (a × b). */
	VMATH_INLINE VEC VectorCross(VEC a, VEC b) {
#ifdef __SSE__
		return(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2))),_mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1)))));
#else
		VEC v = { a.v[1] * b.v[2] - a.v[2] * b.v[1],
			a.v[2] * b.v[0] - a.v[0] * b.v[2],
			a.v[0] * b.v[1] - a.v[1] * b.v[0],
			0};
		return v;
#endif
	}
	
	/** Returns `0` if, and only if, any component of the two vectors \a a and \a b mismatch. */
	VMATH_INLINE int VectorEqual(VEC a, VEC b) {
#ifdef __SSE__
		return _mm_movemask_ps(_mm_cmpeq_ps(a, b));
#else
		return a.v[0] == b.v[0] && a.v[1] == b.v[1] && a.v[2] == b.v[2] && a.v[3] == b.v[3]; // TODO rewrite for floating point comparison
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

		return(_mm_setr_ps(
			(chy_shp * chr) + (shy_chp * shr), // cos(yaw/2) * sin(pitch/2) * cos(roll/2) + sin(yaw/2) * cos(pitch/2) * sin(roll/2)
			(shy_chp * chr) - (chy_shp * shr), // sin(yaw/2) * cos(pitch/2) * cos(roll/2) - cos(yaw/2) * sin(pitch/2) * sin(roll/2)
			(chy_chp * shr) - (shy_shp * chr), // cos(yaw/2) * cos(pitch/2) * sin(roll/2) - sin(yaw/2) * sin(pitch/2) * cos(roll/2)
			(chy_chp * chr) + (shy_shp * shr) // cos(yaw/2) * cos(pitch/2) * cos(roll/2) + sin(yaw/2) * sin(pitch/2) * sin(roll/2)
			));
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

	/** Returns a new ::MAT from the specified components. */
	VMATH_INLINE MAT MatrixSet(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33) {
#ifdef __SSE__
		MAT m = { _mm_setr_ps(m00, m01, m02, m03), _mm_setr_ps(m10, m11, m12, m13), _mm_setr_ps(m20, m21, m22, m23), _mm_setr_ps(m30, m31, m32, m33) };
		return m;
#endif
	}

	/** Loads and returns a ::MAT from the float array \a v.
		@param v The float array to load up. */
	VMATH_INLINE MAT MatrixLoad(float *v) {
		MAT m = { _mm_load_ps(v), _mm_load_ps(v + 4), _mm_load_ps(v + 8), _mm_load_ps(v + 12) };
		return m;
	}

	/** Returns the identity matrix (I). */
	VMATH_INLINE MAT MatrixIdentity() {
#ifdef __SSE__
		MAT m = { _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_setr_ps(0, 0, 0, 1) };
		return m;
#else
		return 0;
#endif
	}

	/** Returns a perspective matrix.
		@param fov The field of vision in degrees.
		@param aspect The aspect ratio of the screen.
		@param zNear The near coordinate of the z-plane.
		@param zFar The far coordinate of the z-plane. */
	VMATH_INLINE MAT MatrixPerspective(float fov, float aspect, float zNear, float zFar) {
#ifdef __SSE__
		const float h = 1.0f / tan(fov * PI / 360);
		MAT m = { _mm_setr_ps(h / aspect, 0, 0, 0), _mm_setr_ps(0, h, 0, 0), _mm_setr_ps(0, 0, (zNear + zFar) / (zNear - zFar), -1), _mm_setr_ps(0, 0, 2 * (zNear * zFar) / (zNear - zFar), 0) };
		return m;
#endif
	}

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

	/** Transposes the matrix \a a. */
	VMATH_INLINE MAT MatrixTranspose(MAT *a) {
		__m128 tmp0 = _mm_unpacklo_ps(a->row0, a->row1), tmp2 = _mm_unpacklo_ps(a->row2, a->row3), tmp1 = _mm_unpackhi_ps(a->row0, a->row1), tmp3 = _mm_unpackhi_ps(a->row2, a->row3);
		MAT m = { _mm_movelh_ps(tmp0, tmp2), _mm_movehl_ps(tmp2, tmp0), _mm_movelh_ps(tmp1, tmp3), _mm_movehl_ps(tmp3, tmp1) };
		return m;
	}

	/** Inverses the matrix \a a using Cramer's rule (a<sup>-1</sup>). */
	VMATH_INLINE MAT MatrixInverse(MAT *a) {
#ifdef __SSE__
		__m128 minor0, minor1, minor2, minor3,
			row0, row1, row2, row3,
			det, tmp1;
#ifdef ROW_MAJOR_MATRIX
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
#endif
	}

	/** Returns a translation matrix. */
	VMATH_INLINE MAT MatrixTranslate(MAT *a, float x, float y, float z) {
		MAT m = { _mm_setr_ps(1, 0, 0, 0), _mm_setr_ps(0, 1, 0, 0), _mm_setr_ps(0, 0, 1, 0), _mm_setr_ps(x, y, z, 1) };
		return m;
	}

	/** Returns a scale transformation matrix. */
	VMATH_INLINE MAT MatrixScale(float x, float y, float z) {
		MAT m = { _mm_setr_ps(x, 0, 0, 0), _mm_setr_ps(0, y, 0, 0), _mm_setr_ps(0, 0, z, 0), _mm_setr_ps(x, y, z, 1) };
		return m;
	}

	/** Returns a matrix based on the quaternion \a a. */
	VMATH_INLINE MAT QuaternionToMatrix(VEC a) {
#ifdef __SSE__
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

		MAT m = { _mm_setr_ps(1 - 2 * (qyy + qzz), 2 * (qxy + qwz), 2 * (qxz - qwy), 0),
			_mm_setr_ps(2 * (qxy - qwz), 1 - 2 * (qxx + qzz), 2 * (qyz + qwx), 0),
			_mm_setr_ps(2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx + qyy), 0),
			_mm_setr_ps(0, 0, 0, 1) };
		return m;
#endif
	}

#ifdef __cplusplus
}
#endif

#endif