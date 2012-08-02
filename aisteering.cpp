#include "mastrix.hpp"

AISteering::AISteering(
	AIPlayer *owner, 
	Position *ownerPos)
{
	bot			= owner;
	botPosition = ownerPos;
}

AISteering::~AISteering()
{
	bot		    = 0;
	botPosition = 0;
}

//this actually causes the ship to move to a waypoint
void AISteering::moveTo(const Position &destination)
{
	turnTo(destination);

	if (isFacing(destination))
		bot->thrust(true);
}

//when you hit an intermediate goal, instantly change your heading and preserve
//your speed so that you don't have to stop (if the turn isn't too sharp)
//if the turn is too sharp, then you have to stop
void AISteering::faceNewGoal(const Position &destination)
{
	const float angleDifference = (float)fabs(getClampedAngleRad(
												  getAngleDifference(destination)));

	assert(angleDifference >= 0);
	assert(angleDifference <= 2 * M_PI);

	//angle difference is sufficiently small, so just change your direction
	bool leftTurnWideEnough  = (angleDifference >= 0 && angleDifference <= M_PI_2);
	bool rightTurnWideEnough = (angleDifference <= (2 * M_PI) && 
							    angleDifference >= M_PI_2 + M_PI);

	bool isTurnWideEnough = leftTurnWideEnough || rightTurnWideEnough;

	if (isTurnWideEnough)
	{
		float oldXVel  = botPosition->getX_vel();
		float oldYVel  = botPosition->getY_vel();
		float oldSpeed = sqrtf(oldXVel * oldXVel + oldYVel * oldYVel);

		botPosition->setR(getAngleFacingTowards(destination));
		float newAngle = botPosition->getR();

		//set the new speed based on the angle difference.
		//A low angle difference means little speed loss, while a sharper turn
		//(close to 90 degrees) means more speed loss
		float newSpeed;
		if (leftTurnWideEnough)
			newSpeed = oldSpeed * (1 - (angleDifference / M_PI_2));
			//newSpeed = oldSpeed * 0.2f;
		else
		{
			//the angle difference normalized to the 0..90 degree range
			float rightAngleDifference = (angleDifference  - (M_PI_2 + M_PI));
			newSpeed = oldSpeed * (1 - (rightAngleDifference / M_PI_2));
			//newSpeed = oldSpeed * 0.2f;
		}
			
		float newXVel = newSpeed *  cos(newAngle);
		float newYVel = newSpeed * -sin(newAngle);

		botPosition->setX_vel(newXVel);
		botPosition->setY_vel(newYVel);

		botPosition->setR_vel(0);
	}
	else	//turn is too sharp, so slow down (stop)
	{
		botPosition->setX_vel(0);
		botPosition->setY_vel(0);
		botPosition->setR_vel(0);

		bot->thrust(false);
		bot->brake(false);
	}	
}

//returns the angle difference (in radians) between the direction 
//that the bot is facing and the vector from the bot to the destination
//...(this does not apply any angle corrections!)
float AISteering::getAngleDifference(const Position &destination) const
{
	float angle		   = botPosition->getR();
	float correctAngle = getAngleFacingTowards(destination);
	
	float angleDifference = angle - correctAngle;

	return angleDifference;	
}

//returns the angle that the bot should have if it were facing the destination
float AISteering::getAngleFacingTowards(const Position &destination) const
{
	float deltaX = destination.getX() - botPosition->getX();
	float deltaY = destination.getY() - botPosition->getY();

	float angleFacingDest = atan2(-deltaY, deltaX);

	return angleFacingDest;
}

//returns true if the bot is facing or close to facing the destination point
bool AISteering::isFacing(const Position &destination) const
{
	return (fabs(getAngleDifference(destination)) < AIPlayer::angleErrorRange);
}

void AISteering::turnTo(const Position &destination)
{
	float uncorrectedAngleDifference = getAngleDifference(destination);
	float correctedAngleDifference   = uncorrectedAngleDifference;

	//angle corrections
	while(correctedAngleDifference <= -M_PI) 
		correctedAngleDifference += 2*M_PI;
	while(correctedAngleDifference >= M_PI) 
		correctedAngleDifference -= 2*M_PI;
	
	//check the uncorrected angle to see if you're facing the target
	//if not, then steer accordingly
	if( abs(correctedAngleDifference) < getDt()*bot->type->getTurnSpeed() )
	{
		botPosition->setR(getHeading(*botPosition, destination));
		bot->turnLeft(false);
		bot->turnRight(false);
	}
/*	if(isFacing(destination))
	{
		bot->turnLeft(false);
		bot->turnRight(false);	
	}*/
	else if(correctedAngleDifference > 0)
	{
		bot->turnLeft(false);
		bot->turnRight(true);
	}
	else
	{
		bot->turnLeft(true);
		bot->turnRight(false);
	}
}