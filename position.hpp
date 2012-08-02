#ifndef POSITION_HPP
#define POSITION_HPP

#include <cmath>
#include "vector.hpp"

class Position
{
public:
	Position();
	Position(float x, float y, float r = 0.0f);
	Position(const Vector2D &v);
	
	float getX() const;
	float getY() const;
	float getR() const;
	void setX(float v);
	void setY(float v);
	void setR(float v);
	void setX_vel(float v);
	void setY_vel(float v);
	void setR_vel(float v);
	void impulse (float dx_vel, float dy_vel);
	void makeRelative(int parent);
	bool getChanged(void);
	void setChanged(bool flag);
	inline float getX_vel() const {return x_vel;}
	inline float getY_vel() const {return y_vel;}
	inline float getR_vel() const {return r_vel;}
	
	inline float getMagnitude() const {return sqrtf(getX()*getX() + getY()*getY());}
	inline float getVelocityMagnitude() const {return sqrtf(getX_vel()*getX_vel() + getY_vel()*getY_vel());}
	
	Vector2D positionVector(void) const { return Vector2D(getX(), getY()); }
	Vector2D velocityVector(void) const { return Vector2D(getX_vel(), getY_vel()); }

	inline const bool operator==(const Position &right) const
		{ return (getX() == right.getX() && getY() == right.getY()); }

	void writeToMessage(SendMessage &msg);
	void readFromMessage(RecvMessage &msg);

private:
	void update_time(void);
	int relativeTo;
	
	float x, y, r;
	float x_vel, y_vel, r_vel;
	bool changed;
	double timeofupdate;
};

#endif	//POSITION_HPP
