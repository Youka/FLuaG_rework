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
#include <algorithm>
#include <functional>

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

	inline std::vector<std::array<Point2d,3>> ear_clipping(std::vector<Point2d> points){
		// Output buffer
		std::vector<std::array<Point2d,3>> triangles;
		// Anything to do?
		if(points.size() > 2){
			// Buffers for calculations
			triangles.reserve(points.size()-2);
			std::vector<char> directions; directions.reserve(points.size()-2);
			std::vector<Point2d> new_points; new_points.reserve(points.size()-1);
			// Evaluate triangles from points
			while(points.size() > 2){
				// Collect angles of point-to-neighbours vectors (exclude first & last point)
				directions.clear();
				double sum_directions = 0;
				for(size_t i = 2; i < points.size(); ++i){
					const double z = normal_z(points[i-2], points[i-1], points[i]);
					directions.push_back(Math::sign(z));
					sum_directions += z;
				}
				// Polygon is convex?
				const char all_direction = Math::sign(sum_directions);
				if(std::all_of(directions.cbegin(), directions.cend(), std::bind(std::equal_to<const char>(), all_direction, std::placeholders::_1))){
					// Just split points into triangles
					for(size_t i = 2; i < points.size(); ++i)
						triangles.push_back({points.front(), points[i-1], points[i]});
					break;	// Finish / just 2 points left
				}else{
					// Pick ears/edge triangles from points
					new_points = {points.front()};
					for(size_t first = 0, next = 1, next_end = points.size()-1; next != next_end; ++next){
						const Point2d& t1 = points[first], t2 = points[next], t3 = points[next+1];
						const char& direction = directions[next-1];	// Remember: directions don't include the first point!!!
						// Point is ear without intersection
						if(direction == all_direction && std::none_of(points.cbegin(), points.cend(), std::bind(in_triangle, std::placeholders::_1, t1, t2, t3)))
							triangles.push_back({t1, t2, t3});
						// Point is valley or ear with intersection
						else if(direction != 0){
							new_points.push_back(t2);
							first = next;
						}
						// Point is on vector...
					}
					new_points.push_back(points.back());
					points.swap(new_points);
				}
			}
		}
		// Return collected triangles
		return triangles;
	}

	inline std::vector<Point2d> curve_flatten(const std::array<Point2d,4> points, const double tolerance){
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
}
