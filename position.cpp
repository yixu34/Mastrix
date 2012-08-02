#include "mastrix.hpp"

Position::Position()
{
	x=0;
	y=0;
	r=0;
	x_vel=0;
	y_vel=0;
	r_vel=0;
	changed = false;
	timeofupdate = getTime();
	relativeTo = 0;
}
Position::Position(float x, float y, float r) 
{
	this->x = x; 
	this->y = y; 
	this->r = r;
	x_vel = y_vel = r_vel = 0;
	changed		 = false;
	timeofupdate = getTime();
	relativeTo = 0;
}
Position::Position(const Vector2D &v)
{
	this->x = v.x; 
	this->y = v.y; 
	this->r = 0;
	x_vel = y_vel = r_vel = 0;
	changed		 = false;
	timeofupdate = getTime();
	relativeTo = 0;
}
void Position::makeRelative(int parent)
{
	Position parentPos = *currentNode->getEntityPosition(parent);
	float dx = getX() - parentPos.getX(),
	      dy = getY() - parentPos.getY();
	x = dx*cos(parentPos.getR()) + dy*sin(parentPos.getR());
	y = dx*sin(parentPos.getR()) + dy*cos(parentPos.getR());
	x_vel = y_vel = 0;
	relativeTo = parent;
	changed = true;
}
void Position::impulse (float dx_vel, float dy_vel)
{
	if(dx_vel == 0 && dy_vel == 0)
		return;
	
	setX_vel(getX_vel()+dx_vel);
	setY_vel(getY_vel()+dy_vel);
}
float Position::getX(void) const
{
	if(relativeTo) {
		Position parent = *currentNode->getEntityPosition(relativeTo);
		float px = parent.getX(),
		      pr = parent.getR();
		return px + x*cos(pr) + y*sin(pr);
	}
	else
		return x + (x_vel*(getTime()-timeofupdate));
}

float Position::getY(void) const
{
	if(relativeTo) {
		Position parent = *currentNode->getEntityPosition(relativeTo);
		float py = parent.getY(),
		      pr = parent.getR();
		return py + x*-sin(pr) + y*cos(pr);
	}
	else
		return y + (y_vel*(getTime()-timeofupdate));
}

float Position::getR(void) const
{
	return r + (r_vel*(getTime()-timeofupdate));
}

enum {
	nonzeroVelocity = 0x01,
	nonzeroRotVel   = 0x02,
	nonzeroRotation = 0x04,
	nonzeroParent   = 0x08,
};

// Rather than just sending (x, y, r, xvel, yvel, rvel) send a bitmask first
// indicating which fields are non-zero, and only send those. This saves
// bandwidth with objects that aren't moving, spinning, etc.
void Position::writeToMessage(SendMessage &msg)
{
	unsigned char nonzeroFields = 0;
	if(x_vel != 0 || y_vel != 0)
		nonzeroFields |= nonzeroVelocity;
	if(r_vel != 0)
		nonzeroFields |= nonzeroRotVel;
	if(r != 0)
		nonzeroFields |= nonzeroRotation;
	if(relativeTo != 0)
		nonzeroFields |= nonzeroParent;
	
	msg.putChar(nonzeroFields);
	msg << x << y;
	msg.putFloat(timeofupdate);
	if(nonzeroFields & nonzeroRotation)
		msg << r;
	if(nonzeroFields & nonzeroVelocity)
		msg << x_vel << y_vel;
	if(nonzeroFields & nonzeroRotVel)
		msg << r_vel;
	if(nonzeroFields & nonzeroParent)
		msg.putInt(relativeTo);
}

void Position::readFromMessage(RecvMessage &msg)
{
	unsigned char nonzeroFields;
	
	nonzeroFields = msg.getChar();
	msg >> x >> y;
	timeofupdate = msg.getFloat();
	if(nonzeroFields & nonzeroRotation)
	     msg >> r;
	else r = 0;
	if(nonzeroFields & nonzeroVelocity)
	     msg >> x_vel >> y_vel;
	else x_vel = y_vel = 0;
	if(nonzeroFields & nonzeroRotVel)
	     msg >> r_vel;
	else r_vel = 0;
	if(nonzeroFields & nonzeroParent)
		relativeTo = msg.getInt();
	else relativeTo = 0;
}
void Position::setX(float v)
{
	if(relativeTo) return;
	update_time();
	x=v;
	changed = true;
}
void Position::setY(float v)
{
	if(relativeTo) return;
	update_time();
	y=v;
	changed = true;
}
void Position::setR(float v)
{
	update_time();
	r=v;
	changed = true;
}
void Position::setR_vel(float v)
{
	setR(getR());
	r_vel=v;
	changed=true;
}
void Position::setX_vel(float v)
{
	setX(getX());
	x_vel=v;
	changed=true;
}
void Position::setY_vel(float v)
{
	setY(getY());
	y_vel=v;
	changed=true;
}

bool Position::getChanged(void)
{
	return changed;
}
void Position::setChanged(bool flag)
{
	changed = flag;
}
void Position::update_time(void)
{
	if(timeofupdate == getTime()) return;
	if(!relativeTo) {
		x = getX();
		y = getY();
		r = getR();
	}
	timeofupdate = getTime();
}
