#ifndef VECTOR_HPP
#define VECTOR_HPP

struct Vector2D
{
public:
	inline Vector2D() : x(0), y(0) {}
	inline Vector2D(float _x, float _y) : x(_x), y(_y) {}
	
	inline Vector2D operator+(const Vector2D &right) const { return Vector2D(x+right.x, y+right.y); }
	inline Vector2D operator-(const Vector2D &right) const { return Vector2D(x-right.x, y-right.y); }
	inline Vector2D operator-()                      const { return Vector2D(-x, -y);               }
	inline Vector2D operator*(float scalar)          const { return Vector2D(x*scalar, y*scalar);   }
	inline Vector2D operator/(float scalar)          const { return Vector2D(x/scalar, y/scalar);   }
	inline Vector2D &operator*=(float scalar)           { x *= scalar; y *= scalar;   return *this; }
	inline Vector2D &operator/=(float scalar)           { x /= scalar; y /= scalar;   return *this; }
	inline Vector2D &operator+=(const Vector2D &right)  { x += right.x; y += right.y; return *this; }
	
	inline float dotProduct(const Vector2D &right)   const { return x*right.x + y*right.y;          }
	inline float getMagnitude(void) const
		{ return sqrt(x*x+y*y); }
	inline float getMagnitudeSquared(void) const
		{ return x*x+y*y; }
	inline void normalize(void)
		{ operator/=(getMagnitude()); }
	inline void rotate(float theta) {
		float newX = x*cos(theta) + y*sin(theta),
		      newY = -x*sin(theta) + y*cos(theta);
		x = newX;
		y = newY;
	}
	inline bool isOnRight(const Vector2D &vec) {
		return (vec.x * y) < (vec.y * x);
	}
	
	float x, y;
};

#endif
