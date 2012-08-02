#ifndef AISTEERING_HPP
#define AISTEERING_HPP

//represents the steering system for the bot.  Handles all turning and movement
//so that the bot focuses only on decision making.

class Position;
class AIPlayer;

class AISteering
{
public:
	AISteering(
		AIPlayer *owner = 0, 
		Position *ownerPos = 0);	
	~AISteering();
	
	void moveTo(const Position &destination);
	void turnTo(const Position &destination);	
	void faceNewGoal(const Position &destination);
	bool isFacing(const Position &destination) const;

private:
	float getAngleDifference(const Position &destination) const;
	float getAngleFacingTowards(const Position &destination) const;

	AIPlayer *bot;
	Position *botPosition;
};

#endif	//AISTEERING_HPP