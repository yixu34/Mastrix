#include "mastrix.hpp"

ServerConsoleVar<float> mineTriggerRadius("mine_dist", 180.0);
ServerConsoleVar<float> mineDamage("mine_damage", 6);
ServerConsoleVar<float> mineDelay("mine_time", 0.5);

Mine::Mine(const Position &pos)
{
	entType = ent_projectile;
	coreRadius = 12;
	isMagnetic = false;
	isArmed = false;
	position = pos;
	sprite = "images/mine1.png";
	radius = mineTriggerRadius;
	drawScale = 0.25;
	
	sendAdd();
}

void Mine::timepass() {

	if (isArmed) {
		detonateTime -= getDt();
		if (detonateTime < 0) detonate();
	}
	Entity::timepass();
}

void Mine::detonate()
{
	radius = mineTriggerRadius;
	SendMessage explodeMsg(message_explode);
		explodeMsg.putFloat(position.getX());
		explodeMsg.putFloat(position.getY());
		explodeMsg.putFloat(radius+20);
		explodeMsg.putInt(explode_mine);
	explodeMsg.sendToEveryone();

    server->playSoundAt("sounds/explosion2.wav", position);

	for(Server::Entpool::iterator ii = server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		if(ii->second->getCollisionEffect() & collide_skip)
			continue;

		if(server->testSphereCollision(this, ii->second)
		 && !ii->second->shouldDelete()
		 && ii->second != this)
		{
			ii->second->takeDamage(mineDamage, getDamageSource());
		}
	}
	
	// Get rid of this ent
	deleteMe();
}

bool Mine::canDetonate(Entity *ent)
{
	if (ent->isPlayer()) return true;
	else return false;
}

bool Mine::canDestroy(Entity *ent)
{
	if (ent->isPlayer()) return true;
	else if (ent->isExplosive()) return true;
	else return false;
}

void Mine::collideWith(Entity *ent)
{
	if(((ent->getPosition().positionVector() - position.positionVector()).getMagnitude() <= coreRadius+ent->getRadius())
		 && canDestroy(ent)) 
	{
		detonate();
	}
	else if (canDetonate(ent)) {
		isArmed = true;
		detonateTime = mineDelay;
		radius = 1;
	}
}

std::string Mine::creationString()
{
	return retprintf("mine %f %f\n", position.getX(), position.getY());
}

SERVER_CONSOLE_COMMAND(mine)
{
	if(argc < 2)
	{
		printfToClient(who, "Usage: mine x y");
		return;
	}
	new Mine(Position(atof(argv[0]), atof(argv[1]))) ;
}