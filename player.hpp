#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "sv_entity.hpp"
#include <string>
#include "weapons.hpp"

class ShipType;

//abstract base class for human players and bots
class Player :public Entity
{
public:
	Player(ShipType *type);
	virtual ~Player() = 0;
	
	void turnLeft(bool active);
	void turnRight(bool active);
	void strafeLeft(bool active);
	void strafeRight(bool active);
	void thrust(bool active);
	void brake(bool active);
	void fire1(bool active);
	void fire2(bool active);
	virtual void takeDamage(float amt, int source);
	virtual void heal(float amt);
	void trackMouse(float x, float y);
	virtual int getCollisionEffect(void);
	bool isPlayer(void) { return true; }
	void setName(std::string name);
	inline std::string getName(void) { return name; }
	bool canCollideWith(Entity *ent);
	
	virtual bool isHuman(void) { return false; }
	virtual int getClientId(void) { return -1; }
	
	virtual float getMaxSpeed(void) const { return type->getMaxSpeed(); }
	
	virtual void die(int killer);
	virtual void deleteMe(void);
	
	virtual void timepass(void);
	
	inline team_type getTeam() const       { return team; }
	inline void setTeam(team_type newTeam) { team = newTeam; }
	
	ShipType *type;
	WeaponPool weapons;
	float mouseX, mouseY;
	
protected:
	void processInput();
	void activateShield(float time, int type);

	bool turningLeft;
	bool turningRight;
	bool thrusting;
	bool braking;
	bool shooting;
	bool strafingLeft;
	bool strafingRight;
	bool shielding;
	
	float afterburn_global, afterburn_left, afterburn_right, afterburn_thrust, afterburn_brake;
	float warmup;
	
	float health, maxHealth;
	float shieldTime;
	short engines_set;

	std::string name;
};


extern ServerConsoleVar<float> sharedABMax;
extern ServerConsoleVar<float> strafeABMax;
extern ServerConsoleVar<float> thrustABMax;
extern ServerConsoleVar<float> brakeABMax;
extern ServerConsoleVar<float> warmupFactor;
extern ServerConsoleVar<float> maxWarmup;

#endif	//PLAYER_HPP
