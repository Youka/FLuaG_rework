/*
Project: FLuaG
File: math.hpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <functional>

// Useful math defines (not standard)
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_SQRT2		1.41421356237309504880

#ifndef POINT_ON_LINE_TOLERANCE
	#define POINT_ON_LINE_TOLERANCE 0.00001
#endif

namespace Math{
	template<typename T>
	inline char sign(const T x) noexcept{
		return x < 0 ? -1 : (x > 0 ? 1 : 0);
	}

	inline int fac(int n) noexcept{
		int k = 1;
		while(n > 1)
			k *= n--;
		return k;
	}

	inline double bin_coeff(const int i, const int n) noexcept{
		return fac(n) / (fac(i) * fac(n-i));
	}

	inline double bernstein(const int i, const int n, const double t) noexcept{
		return bin_coeff(i, n) * std::pow(t, i) * std::pow(1-t, n-i);
	}
}

namespace Geometry{
	// 2D
	struct Point2d{
		double x, y;
		Point2d operator+(const Point2d& other) const noexcept{return {this->x + other.x, this->y + other.y};}
		Point2d operator-(const Point2d& other) const noexcept{return {this->x - other.x, this->y - other.y};}
		Point2d operator*(const Point2d& other) const noexcept{return {this->x * other.x, this->y * other.y};}
		Point2d operator/(const Point2d& other) const noexcept{return {this->x / other.x, this->y / other.y};}
		Point2d operator-() const noexcept{return {-this->x, -this->y};}
		bool operator==(const Point2d& other) const noexcept{return this->x == other.x && this->y == other.y;}
		bool operator!=(const Point2d& other) const noexcept{return !(*this == other);}
	};

	inline double normal_z(const Point2d& v1, const Point2d& v2) noexcept{
		return v1.x * v2.y - v1.y * v2.x;
	}
	inline double normal_z(const Point2d& p0, const Point2d& p1, const Point2d& p2) noexcept{
		return normal_z(p1 - p0, p2 - p1);
	}

	inline bool on_line(const Point2d& p, const Point2d& l0, const Point2d& l1) noexcept{
		return std::abs(std::hypot(p.x-l0.x, p.y-l0.y) + std::hypot(p.x-l1.x, p.y-l1.y) - std::hypot(l1.x-l0.x, l1.y-l0.y)) < POINT_ON_LINE_TOLERANCE;
	}

	inline Point2d line_intersect(const Point2d& l1p1, const Point2d& l1p2, const Point2d& l2p1, const Point2d& l2p2){
		// Check direction
		const Point2d l1 = l1p1 - l1p2,
			l2 = l2p1 - l2p2;
		const double det = normal_z(l1, l2);
		if(det == 0)
			throw std::out_of_range("Lines parallel!");
		// Get intersection point
		const double pre = normal_z(l1p1, l1p2),
			post = normal_z(l2p1, l2p2);
		const Point2d lx = (Point2d{pre,pre} * l2 - l1 * Point2d{post,post}) / Point2d{det,det};
		// Check point is on lines
		if(!(on_line(lx, l1p1, l1p2) && on_line(lx, l2p1, l2p2)))
			throw std::length_error("Line intersection not on both lines!");
		// Return intersection
		return lx;
	}

	inline bool in_triangle(const Point2d& p, const Point2d& t1, const Point2d& t2, const Point2d& t3) noexcept{
		const char t2t3p = Math::sign(normal_z(t2, t3, p));
		return Math::sign(normal_z(t1, t2, p)) == t2t3p && t2t3p == Math::sign(normal_z(t3, t1, p));
	}

	inline Point2d stretch(const Point2d& v, const double new_length) noexcept{
		double factor;
		return v.x == 0 && v.y == 0 ? v : (factor = new_length / std::hypot(v.x, v.y), Point2d{factor,factor} * v);
	}

	inline Point2d rotate(const Point2d& v, const double angle) noexcept{
		const double sin_angle = std::sin(angle),
			cos_angle = std::cos(angle);
		return {cos_angle * v.x - sin_angle * v.y, sin_angle * v.x + cos_angle * v.y};
	}

	inline std::vector<std::array<Point2d,4>> arc_to_curves(const Point2d& start, const Point2d& center, const double angle) noexcept{
		// Output buffer
		std::vector<std::array<Point2d,4>> curves;
		// Anything to do?
		if(angle != 0){
			// Save constants
			constexpr static const double kappa = 4 * (M_SQRT2 - 1) / 3;
			const char direction = Math::sign(angle);
			const double angle_abs = std::abs(angle);
			// Go through 1/4 circle pieces
			Point2d rel_start = start - center;
			for(double angle_sum = 0; angle_sum < angle_abs; angle_sum += M_PI_2){	// 90° steps
				// Arc size
				const double current_angle = std::min(angle_abs - angle_sum, M_PI_2);
				// Get arc end point
				const Point2d rel_end = rotate(rel_start, direction * current_angle);
				// Get arc start-to-end vector & scale for control points
				Point2d rel_start_end = rel_end - rel_start;
				rel_start_end = stretch(rel_start_end, std::sqrt((rel_start_end.x*rel_start_end.x + rel_start_end.y*rel_start_end.y) * 0.5) * kappa);
				// Get arc control points
				const Point2d rel_control1 = rel_start + rotate(rel_start_end, direction * current_angle * -0.5),
					rel_control2 = rel_end + rotate(-rel_start_end, direction * current_angle * 0.5);
				// Insert arc to output
				curves.push_back({center + rel_start, center + rel_control1, center + rel_control2, center + rel_end});
				// Prepare next arc
				rel_start = rel_end;
			}
		}
		// Return what was collected
		return curves;
	}

	inline std::vector<Point2d> curve_flatten(const std::array<Point2d,4> points, const double tolerance) noexcept{
		// Check valid arguments
		if(tolerance <= 0)
			throw std::out_of_range("Invalid tolerance!");
		// Helper functions
		auto curve_split = [](const std::array<Point2d,4>& points) -> std::array<Point2d,8>{
			static const Point2d half{0.5, 0.5};
			const Point2d p01 = (points[0] + points[1]) * half,
				p12 = (points[1] + points[2]) * half,
				p23 = (points[2] + points[3]) * half,
				p012 = (p01 + p12) * half,
				p123 = (p12 + p23) * half,
				p0123 = (p012 + p123) * half;
			return {points[0], p01, p012, p0123, p0123, p123, p23, points[3]};
		};
		auto curve_is_flat = [tolerance](const std::array<Point2d,4>& points){
			std::array<Point2d,3> vecs{{
				points[1] - points[0],
				points[2] - points[1],
				points[3] - points[2]
			}};
			const auto vecs_end = std::remove(vecs.begin(), vecs.end(), Point2d{0,0});
			if(vecs.cbegin()+2 <= vecs_end)
				for(auto it = vecs.cbegin()+1; it != vecs_end; ++it)
					if(std::abs(normal_z(*(it-1), *it)) > tolerance)
						return false;
			return true;
		};
		std::function<void(const std::array<Point2d,4>&, std::vector<Point2d>&)> curve_to_lines;
		curve_to_lines = [&curve_to_lines,&curve_split,&curve_is_flat](const std::array<Point2d,4>& points, std::vector<Point2d>& out){
			if(curve_is_flat(points))
				out.push_back(points.back());
			else{
				const auto& points2 = curve_split(points);
				curve_to_lines({points2[0], points2[1], points2[2], points2[3]}, out);
				curve_to_lines({points2[4], points2[5], points2[6], points2[7]}, out);
			}
		};
		// Convert curve to lines
		std::vector<Point2d> lines{points.front()};
		curve_to_lines(points, lines);
		return lines;
	}

	// 4D
	template<typename T>
	class Matrix4x4 : public std::array<T,16>{
		public:
			// Transparent template type
			using super_t = std::array<T,16>;
			// Ctors
			Matrix4x4() noexcept{this->identity();}
			Matrix4x4(const T* p) noexcept{std::copy(p, p+16, this->begin());}
			Matrix4x4(const super_t& arr) noexcept : super_t(arr){}
			Matrix4x4(const T x11, const T x12, const T x13, const T x14,
					const T x21, const T x22, const T x23, const T x24,
					const T x31, const T x32, const T x33, const T x34,
					const T x41, const T x42, const T x43, const T x44) noexcept
					: super_t({x11, x12, x13, x14,
							x21, x22, x23, x24,
							x31, x32, x33, x34,
							x41, x42, x43, x44}){}
			// Unary methods
			void identity() noexcept{
				*this = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
			}
			bool invert() noexcept{
				const super_t inv_matrix = {
				/* 11 */	(*this)[5]*(*this)[10]*(*this)[15] + (*this)[6]*(*this)[11]*(*this)[13] + (*this)[7]*(*this)[9]*(*this)[14] - (*this)[5]*(*this)[11]*(*this)[14] - (*this)[6]*(*this)[9]*(*this)[15] - (*this)[7]*(*this)[10]*(*this)[13],
				/* 12 */	(*this)[1]*(*this)[11]*(*this)[14] + (*this)[2]*(*this)[9]*(*this)[15] + (*this)[3]*(*this)[10]*(*this)[13] - (*this)[1]*(*this)[10]*(*this)[15] - (*this)[2]*(*this)[11]*(*this)[13] - (*this)[3]*(*this)[9]*(*this)[14],
				/* 13 */	(*this)[1]*(*this)[6]*(*this)[15] + (*this)[2]*(*this)[7]*(*this)[13] + (*this)[3]*(*this)[5]*(*this)[14] - (*this)[1]*(*this)[7]*(*this)[14] - (*this)[2]*(*this)[5]*(*this)[15] - (*this)[3]*(*this)[6]*(*this)[13],
				/* 14 */	(*this)[1]*(*this)[7]*(*this)[10] + (*this)[2]*(*this)[5]*(*this)[11] + (*this)[3]*(*this)[6]*(*this)[9] - (*this)[1]*(*this)[6]*(*this)[11] - (*this)[2]*(*this)[7]*(*this)[9] - (*this)[3]*(*this)[5]*(*this)[10],
				/* 21 */	(*this)[4]*(*this)[11]*(*this)[14] + (*this)[6]*(*this)[8]*(*this)[15] + (*this)[7]*(*this)[10]*(*this)[12] - (*this)[4]*(*this)[10]*(*this)[15] - (*this)[6]*(*this)[11]*(*this)[12] - (*this)[7]*(*this)[8]*(*this)[14],
				/* 22 */	(*this)[0]*(*this)[10]*(*this)[15] + (*this)[2]*(*this)[11]*(*this)[12] + (*this)[3]*(*this)[8]*(*this)[14] - (*this)[0]*(*this)[11]*(*this)[14] - (*this)[2]*(*this)[8]*(*this)[15] - (*this)[3]*(*this)[10]*(*this)[12],
				/* 23 */	(*this)[0]*(*this)[7]*(*this)[14] + (*this)[2]*(*this)[4]*(*this)[15] + (*this)[3]*(*this)[6]*(*this)[12] - (*this)[0]*(*this)[6]*(*this)[15] - (*this)[2]*(*this)[7]*(*this)[12] - (*this)[3]*(*this)[4]*(*this)[14],
				/* 24 */	(*this)[0]*(*this)[6]*(*this)[11] + (*this)[2]*(*this)[7]*(*this)[8] + (*this)[3]*(*this)[4]*(*this)[10] - (*this)[0]*(*this)[7]*(*this)[10] - (*this)[2]*(*this)[4]*(*this)[11] - (*this)[3]*(*this)[6]*(*this)[8],
				/* 31 */	(*this)[4]*(*this)[9]*(*this)[15] + (*this)[5]*(*this)[11]*(*this)[12] + (*this)[7]*(*this)[8]*(*this)[13] - (*this)[4]*(*this)[11]*(*this)[13] - (*this)[5]*(*this)[8]*(*this)[15] - (*this)[7]*(*this)[9]*(*this)[12],
				/* 32 */	(*this)[0]*(*this)[11]*(*this)[13] + (*this)[1]*(*this)[8]*(*this)[15] + (*this)[3]*(*this)[9]*(*this)[12] - (*this)[0]*(*this)[9]*(*this)[15] - (*this)[1]*(*this)[11]*(*this)[12] - (*this)[3]*(*this)[8]*(*this)[13],
				/* 33 */	(*this)[0]*(*this)[5]*(*this)[15] + (*this)[1]*(*this)[7]*(*this)[12] + (*this)[3]*(*this)[4]*(*this)[13] - (*this)[0]*(*this)[7]*(*this)[13] - (*this)[1]*(*this)[4]*(*this)[15] - (*this)[3]*(*this)[5]*(*this)[12],
				/* 34 */	(*this)[0]*(*this)[7]*(*this)[9] + (*this)[1]*(*this)[4]*(*this)[11] + (*this)[3]*(*this)[5]*(*this)[8] - (*this)[0]*(*this)[5]*(*this)[11] - (*this)[1]*(*this)[7]*(*this)[8] - (*this)[3]*(*this)[4]*(*this)[9],
				/* 41 */	(*this)[4]*(*this)[10]*(*this)[13] + (*this)[5]*(*this)[8]*(*this)[14] + (*this)[6]*(*this)[9]*(*this)[12] - (*this)[4]*(*this)[9]*(*this)[14] - (*this)[5]*(*this)[10]*(*this)[12] - (*this)[6]*(*this)[8]*(*this)[13],
				/* 42 */	(*this)[0]*(*this)[9]*(*this)[14] + (*this)[1]*(*this)[10]*(*this)[12] + (*this)[2]*(*this)[8]*(*this)[13] - (*this)[0]*(*this)[10]*(*this)[13] - (*this)[1]*(*this)[8]*(*this)[14] - (*this)[2]*(*this)[9]*(*this)[12],
				/* 43 */	(*this)[0]*(*this)[6]*(*this)[13] + (*this)[1]*(*this)[4]*(*this)[14] + (*this)[2]*(*this)[5]*(*this)[12] - (*this)[0]*(*this)[5]*(*this)[14] - (*this)[1]*(*this)[6]*(*this)[12] - (*this)[2]*(*this)[4]*(*this)[13],
				/* 44 */	(*this)[0]*(*this)[5]*(*this)[10] + (*this)[1]*(*this)[6]*(*this)[8] + (*this)[2]*(*this)[4]*(*this)[9] - (*this)[0]*(*this)[6]*(*this)[9] - (*this)[1]*(*this)[4]*(*this)[10] - (*this)[2]*(*this)[5]*(*this)[8]
				};
				const T delta = (*this)[0] * inv_matrix[0] +
					(*this)[1] * inv_matrix[4] +
					(*this)[2] * inv_matrix[8] +
					(*this)[3] * inv_matrix[12];
				if(delta == 0)
					return false;
				std::transform(inv_matrix.cbegin(), inv_matrix.cend(), (*this).begin(), std::bind(std::multiplies<T>(), 1 / delta, std::placeholders::_1));
				return true;
			}
			// Binary methods
			Matrix4x4 operator*(const Matrix4x4& other) const noexcept{
				return {
					(*this)[0] * other[0] + (*this)[1] * other[4] + (*this)[2] * other[8] + (*this)[3] * other[12],
					(*this)[0] * other[1] + (*this)[1] * other[5] + (*this)[2] * other[9] + (*this)[3] * other[13],
					(*this)[0] * other[2] + (*this)[1] * other[6] + (*this)[2] * other[10] + (*this)[3] * other[14],
					(*this)[0] * other[3] + (*this)[1] * other[7] + (*this)[2] * other[11] + (*this)[3] * other[15],
					(*this)[4] * other[0] + (*this)[5] * other[4] + (*this)[6] * other[8] + (*this)[7] * other[12],
					(*this)[4] * other[1] + (*this)[5] * other[5] + (*this)[6] * other[9] + (*this)[7] * other[13],
					(*this)[4] * other[2] + (*this)[5] * other[6] + (*this)[6] * other[10] + (*this)[7] * other[14],
					(*this)[4] * other[3] + (*this)[5] * other[7] + (*this)[6] * other[11] + (*this)[7] * other[15],
					(*this)[8] * other[0] + (*this)[9] * other[4] + (*this)[10] * other[8] + (*this)[11] * other[12],
					(*this)[8] * other[1] + (*this)[9] * other[5] + (*this)[10] * other[9] + (*this)[11] * other[13],
					(*this)[8] * other[2] + (*this)[9] * other[6] + (*this)[10] * other[10] + (*this)[11] * other[14],
					(*this)[8] * other[3] + (*this)[9] * other[7] + (*this)[10] * other[11] + (*this)[11] * other[15],
					(*this)[12] * other[0] + (*this)[13] * other[4] + (*this)[14] * other[8] + (*this)[15] * other[12],
					(*this)[12] * other[1] + (*this)[13] * other[5] + (*this)[14] * other[9] + (*this)[15] * other[13],
					(*this)[12] * other[2] + (*this)[13] * other[6] + (*this)[14] * other[10] + (*this)[15] * other[14],
					(*this)[12] * other[3] + (*this)[13] * other[7] + (*this)[14] * other[11] + (*this)[15] * other[15]
				};
			}
			enum class Order{APPEND, PREPEND};
			Matrix4x4& multiply(const Matrix4x4& other, const Order order = Order::PREPEND) noexcept{
				return *this = (order == Order::PREPEND ? *this * other : other * *this);
			}
			Matrix4x4& translate(const T x, const T y, const T z, const Order order = Order::PREPEND) noexcept{
				return this->multiply({
						1, 0, 0, x,
						0, 1, 0, y,
						0, 0, 1, z,
						0, 0, 0, 1
					}, order);
			}
			Matrix4x4& scale(const T x, const T y, const T z, const Order order = Order::PREPEND) noexcept{
				return this->multiply({
						x, 0, 0, 0,
						0, y, 0, 0,
						0, 0, z, 0,
						0, 0, 0, 1
					}, order);
			}
			enum class Axis{X, Y, Z};
			Matrix4x4& rotate(const T angle, const Axis axis, const Order order = Order::PREPEND) noexcept{
				const auto sin = std::sin(angle),
					cos = std::cos(angle);
				switch(axis){
					case Axis::X:
						return this->multiply({
								1, 0, 0, 0,
								0, cos, -sin, 0,
								0, sin, cos, 0,
								0, 0, 0, 1
							}, order);
					case Axis::Y:
						return this->multiply({
								cos, 0, sin, 0,
								0, 1, 0, 0,
								-sin, 0, cos, 0,
								0, 0, 0, 1
							}, order);
					case Axis::Z:
						return this->multiply({
								cos, -sin, 0, 0,
								sin, cos, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1
							}, order);
					default: return *this;
				}
			}
			// Transformations
			std::array<T,4> transform(const T* vec) const noexcept{
				return {
					(*this)[0] * vec[0] + (*this)[1] * vec[1] + (*this)[2] * vec[2] + (*this)[3] * vec[3],
					(*this)[4] * vec[0] + (*this)[5] * vec[1] + (*this)[6] * vec[2] + (*this)[7] * vec[3],
					(*this)[8] * vec[0] + (*this)[9] * vec[1] + (*this)[10] * vec[2] + (*this)[11] * vec[3],
					(*this)[12] * vec[0] + (*this)[13] * vec[1] + (*this)[14] * vec[2] + (*this)[15] * vec[3]
				};
			}
			std::array<T,4> transform(const std::array<T,4>& vec) const noexcept{
				return this->transform(vec.cbegin());
			}
			std::array<T,4> transform(const T vec_x, const T vec_y, const T vec_z, const T vec_w) const noexcept{
				return this->transform({vec_x, vec_y, vec_z, vec_w});
			}
	};
	using Matrix4x4f = Matrix4x4<float>;
	using Matrix4x4d = Matrix4x4<double>;
}
