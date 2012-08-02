#include "mastrix.hpp"

ServerConsoleVar<float> fireballExplodeRadius("fireball_radius", 200);
ServerConsoleVar<float> fireballDamage("fireball_damage", 10);
ServerConsoleVar<float> missileExplodeRadius("missile_radius", 50);
ServerConsoleVar<float> missileDamage("missile_damage", 2);
ServerConsoleVar<float> clusterBombDamage("clusterbomb_damage", 3);
ServerConsoleVar<float> bounceRange("bounce_range", 500.0f);

Shot::Shot(const Position &pos, Player *owner)
{
	position = pos;
	age = 0;
	armDelay = 0;
	entType = ent_projectile;
	
	ownerTeam = owner->getTeam();
	this->owner = owner;
	
	if(owner != 0 && owner->isHuman())
		shooter = owner->getClientId();
	else
		shooter = -1;
}

void Shot::timepass(void)
{
	Entity::timepass();
	age += getDt();
	
    //kill the shot before it gets to the edge
	const int borderPadding = 10;	

	if(position.getX()>rightBorder  - borderPadding ||
	   position.getX()<leftBorder   + borderPadding || 
	   position.getY()>bottomBorder - borderPadding ||
	   position.getY()<topBorder	+ borderPadding)
	{
		deleteMe();
	}
}

bool Shot::canCollideWith(Entity *ent)
{
	if(ent->isPlayer() && ((Player*)ent)->getTeam() == ownerTeam && ownerTeam != team_free)
		return false;
	else if(ent == owner)
		return false;
	else if(ent->isShot())
		return false;
	else
		return true;
}
int Shot::getCollisionEffect(void)
{
	if(age > armDelay)
		return collide_destroy | collide_damage;
	else
		return collide_skip;
}

RedPea::RedPea(const Position &pos, Player *owner, int level)
	: Shot(pos, owner)
{
//	sprite = "images/shot.png";
	sprite = "";
	radius = (images.getImage(sprite.c_str())->getWidth() / 2) * drawScale * 0.5;
	this->level = level;
	sendAdd();
}

int RedPea::getDrawEffect(void)
{
	switch(level)
	{
		default: return effect_shot;
		case 2: return effect_shot2;
		case 3: return effect_shot3;
	}
}

float RedPea::getCollisionDamage() {
	switch(level) {
		case 1: return 1.0;		break;
		case 2: return 1.2;		break;
		case 3: return 1.5;		break;
		default: return 1.0;
	}
}

Fireball::Fireball(const Position &pos, Player *owner)
	: Shot(pos, owner)
{
	travelDistance = sqrt(owner->mouseX*owner->mouseX + owner->mouseY*owner->mouseY);
	if(travelDistance < fireballExplodeRadius + 100) travelDistance = fireballExplodeRadius + 100;
	
	sprite    = "images/star1.png";
	drawScale = 0.1;
	radius    = (images.getImage(sprite.c_str())->getWidth() / 2) * drawScale * 0.5;
	position.setR_vel(5);
	exploding = false;
	sendAdd();
}

void Fireball::timepass()
{
	Shot::timepass();
	travelDistance -= position.getVelocityMagnitude() * getDt();
	if(travelDistance < 0)
		detonate();
}


void Fireball::detonate(void)
{
	if(exploding) return;
	exploding = true;
	radius = fireballExplodeRadius;
	
	// Draw graphical effect for explosion
	SendMessage explodeMsg(message_explode);
		explodeMsg.putFloat(position.getX());
		explodeMsg.putFloat(position.getY());
		explodeMsg.putFloat(fireballExplodeRadius);
		explodeMsg.putInt(explode_fireball);
	explodeMsg.sendToEveryone();
	
	server->playSoundAt("sounds/Explosion6.wav", position);
	
	// Toast everything inside the blast radius
	for(Server::Entpool::iterator ii = server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		if(ii->second->getCollisionEffect() & collide_skip)
			continue;

		if(server->testSphereCollision(this, ii->second)
		 && !ii->second->shouldDelete()
		 && ii->second != this)
		{
			ii->second->takeDamage(fireballDamage, getDamageSource());
		}
	}
	
	// Get rid of this ent
	deleteMe();
}

void BaseMissile::detonate(void)
{
	if(exploding) return;
	exploding = true;
	radius = missileExplodeRadius;
	
	// Draw graphical effect for explosion
	SendMessage explodeMsg(message_explode);
		explodeMsg.putFloat(position.getX());
		explodeMsg.putFloat(position.getY());
		explodeMsg.putFloat(missileExplodeRadius);
		explodeMsg.putInt(explode_missile);
	explodeMsg.sendToEveryone();
	
	// Toast everything inside the blast radius
	for(Server::Entpool::iterator ii = server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		if(ii->second->getCollisionEffect() & collide_skip)
			continue;
		if(server->testSphereCollision(this, ii->second)
		 && !ii->second->shouldDelete()
		 && ii->second != this)
		{
			ii->second->takeDamage(missileDamage, getDamageSource());
		}
	}
	server->playSoundAt("sounds/Explosion3.wav", position);
	
	// Get rid of this ent
	deleteMe();
}

Missile::Missile(const Position &pos, Player *source)
	: BaseMissile(pos, source)
{
	sprite = "images/missile2.png";
	drawScale = 0.3;
	radius = (images.getImage(sprite.c_str())->getWidth() / 2) * drawScale * 0.5;
	exploding = false;
	sendAdd();
}

HomingMissile::HomingMissile(const Position &pos, Player *source)
	: BaseMissile(pos, source),
	  targetPos(source->getPosition().getX()+source->mouseX, source->getPosition().getY()+source->mouseY)
{
	sprite = "images/homingmissile.png";
	drawScale = 0.3;
	radius = (images.getImage(sprite.c_str())->getWidth() / 2) * drawScale * 0.5;
	exploding = false;
	acquireTarget();
	sendAdd();
}

void HomingMissile::acquireTarget(void)
{
	int best_target = -1;
	float best_dist = 10000;
	
	for(Server::Entpool::iterator ii=server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		if(!ii->second->isPlayer())
			continue;
		Player *ent = (Player*)ii->second;
		float dist = (ent->getPosition().positionVector() - targetPos).getMagnitude();
		if(dist > best_dist)
			continue;
		
		if(ent->shouldDelete() || !ent->canCollideWith(this))
			continue;
		if(ent->getTeam() == ownerTeam && ownerTeam != team_free)
			continue;
		
		best_target = ent->getEntId();
		best_dist = dist;
	}
	target = best_target;
}

ServerConsoleVar<float> missileTurnSpeed("missile_turnspeed", 1);

void HomingMissile::timepass(void)
{
	BaseMissile::timepass();
	if(target==-1)
		return;
	if(server->entities.find(target) == server->entities.end()) {
		target=-1;
		position.setR_vel(0.0);
		return;
	}
	Entity *ent = server->entities[target];
	Vector2D targetPosition = ent->getPosition().positionVector();
	Vector2D relPosition = targetPosition - position.positionVector();
	float targetHeading = atan2(-relPosition.y, relPosition.x);
	float angleDifference = position.getR() - targetHeading;
	while(angleDifference < -M_PI) angleDifference += 2*M_PI;
	while(angleDifference > M_PI) angleDifference -= 2*M_PI;
	
	if(abs(angleDifference) < missileTurnSpeed*getDt()) {
		position.setR(targetHeading);
		if(position.getR_vel() != 0.0)
			position.setR_vel(0.0);
	} else if(angleDifference > 0) {
		position.setR_vel(-missileTurnSpeed);
	} else {
		position.setR_vel(missileTurnSpeed);
	}
	position.setX_vel( missileSpeed * cos(position.getR()) );
	position.setY_vel( missileSpeed * -sin(position.getR()) );
}
//
//ClusterBomb::ClusterBomb(
//    const Position &pos, 
//	int numExplosions, 
//    Player *owner
//    ) : Shot(pos, owner)
//{
//    initialize(pos, numExplosions);
//    shooterEntID = owner->getEntId();
//
//	sendAdd();
//}
//
////alternate constructor, used to create copies (bounces) of this shot.
////the Shot base class constructor will see a null Owner pointer, and assign
////a shooter ID of -1, which will get overridden by this constructor.
//ClusterBomb::ClusterBomb(
//    const Position &pos, 
//	int numExplosions, 
//    int clientID, 
//    int shooterEntID
//    ) : Shot(pos, 0)
//{
//    initialize(pos, numExplosions);
//
//    //override the assignment done in the Shot constructor
//    shooter            = clientID;
//    this->shooterEntID = shooterEntID;
//
//	sendAdd();
//}
//
////initialize stuff that's common to both constructors
//void ClusterBomb::initialize(
//	const Position &pos, 
//	int numExplosions)
//{
//    numExplosionsLeft = numExplosions;
//
//    sprite    = "images/ast-small.png";
//	drawScale = 0.7;
//	radius    = (images.getImage(sprite.c_str())->getWidth() / 2) * drawScale * 0.5;
//	exploding = false;
//    initialX  = pos.getX();
//    initialY  = pos.getY();
//
//	//position.setR_vel(5);
//}
//
//void ClusterBomb::detonate()
//{
//	if (exploding || numExplosionsLeft <= 0)
//		return;
//
//	exploding = true;
//
//    float sourceX = initialX;
//    float sourceY = initialY;
//    float destX   = 0.0f;
//    float destY   = 0.0f;
//
//	//Player *nextTarget = 0;
//
//    //find up to numBounces targets and damage them
//	int explosionCounter = 0;
//	Server::Entpool::iterator ii;
//	for (ii = server->entities.begin(); ii != server->entities.end(); ++ii)
//    {
//        /*nextTarget = getNearestTargetFrom(nextTarget);
//        if (nextTarget == 0)
//            break;*/
//
//		Entity *nextTarget = ii->second;
//
//		Vector2D relPos = nextTarget->getPosition().positionVector() - getPosition().positionVector();
//		relPos.normalize();
//		relPos *= clusterBombSpeed;
//
//		Position startPos = getPosition();
//		startPos.setX_vel(relPos.x);
//		startPos.setY_vel(relPos.y);
//		
//		new ClusterBomb(
//			startPos, 
//			numExplosionsLeft - 1, 
//			shooter, 
//			shooterEntID);
//
//        /*float damageAmount = clusterBombDamage / i;
//
//        destX = nextTarget->getPosition().getX();
//        destY = nextTarget->getPosition().getY();
//        nextTarget->takeDamage(damageAmount, getDamageSource());*/
//
//        // draw graphical effect
//       /* SendMessage lightningMsg(message_lightning);
//            lightningMsg.putFloat(sourceX);
//            lightningMsg.putFloat(sourceY);
//            lightningMsg.putFloat(destX);
//            lightningMsg.putFloat(destY);
//        lightningMsg.sendToEveryone();*/
//
//        //update the source/destination coordinates for the later jumps
//        /*sourceX = destX;
//        sourceY = destY;*/
//
//		explosionCounter++;
//		if (explosionCounter > numExplosions)
//			break;
//    }
//
//
//	// Get rid of this ent
//	deleteMe();
//}
//
////find a (sufficiently close) target
////Player *ClusterBomb::getNearestTargetFrom(Entity *excludedEnt)
////{
////    Server::Entpool::iterator ii;
////    for (ii = server->entities.begin(); ii != server->entities.end(); ++ii)
////    {
////        Entity *currentEnt = ii->second;
////
////        if (currentEnt->isPlayer() && 
////            currentEnt != this && 
////            ii->first  != shooterEntID && 
////            currentEnt != excludedEnt && 
////            isEntityCloseEnough(currentEnt))
////        {
////            return (Player *)currentEnt;
////        }
////    }
////
////    return 0;   //no target found
////}
//
////bool ClusterBomb::isEntityCloseEnough(Entity *target)
////{
////    float distSquared = distSquaredBetween(
////                                    getPosition(), 
////                                    target->getPosition());
////
////    return (distSquared <= bounceRange * bounceRange);   
////}
