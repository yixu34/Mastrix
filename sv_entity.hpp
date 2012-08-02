#ifndef SVENTITY_HPP
#define SVENTITY_HPP

#include "position.hpp"
#include "message.hpp"
#include "particlepool.hpp"
#include "protocol.hpp"
#include <string>

enum {
	collide_skip          = 0x0001, // Perform no collision detection on this object
	collide_bounce        = 0x0002, // Bounces off objects, and objects bounce off it
	collide_damage        = 0x0004, // Deals getCollisionDamage() to anything it collides with
	collide_no_gravity    = 0x0008, // Exempt from gravity
	collide_destroy       = 0x0010, // Disappears on collision
	collide_immobile      = 0x0020, // Can never be bounced
	collide_custom        = 0x0040, // Call function instead of normal effect
};

enum {
	env_no_mass       = 0x0001,
	env_no_collide    = 0x0002,
	env_no_boundaries = 0x0004,
	env_rectangular   = 0x0008,
	env_wraps         = 0x0010,
	env_hidden        = 0x0020,
};

enum explosion_type;

class Entity
{
public:
	Entity();
	virtual ~Entity();
	
	void sendAdd(void);
	void sendAdd(int destination);
	void sendUpdate(void);
	void sendUpdate(int destination);
	void sendDrop(void);
	
	bool shouldDelete(void) { return isDead; }
	int getEntId(void) { return entID; }
	float getSpeedSquared() const;
	virtual int getCollisionEffect(void) { return collide_skip; }
	virtual float getMaxSpeed() const;
	virtual float getMass(void) { return 1.0; }
	void gravity_affect(Entity *object);
	virtual float getCollisionDamage(void) { return 1.0; }
	virtual int getDamageSource(void) { return -1; }
	virtual void takeDamage(float amt, int source) { }
	virtual bool hasGravity( void ) { return false; }
	virtual bool staysInBounds(void) { return true; }
	virtual std::string creationString(void) { return ""; }
	virtual int getDrawEffect(void) { return effect_none; }
	virtual void collideWith(Entity *ent) {}
	virtual bool canCollideWith(Entity *ent) { return true; }
	
	virtual bool isPlayer(void) { return false; }
	virtual bool isHuman(void) { return false; }
	virtual bool isWaypointMarker()	{ return false;}
	virtual bool isShot() { return false; }
	virtual bool isPeaShot() {return false;}
	virtual bool isExplosive(){ return false; }
	virtual bool isAsteroid() { return false; }
	virtual bool isMine() {return false; }
	
	virtual void timepass(); 

	void hide();
	void show();
	
	virtual explosion_type getExplodeType() const
		{ return explodeType; }
	
	virtual void die(int killer);
	virtual void deleteMe(void);

	inline  Position &getPosition()
		{ return position; }
	inline void setPosition(const Position &pos) {
		position = pos;
		position.setChanged(true);
	}
	void rescale(float scale) {
		sendDrop();
		radius *= (scale/drawScale);
		drawScale = scale;
		sendAdd();
	}

	inline float getRadius() const
		{ return radius; }
	inline std::string getLabel() const
		{ return label; }
	virtual bool isRectangular(void) const { return false; }
	virtual void getDimensions(float *w, float *h) const { }

//	inline bool isOwnedBy(Entity *ent) const
//		{ return (owner == ent); }

	inline bool isVisible()
		{ return visible; }
	
protected:	
	void msgAddBody(SendMessage &add);
	void msgUpdateBody(SendMessage &update);
	void wrap(void);
	
	std::string	   sprite;
	Position	   position;
	float		   drawScale;	
	float		   radius;
	int			   entID;
	bool		   isDead;
//	Entity        *owner;
	explosion_type explodeType;
	float		   mass;
	bool		   visible;
	int			   entType;
	std::string    label;

	team_type team;
};

class EnvironmentEntity :public Entity
{
public:
	EnvironmentEntity(const char *imageName, float x, float y, float scale, float rotation, const char *label, int flags);
	int getCollisionEffect(void) {
		return ((flags&env_no_collide)?collide_skip:0)
		     | collide_damage | collide_bounce | collide_no_gravity | collide_immobile;
	}
	bool staysInBounds(void) { return !(flags & (env_no_boundaries|env_wraps)); }
	float getMass(void) { return (flags&env_no_mass)?0:1000; }
	bool hasGravity(void) { return !(flags&env_no_mass);}
	std::string creationString(void);
	void addFlag(int flag) { flags |= flag; }
	static int flagFromString(const char *str);
	
	void timepass(void);
	
	bool isRectangular(void) const { if(flags&env_rectangular) return true; else return false; }
	void getDimensions(float *w, float *h) const { *w=width; *h=height; }
	
protected:
	int flags;
	float width, height;
};

class DummyEntity :public Entity
{
public:
	DummyEntity(float x, float y);
	bool hasGravity(void) { return false; }
	int getCollisionEffect(void) {
		return collide_skip | collide_no_gravity | collide_immobile;
	}
};


extern ServerConsoleVar<float> gravConst;

#endif	//SVENTITY_HPP
