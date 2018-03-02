// (C) 2018 University of Bristol. See License.txt

/*
 * operations.h
 *
 */

#ifndef MATH_OPERATORS_H_
#define MATH_OPERATORS_H_

template <class T>
T operator*(const bool& x, const T& y) { return x ? y : T(); }
template <class T>
T operator*(const T& y, const bool& x) { return x ? y : T(); }
template <class T>
T& operator*=(const T& y, const bool& x) { y = x ? y : T(); return y; }

template <class T, class U>
T operator+(const T& x, const U& y) { T res; res.add(x, y); return res; }
template <class T>
T operator*(const T& x, const T& y) { T res; res.mul(x, y); return res; }
template <class T, class U>
T operator-(const T& x, const U& y) { T res; res.sub(x, y); return res; }

template <class T, class U>
T& operator+=(T& x, const U& y) { x.add(y); return x; }
template <class T>
T& operator*=(T& x, const T& y) { x.mul(y); return x; }
template <class T, class U>
T& operator-=(T& x, const U& y) { x.sub(y); return x; }

template <class T, class U>
T operator/(const T& x, const U& y) { U inv = y; inv.invert(); return x * inv; }
template <class T, class U>
T& operator/=(const T& x, const U& y) { U inv = y; inv.invert(); return x *= inv; }

#endif /* MATH_OPERATORS_H_ */
