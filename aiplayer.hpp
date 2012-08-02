#ifndef AIPLAYER_HPP
#define AIPLAYER_HPP

#include "aipathfinding.hpp"
#include "player.hpp"
#include <stack>
#include <queue>
#include <vector>

class AIPlayer:public Player
{
public:
	AIPlayer(
		float x, 
		float y, 
		ShipType *type, 
		team_type newTeam);
	virtual ~AIPlayer();

	std::string nextCommand;
	std::string deathTrigger;

	void timepass(void);
	void die(int killer);

	inline void setState(ai_state state)
	{	aiState = state; }

	inline ai_state getState() const
	{	return aiState; }

	inline ai_role getRole() const
	{	return aiRole; }

	/*void setStopAtNextGoal(bool stopAtNextGoal)
	{	shouldStopAtGoal = stopAtNextGoal; }*/


	//void setGoal(WaypointMarker *destination);
	//void setGoal(const Position &destination);

	int getCollisionEffect(void);
	bool isHuman(void) { return false; }

	/*void setNewGoal(WaypointMarker *destination);
	void setNewGoal(const Position &destination);*/
	void updateOldGoal();

	virtual bool isTurret() = 0;
	
	//indicates that the bot currently has no target
	static const int noTarget;

	//acceptable range of error for angle calculations, to minimize "wiggling"
	static const float angleErrorRange;
	
protected:
	ai_state		  aiState;
	ai_role			  aiRole;
	ai_steering_state aiSteeringState;
	
	Position targetPosition;

	int	targetID;
	
	float aimOffsetX, aimOffsetY;

	bool canAttack;
	
	virtual void runAI() = 0;
	
	void  updateTargetPosition();

	int   getBestTargetID() const;
	
	
	bool hasTarget();
	bool isEntityValid(Entity *ent) const;	
	bool isEntityVisible(Entity *ent) const;
	bool isEntityInRange(Entity *ent) const;
	bool shouldFaceTarget() const;

	void attackTarget(Entity *target);
	
	Position getPredictedTargetPosition();

	//returns the distance squared between a destination and the bot
	float distSquaredTo(const Position &destination) const;

	AISteering    steering;

	float shotInaccuracy;
};

class AIShip:public AIPlayer
{
public:
	AIShip(
		float x, 
		float y, 
		ShipType *type, 
		team_type newTeam);
	~AIShip();

	void setNewGoal(WaypointMarker *destination);
	void setNewGoal(const Position &destination);
	void updateOldGoal();

	bool isTurret() { return false; }

	void setStopAtNextGoal(bool stopAtNextGoal)
	{	shouldStopAtGoal = stopAtNextGoal; }
	
protected:
	bool shouldStopAtGoal;

	Position oldGoal;
	Position currentGoal;

	WaypointMarker *oldTargetWaypoint;

	void updateGoal();
	void runAI();
	void roam();
	void chase();
	void stop();

	//int   getBestTargetID() const;
	
	float rateEntity(Entity *ent);

	//bool  hasTarget();
	bool  isEntityValid(Entity *ent) const;	
	//bool  shouldFaceTarget() const;
	bool  isNearNextWaypoint() const;
	bool  isCloseToGoal();


	//used to determine roaming goals
	bool isNextWaypointFarEnough() const;
	WaypointMarker *getNearestWaypoint() const;
	WaypointMarker *getRandomGoalFrom(WaypointMarker *source) const;

	AIPathfinding pathfinding;
};

class AITurret:public AIPlayer
{
public:
	AITurret(
		float x, 
		float y, 
		ShipType *type, 
		team_type newTeam);
	~AITurret();

	bool isTurret() { return true; }

protected:
	void runAI();
};



#endif	//AIPLAYER_HPP
