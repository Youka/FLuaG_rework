/*
Project: FLuaG
File: math.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

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

namespace Math{
	template<typename T>
	inline char sign(const T x){
		return x < 0 ? -1 : (x > 0 ? 1 : 0);
	}

	inline int fac(int n){
		int k = 1;
		while(n > 1)
			k *= n--;
		return k;
	}

	inline double bin_coeff(const int i, const int n){
		return fac(n) / (fac(i) * fac(n-i));
	}

	inline double bernstein(const int i, const int n, const double t){
		return bin_coeff(i, n) * std::pow(t, i) * std::pow(1-t, n-i);
	}
}

namespace Geometry{
	struct Point2d{
		double x, y;
		Point2d operator+(const Point2d& other) const{return {this->x + other.x, this->y + other.y};};
		Point2d operator-(const Point2d& other) const{return {this->x - other.x, this->y - other.y};};
		Point2d operator*(const Point2d& other) const{return {this->x * other.x, this->y * other.y};};
		Point2d operator/(const Point2d& other) const{return {this->x / other.x, this->y / other.y};};
		Point2d operator-() const{return {-this->x, -this->y};};
		bool operator==(const Point2d& other) const{return this->x == other.x && this->y == other.y;};
		bool operator!=(const Point2d& other) const{return !(*this == other);};
	};

	inline double normal_z(const Point2d& v1, const Point2d& v2){
		return v1.x * v2.y - v1.y * v2.x;
	}
	inline double normal_z(const Point2d& p0, const Point2d& p1, const Point2d& p2){
		return normal_z(p1 - p0, p2 - p1);
	}

	inline bool on_line(const Point2d& p, const Point2d& l0, const Point2d& l1){
		static const auto length_sqr = [](const Point2d& p){return p.x * p.x + p.y * p.y;};
		return length_sqr(l1 - l0) == length_sqr(p - l0) + length_sqr(p - l1);
	}

	inline Point2d line_intersect(const Point2d& l1p1, const Point2d& l1p2, const Point2d& l2p1, const Point2d& l2p2){
		// Check direction
		const Point2d l1 = l1p2 - l1p1,
			l2 = l2p2 - l2p1;
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

	inline bool in_triangle(const Point2d& p, const Point2d& t1, const Point2d& t2, const Point2d& t3){
		const char t2t3p = Math::sign(normal_z(t2, t3, p));
		return Math::sign(normal_z(t1, t2, p)) == t2t3p && t2t3p == Math::sign(normal_z(t3, t1, p));
	}

	inline Point2d stretch(const Point2d& v, const double new_length){
		double factor;
		return v.x == 0 && v.y == 0 ? v : (factor = new_length / std::hypot(v.x, v.y), Point2d{factor,factor} * v);
	}

	inline Point2d rotate(const Point2d& v, const double angle){
		const double sin_angle = std::sin(angle),
			cos_angle = std::cos(angle);
		return {cos_angle * v.x - sin_angle * v.y, sin_angle * v.x + cos_angle * v.y};
	}

	inline std::vector<std::array<Point2d,4>> arc_to_curves(const Point2d& start, const Point2d& center, const double angle){
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
}
