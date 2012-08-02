#ifndef SHOT_HPP
#define SHOT_HPP

#include "sv_entity.hpp"

class Position;
class Player;

class Shot :public Entity
{
public:
	Shot(const Position &pos, Player *source);
	
	int getCollisionEffect(void);
	int getDamageSource(void) { return shooter; }
	virtual void timepass(void);
	bool staysInBounds(void) { return false; }
	float getMaxSpeed(void) const  { return 10000; }
	bool canCollideWith(Entity *ent);
	bool isShot() { return true; }
	
protected:
	Entity *owner;
	float age;
	int   shooter;
	float armDelay;
	int ownerTeam;
};

class Fireball :public Shot
{
public:
	Fireball(const Position &pos, Player *source);
	int getDrawEffect(void) { return effect_trail; }
	void takeDamage(float amt, int source) { detonate(); }
	void timepass(void);
	bool isExplosive() {return true;}
	
protected:
	float travelDistance;
	void detonate(void);
	bool exploding;
};

class RedPea :public Shot
{
public:
	RedPea(const Position &pos, Player *source, int level);
	int getDrawEffect(void);
	float getCollisionDamage();
	bool isPeaShot() {return true;}
	
protected:
	int level;
};

class BaseMissile :public Shot
{
public:
	BaseMissile(const Position &pos, Player *source) :Shot(pos,source) {}
	int getDrawEffect(void) { return effect_missile_trail; }
	void takeDamage(float amt, int source) { detonate(); }
	bool isExplosive() {return true;}
	
protected:
	void detonate(void);
	bool exploding;
};

class Missile :public BaseMissile
{
public:
	Missile(const Position &pos, Player *source);
};

class HomingMissile :public BaseMissile
{
public:
	HomingMissile(const Position &pos, Player *source);
	void timepass(void);
	
protected:
	void acquireTarget(void);
	Vector2D targetPos;
	int target;
};

//class ClusterBomb :public Shot
//{
//public:
//	ClusterBomb(
//		const Position &pos, 
//		int numExplosions, 
//		Player *source);
//
//    ClusterBomb(
//        const Position &pos, 
//		int numExplosions, 
//        int clientID, 
//        int shooterEntID);
//
//    void initialize(
//		const Position &pos, 
//		int numExplosions);
//
//	int getDrawEffect(void) { return effect_missile_trail; }
//	void takeDamage(float amt, int source) { detonate(); }
//    Player *getNearestTargetFrom(Entity *excludedEnt);
//    bool isEntityCloseEnough(Entity *target);
//
//private:
//	int  numExplosionsLeft;
//    int  shooterEntID;
//	void detonate(void);
//	bool exploding;
//    int  initialX;
//    int  initialY;
//};

#endif	//SHOT_HPP
