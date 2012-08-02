#include "mastrix.hpp"

ServerConsoleVar<float> botInaccuracy("bot_inaccuracy", 30.0f);
ServerConsoleVar<float> botVisibleRange("bot_visible_range", 1500.0f);
ServerConsoleVar<float> botFiringRange("bot_firing_range", 1500.0f);
ServerConsoleVar<float> minWaypointDistance("min_waypoint_dist", 1500.0f);
ServerConsoleVar<bool>  enableChase("enable_chase", true);
ServerConsoleVar<bool>  botsFrozen("bots_frozen", false);
ServerConsoleVar<float> turretHitpointScale("turret_hitpoint_scale", 5.5f);


SERVER_CONSOLE_COMMAND(bot_destination)
{
	if (argc < 3) 
	{
		printfToClient(who, "Usage: bot_destination entID x y");
		return;
	}

	int botID = atoi(argv[0]);
	float x   = atof(argv[1]);
	float y   = atof(argv[2]);

	if(server->entities.find(botID) == server->entities.end()) return;
	if(!server->entities[botID]->isPlayer() || server->entities[botID]->isHuman()) return;
	
	AIPlayer *bot = (AIPlayer *)server->entities[botID];

	if (!bot->isTurret())
	{
		((AIShip *)bot)->setNewGoal(Position(x, y));
		bot->setState(state_roaming);
	}
}

SERVER_CONSOLE_COMMAND(bot_on_finish)
{
	if (argc < 2)
	{
		printfToClient(who, "Usage: bot_destination entID var");
		return;
	}

	int botID	  = atoi(argv[0]);
	
	if(server->entities.find(botID) == server->entities.end()) return;
	if(!server->entities[botID]->isPlayer() || server->entities[botID]->isHuman()) return;
	
	AIPlayer *bot = (AIPlayer *)server->entities[botID];
	
	bot->nextCommand = argv[1];
}

SERVER_CONSOLE_COMMAND(bot_on_die)
{
	if(argc < 2)
	{
		printfToClient(who, "Usage: bot_on_die entID var");
		return;
	}
	int botID	  = atoi(argv[0]);
	
	if(server->entities.find(botID) == server->entities.end()) return;
	if(!server->entities[botID]->isPlayer() || server->entities[botID]->isHuman()) return;
	
	AIPlayer *bot = (AIPlayer *)server->entities[botID];
	
	bot->deathTrigger = argv[1];
}

SERVER_CONSOLE_COMMAND(bot_stop)
{
	if (argc < 1)
	{
		printfToClient(who, "Usage: bot_stop entID");
		return;
	}

	int botID = atoi(argv[0]);
	
	if(server->entities.find(botID) == server->entities.end()) return;
	if(!server->entities[botID]->isPlayer() || server->entities[botID]->isHuman()) return;
	AIPlayer *bot = (AIPlayer *)server->entities[botID];

	bot->setState(state_stopping);
}

//static initializers
const int AIPlayer::noTarget = -1;
const float AIPlayer::angleErrorRange = 0.045f;

AIPlayer::AIPlayer(
	float x, 
	float y, 
	ShipType *type, 
	team_type newTeam)
	: Player(type)
{
	position.setX(x);
	position.setY(y);
	position.setR(0);

	//for now, let's only deal with one type of ship - aggressive chasers
	aiState		    = state_roaming;
	aiRole		    = role_chaser;
	aiSteeringState = steering_state_at_new_dest;

	targetID = noTarget;

	////this is the waypoint that the bot is nearest to upon spawning
	//WaypointMarker *source = getNearestWaypoint();
	//currentGoal			   = source->getPosition();
	//oldGoal				   = getPosition();

	//highlight the goal for testing
	/*WaypointMarker *dest = server->getWaypointNearestTo(currentGoal);
	server->highlightEntity(dest);	*/
	
	//initialize the bot's steering system
	steering = AISteering(this, &position);

	targetPosition = Position(0, 0);
	aimOffsetX = aimOffsetY = 0;
	nextCommand = "";

	//if this is a tanker, then ally with the human team
	/*if (type->isOnHumanTeam())
	{
		setTeam(team_human);
		canAttack = false;
	}
	else
	{
		setTeam(team_enemy);
		canAttack = true;
	}*/
	setTeam(newTeam);

    canAttack = type->hasWeapons();

	//give human allied turrets extra hitpoints
	/*if (canAttack && getTeam() == team_human && !type->isMobile())
		health = type->getHitpoints() * turretHitpointScale;*/

    shotInaccuracy = randFloat(-botInaccuracy, botInaccuracy);

    bool isHumanAlly = getTeam() == team_human;
    if (isHumanAlly)
        entType = ent_ai_ally_ship;
    else
        entType = ent_ai_enemy_ship;

	sendAdd();

    if (isHumanAlly)
        server->markAsAlly(this);
}

AIPlayer::~AIPlayer()
{}

int AIPlayer::getCollisionEffect(void)
{
	return collide_damage | collide_no_gravity;
}

void AIPlayer::timepass()
{
	Player::timepass();

	if (shieldTime > 0.0) {
		shieldTime -= getDt();
	} else {
		shielding = false;
		shieldTime = 0.0;
	}

	Entity::timepass();
	runAI();
}

//given the bot's position and shot velocity and the 
//target's position and velocity, predict where the target will be so that
//the bot leads its shots.
Position AIPlayer::getPredictedTargetPosition()
{
	Vector2D targetVelocity = targetPosition.velocityVector();

	//assuming that shots don't get ship velocity,  (this is true by default)
	//we use the shot's velocity as the source velocity
	float currentShotSpeed = weapons.primary()->getSpeed();

	float shotVelX =  currentShotSpeed * cos(getPosition().getR());
	float shotVelY = -currentShotSpeed * sin(getPosition().getR());
	Vector2D sourceVelocity = Vector2D(shotVelX, shotVelY);

	//...here I was going to account for shots getting ship velocity, but
	//I can't seem to find where that's being handled...
	
	Vector2D relVelocity    = targetVelocity - sourceVelocity;
	Vector2D relPosition    = targetPosition.positionVector() - getPosition().positionVector();
	
	float closingTime       = relPosition.getMagnitude() / 
		                      relVelocity.getMagnitude();
	
	Vector2D predictedPos   = targetPosition.positionVector() + (targetVelocity * closingTime); 
	
	float inaccuracyFactor = targetVelocity.getMagnitude() + 150;
	
	//now add some inaccuracy
	predictedPos.x += aimOffsetX*inaccuracyFactor;
	predictedPos.y += aimOffsetY*inaccuracyFactor;
	aimOffsetX += randFloat(-.03, .03);
	aimOffsetY += randFloat(-.03, .03);
	aimOffsetX *= 0.99;
	aimOffsetY *= 0.99;

	////...and now improve your aim
	//if (shotInaccuracy > 0)
	//	shotInaccuracy -= 0.1f;
	//else
	//	shotInaccuracy += 0.1f;

	return Position(predictedPos);
}

//called each frame to keep track of the target's position (if it's alive)
//assumed that hasTarget() has been called just before this!
void AIPlayer::updateTargetPosition()
{
	Entity *target = server->entities[targetID];
	targetPosition = target->getPosition();
}

//returns true if it's time to start facing the target (in chase mode)
bool AIPlayer::shouldFaceTarget() const
{
	float maxSpeed = getMaxSpeed();
	return (getSpeedSquared() >= maxSpeed * maxSpeed);
}



bool AIPlayer::hasTarget()
{
	if (server->entities.find(targetID) == server->entities.end())
	{
		targetID = noTarget;
		return false;
	}
	else
	{
		Entity *target = server->entities[targetID];
		return isEntityVisible(target);
	}
}


float AIPlayer::distSquaredTo(const Position &destination) const
{
	return distSquaredBetween(position, destination);
}

//search through the server entities and find a target
int AIPlayer::getBestTargetID() const
{
	//if you already have a valid target, then keep it
	if (targetID != noTarget)
		return targetID;

	Server::Entpool::iterator ii;
	for (ii = server->entities.begin(); ii != server->entities.end(); ++ii)
	{
		if (isEntityValid(ii->second))
			return ii->first;
	}

	return noTarget;
}

//returns true if ent is a valid target for the bot, false otherwise
//..."valid" conditions:  (for now)
//...1) close enough (within botVisibleRange)
//...2) is a ship
//...3) not the bot itself
//...maybe more to come
bool AIPlayer::isEntityValid(Entity *ent) const
{
	bool isCloseEnough = isEntityVisible(ent);

	//TODO:  add a virtual function, i.e. isShip().
	//Only targeting human players for now
	bool isPlayer	   = ent->isPlayer();
	bool isNotSelf	   = ent != this;

	bool isEnemy = false;
	if (isPlayer)
	{
		Player *targetShip = (Player *)ent;
		isEnemy = !(targetShip->getTeam() == getTeam());// && 
			      //  targetShip->getTeam() == team_free);
	}

	return (isCloseEnough && isNotSelf && isPlayer && isEnemy);
}

bool AIPlayer::isEntityVisible(Entity *ent) const
{
	float minDistSquared = botVisibleRange * botVisibleRange;

	return (distSquaredBetween(
						position, 
						ent->getPosition()) <= minDistSquared);
}

bool AIPlayer::isEntityInRange(Entity *ent) const
{
	float minRangeSquared = botFiringRange * botFiringRange;

	return (distSquaredBetween(
						position, 
						ent->getPosition()) <= minRangeSquared);
}

void AIPlayer::attackTarget(Entity *target)
{
	//shoot your target if it happens to cross your path
	if (steering.isFacing(targetPosition) && 
		isEntityVisible(target) && 
		isEntityInRange(target))
		fire1(true);
	else
		fire1(false);
}

void AIPlayer::die(int killer)
{
	serverScripting.defineVariable(deathTrigger, retprintf("%i", killer), tag_map_temporary);
	Player::die(killer);
}


//
// AIShip methods
//
AIShip::AIShip(
	float x, 
	float y, 
	ShipType *type, 
	team_type newTeam
	) : AIPlayer(x, y, type, newTeam)
{
	//this is the waypoint that the bot is nearest to upon spawning
	WaypointMarker *source = getNearestWaypoint();
	currentGoal			   = source->getPosition();
	oldGoal				   = getPosition();

	WaypointMarker *dest = server->getWaypointNearestTo(currentGoal);
	server->highlightEntity(dest);

	shouldStopAtGoal = false;

	oldTargetWaypoint = 0;
}

AIShip::~AIShip()
{}

void AIShip::runAI()
{
  /*  if (botsFrozen)
        aiState = state_stopping;
    else
        aiState = state_chasing;*/

	updateGoal();

	switch(aiState)
	{
	case state_roaming:
		roam();
		break;
	case state_chasing:
		chase();
		break;
	case state_evading:
		break;
	case state_stopping:
		stop();
		break;
	};

//	pathfinding.printTraversalSequence();
}

void AIShip::updateGoal()
{
	//you've approximately reached your goal, so roam to a new one
	if (isCloseToGoal())
	{
		//refill the traversal stack if empty (if we're at the destination)
		if (pathfinding.getNumRemainingWaypoints() == 0)
		{
			if(nextCommand != "") {
				serverScripting.defineVariable(nextCommand, "", tag_map_temporary);
			}
			WaypointMarker *oldDest = getNearestWaypoint();
			server->unhighlightEntity(oldDest);
			WaypointMarker *newDest = getRandomGoalFrom(oldDest);
			assert(newDest != oldDest);

			setNewGoal(newDest);
			server->highlightEntity(newDest);
		}
		else
		{
			currentGoal = pathfinding.getNextWaypoint()->getPosition();
			updateOldGoal();
		}
		
		steering.faceNewGoal(currentGoal);

		aiSteeringState = steering_state_at_new_dest;
	}//if
}


//either pick a random neighbor, or randomly pick a destination and 
//construct a path
void AIShip::roam()
{
	steering.moveTo(currentGoal);

	//make sure we're not shooting!
	fire1(false);

	//if this is a tanker (non-combat bot) then make sure we can only roam
	if (!(canAttack && enableChase))
		return;

	//try to find a suitable target
	targetID = getBestTargetID();
	if (targetID != noTarget)
	{
		//this might act up if a path doesn't exist to the target waypoint
		aiState = state_chasing;
	}
}

void AIShip::chase()
{
	if (!(hasTarget() && canAttack))
	{
		aiState = state_roaming;
		return;
	}

	updateTargetPosition();

	//chase the target (if you're close to the first intermediate goal, 
	//pop it off the stack and pick the next one)
	Entity *target = server->entities[targetID];
	if (isCloseToGoal()) 
	{
		Position oldPos = currentGoal;
		WaypointMarker *newDest = server->getWaypointNearestTo(target->getPosition());
		setNewGoal(newDest);

        if (!pathfinding.doesPathExistTo(newDest))
            setNewGoal(getNearestWaypoint()->getRandomNeighbor());

        //keeps bots from stopping at one place
		if (currentGoal == oldPos)
            setNewGoal(server->getWaypointNearestTo(oldGoal)->getRandomNeighbor());

		//pathfinding.getNextWaypoint();
	}

	//shoot your target if it happens to cross your path
	attackTarget(target);

	//server->unhighlightEntity(this);

	switch(aiSteeringState)
	{
	//we just arrived at a new waypoint; now decide if the waypoint is far 
	//enough so that we can thrust, turn, and thrust again.  If not, just
	//thrust.
	case steering_state_at_new_dest:
		if (isNextWaypointFarEnough())
			aiSteeringState = steering_state_thrust_to_dest;
		else
			aiSteeringState = steering_state_dest_too_close;
		break;

	case steering_state_thrust_to_dest:
		steering.moveTo(currentGoal);

		if (shouldFaceTarget())
			aiSteeringState = steering_state_face_target;
		break;

	case steering_state_face_target:
		thrust(false);
		brake(false);

		if (isNearNextWaypoint())
		{
			if(steering.isFacing(currentGoal))
				thrust(true);
			steering.turnTo(currentGoal);
			fire1(false);
			aiSteeringState = steering_state_return_to_dest;
		}
		else
		{
			//steering.turnTo(targetPosition);
			steering.turnTo(getPredictedTargetPosition());
			//fire1(true);
			attackTarget(target);
		}
		break;

	case steering_state_return_to_dest:
		/*steering.turnTo(currentGoal);

		if (steering.isFacing(currentGoal))
			aiSteeringState = steering_state_thrust_to_dest;*/
		steering.moveTo(currentGoal);
		break;

	case steering_state_dest_too_close:
		steering.moveTo(currentGoal);
		break;
	};
}

void AIShip::stop()
{
	brake(false);
	thrust(false);
	fire1(false);

	getPosition().setX_vel(0);
	getPosition().setY_vel(0);
}

//TODO:  fix the vector projections so that the bot stops more accurately
bool AIShip::isCloseToGoal()
{
	return (distSquaredTo(currentGoal) <= 9000.0f);	

	/*Position relPosBetweenGoals = currentGoal - oldGoal;
	Position botRelPos          = getPosition() - oldGoal;
	
	return (fabs(botRelPos * relPosBetweenGoals) <= 12);*/


	/*Position relPos       = currentGoal - oldGoal;
	float relPosMagnitude = relPos.getMagnitude();
	Position posAlongDest = Position(
								relPos.getY(), 
								-relPos.getX()) / relPosMagnitude;

	Position projection = posAlongDest * (getPosition() * posAlongDest);
	float distToDest    = (getPosition() - projection - oldGoal).getMagnitude();

	return (distToDest > relPosMagnitude);*/
}

bool AIShip::isNextWaypointFarEnough() const
{
	float waypointDistSquared = distSquaredBetween(
											position, 
											currentGoal);

	return (waypointDistSquared >= minWaypointDistance);
}

bool AIShip::isNearNextWaypoint() const
{
	return (distSquaredBetween(position, currentGoal) <= 15000.0f);
}

//find the map waypoint that is most nearly corresponds to your current position
WaypointMarker *AIShip::getNearestWaypoint() const
{
	return (server->getWaypointNearestTo(position));
}

//find the a random goal waypoint starting from your current waypoint
//(i.e. pick a random waypoint that is not the one you are at right now)
WaypointMarker *AIShip::getRandomGoalFrom(WaypointMarker *source) const
{
	assert(source != 0);

	const Server::WaypointPool &serverWaypoints = server->waypoints;
	const int maxWaypointIndex = serverWaypoints.size() - 1;

	int randomWaypointIndex			 = randInt(0, maxWaypointIndex);
	WaypointMarker *possibleWaypoint = serverWaypoints.at(randomWaypointIndex);

	//found a collision; the random index happened to map to the source waypoint
	//randomly select another index -around- that one
	if (possibleWaypoint == source)
	{
		if (randomWaypointIndex == 0)
			randomWaypointIndex = randInt(1, maxWaypointIndex);
		else if (randomWaypointIndex == maxWaypointIndex)
			randomWaypointIndex = randInt(0, maxWaypointIndex - 1);
		else
		{
			//the general case, that a collision is found in the middle
			//we split the waypoint vector at the collision; the side with more
			//indices will have a higher chance of being picked.  We then pick 
			//a random index from within that side.
			const int numIndicesOnLeft  = randomWaypointIndex;
			const int numIndicesOnRight = maxWaypointIndex - randomWaypointIndex;

			const float leftProbability = (float)numIndicesOnLeft / 
										  (float)(numIndicesOnLeft + numIndicesOnRight);

			if (randFloat(0, 1) <= leftProbability)
				randomWaypointIndex = randInt(0, randomWaypointIndex - 1);
			else
				randomWaypointIndex = randInt(randomWaypointIndex + 1, maxWaypointIndex);
		}
	}

	//some sanity checks
	assert(randomWaypointIndex >= 0);
	assert((unsigned)randomWaypointIndex < server->waypoints.size());
	assert(server->waypoints.at(randomWaypointIndex) != 0);

	return serverWaypoints.at(randomWaypointIndex);
}

//determine the strength/weakness of the target entity based on
//its hitpoints, mass, size, and proximity
float AIShip::rateEntity(Entity *ent)
{
	return 0.0f;
}


//used to specify a waypoint that the bot should want to go to.
//useful for issuing bot movement commands via scripting.
void AIShip::setNewGoal(WaypointMarker *destination)
{
	//discard all previous intermediate destinations...
	pathfinding.clearTraversalSequence();
	
	pathfinding.constructShortestPathTo(
		getNearestWaypoint(),
		destination);

	currentGoal = pathfinding.getNextWaypoint()->getPosition();
	updateOldGoal();	
}

void AIShip::setNewGoal(const Position &destination)
{
	setNewGoal(server->getWaypointNearestTo(destination));
}

void AIShip::updateOldGoal()
{
	oldGoal = getPosition();
}


//
// AITurret methods
//
AITurret::AITurret(
	float x, 
	float y, 
	ShipType *type, 
	team_type newTeam
	):AIPlayer(x, y, type, newTeam)
{}

AITurret::~AITurret()
{}

void AITurret::runAI()
{
	if (!canAttack)
		return;

	//try to find a suitable target
	targetID = getBestTargetID();
	if (hasTarget())
	{
		updateTargetPosition();
		steering.turnTo(getPredictedTargetPosition());
		
		Entity *target = server->entities[targetID];
	
		attackTarget(target);
	
	}
	else
	{
		fire1(false);
	}
}
