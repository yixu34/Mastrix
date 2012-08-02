#ifndef CLENTITY_HPP
#define CLENTITY_HPP

#include <string>
#include "position.hpp"

class CLEntity
{
public:
	CLEntity(
		int newID, 
		std::string newSpriteName, 
		float drawScale, 
		Position &newPos,
		int type,
		int team,
		std::string label,
		int effect);
	virtual ~CLEntity();

	inline int  getID() const			   
		{ return entID; }
	inline void setPosition(const Position &pos) 
		{ position = pos; }
	inline Position &getPosition(void)
		{ return position; }
	inline int getType()
		{ return entType; }
	inline int getTeam()
		{ return entTeam; }
	inline float getRadius()
		{ return radius; }
	inline void setEngines(short whichEngines, float currRadius)
		{ enginesSet = whichEngines; radius = currRadius;}
	inline void setRadius(float r) {radius = r;}
	inline void setEffect(int e) {effect = e;}
	void setShield(float time, short type, float rad);
	//void drawShield();

	//virtual int getTeam() {return entTeam;}

//private:
	int			entID;
	std::string spriteName;
	float		scale;
	Position	position;
	//ImageX		*sprite;
	Image	   *sprite;
	short		enginesSet;
	float		radius;
	bool		shouldChangeColor;
	bool		isVisible;
	int			entType;
	int			effect;
	std::string label;
	float       shieldTime;
	float       shieldRadius;
	short       shieldType;

    bool        isAlly;

	int			entTeam;

	//Used to color bots for debugging
	Color		color;
};



#endif	//CL_ENTITY_HPP
