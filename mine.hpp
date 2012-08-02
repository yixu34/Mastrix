#ifndef MINE_HPP
#define MINE_HPP

#include "server.hpp"

class Mine : public Entity
{

public:
	Mine(const Position &pos);
	void detonate();
	bool isMine() {return true;}
	bool canDetonate(Entity *ent);
	bool canDestroy(Entity *ent);
	int getCollisionEffect() {return collide_custom | collide_no_gravity;}
	void collideWith(Entity *ent);
	void timepass();
	std::string creationString();

protected:
	float coreRadius;
	bool isMagnetic;
	bool isArmed;
	float detonateTime;

};

#endif