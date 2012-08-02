#include "mastrix.hpp"
#include <typeinfo>

ClientConsoleVar<bool> showEdgeLengths("show_edge_lengths", false);

CLEntity::CLEntity(
	int newID, 
	std::string newSpriteName, 
	float drawScale, 
	Position &newPos,
	int type,
	int team,
	std::string label,
	int effect)
{
	entID	     = newID;
	spriteName   = newSpriteName;
	scale	     = drawScale;
	position     = newPos;	
	sprite       = images.getImage(newSpriteName.c_str());
	entType	     = type;
	entTeam		 = team;
	enginesSet   = 0x0;
	color	     = Color(255, 255, 255);
	this->label  = label;
	this->effect = effect;
	shieldTime   = 0.0;
    isAlly       = false;
    shouldChangeColor = false;
}

CLEntity::~CLEntity()
{
	sprite = 0;
}

void CLEntity::setShield(float time, short type, float rad) {
	shieldRadius = rad; 
	shieldTime = time; 
	shieldType = type;
}

/*
void CLEntity::drawShield() {
	int density = 180;
	for (int i = 0; i < density; i++) {
		float origX = position.getX();
		float origY = position.getY();

		float dotX = position.getX() + radius*cos(randFloat(0.0, M_PI*2));
		float dotY = position.getY() + radius*sin(randFloat(0.0, M_PI*2));
		glEnable(GL_POINT_SMOOTH);
			graphics.drawPoint(Color(64,64,255),dotX,dotY,10);
		glDisable(GL_POINT_SMOOTH);
	}
}
*/

void Client::drawWaypointEdges()
{
	if (!areEdgesVisible)
		return;

	for (CLEdgePool::iterator ii = edges.begin(); ii != edges.end(); ++ii)
	{
		ClientEdge &currentEdge = (*ii);

		if (entpool.find(currentEdge.getEnt1ID()) == entpool.end())
			return;
		if (entpool.find(currentEdge.getEnt2ID()) == entpool.end())
			return;
		Position &point1 = entpool[currentEdge.getEnt1ID()]->getPosition();
		Position &point2 = entpool[currentEdge.getEnt2ID()]->getPosition();

		float point1DrawX = point1.getX();
		float point1DrawY = point1.getY();
		float point2DrawX = point2.getX();
		float point2DrawY = point2.getY();

		//now convert these world coordinates into drawing coordinates
		convertToDrawCoords(point1DrawX, point1DrawY);
		convertToDrawCoords(point2DrawX, point2DrawY);

		graphics.drawLine(
				Color(255, 0, 0), 
				point1DrawX, 
				point1DrawY, 
				point2DrawX, 
				point2DrawY);

		if (showEdgeLengths)
		{
			float dist = sqrtf(distSquaredBetween(
										Position(point1.getX(), point1.getY()), 
										Position(point2.getX(), point2.getY())));
			int x;
			int y;
			graphics.drawText(
				fcvt(dist, 0, &x, &y), 
				(point2DrawX + point1DrawX) / 2, 
				(point2DrawY + point1DrawY) / 2);
		}
	}
}

void Client::drawMapBorders()
{
	static Color borderColor = Color(255, 255, 0);

	//need to convert world border coordinates to camera coordinates
	float topLeftX     = leftBorder;
	float topLeftY     = topBorder;

	float topRightX    = rightBorder;
	float topRightY    = topBorder;

	float bottomRightX = rightBorder;
	float bottomRightY = bottomBorder;

	float bottomLeftX  = leftBorder;
	float bottomLeftY  = bottomBorder;

	convertToDrawCoords(topLeftX, topLeftY);
	convertToDrawCoords(topRightX, topRightY);
	convertToDrawCoords(bottomRightX, bottomRightY);
	convertToDrawCoords(bottomLeftX, bottomLeftY);

	const float borderWidth = 10.0f;
	
	glColor3f(1, 1, 0);
	glLineWidth(3);
	glBegin(GL_LINE_LOOP);
		glVertex2f(topLeftX,     topLeftY);
		glVertex2f(topRightX,    topRightY);
		glVertex2f(bottomRightX, bottomRightY);
		glVertex2f(bottomLeftX,  bottomLeftY);
	glEnd();
}


void Client::drawRects()
{
	for(unsigned ii=0; ii<rectpool.size(); ii++)
	{
		float min_x = rectpool[ii].min_x,
		      max_x = rectpool[ii].max_x,
		      min_y = rectpool[ii].min_y,
		      max_y = rectpool[ii].max_y;
		convertToDrawCoords(min_x, min_y);
		convertToDrawCoords(max_x, max_y);
		
		glColor3f(0.6, 0.6, 1.0);
		glBegin(GL_LINE_LOOP);
			glVertex2f(min_x, min_y);
			glVertex2f(max_x, min_y);
			glVertex2f(max_x, max_y);
			glVertex2f(min_x, max_y);
		glEnd();
		glColor4f(0.6, 0.6, 1.0, 0.3);
		glBegin(GL_QUADS);
			glVertex2f(min_x, min_y);
			glVertex2f(max_x, min_y);
			glVertex2f(max_x, max_y);
			glVertex2f(min_x, max_y);
		glEnd();
		graphics.drawText(rectpool[ii].name.c_str(), min_x+2, max_y-2);
	}
}

static void drawShot(int level, float x, float y);

void Client::drawEnt(CLEntity *ent)
{
	if (!ent->isVisible)
		return;

	float draw_x = ent->position.getX()*zoom - getCamera().getX()*zoom + 
				   viewport->center_x;
	float draw_y = ent->position.getY()*zoom - getCamera().getY()*zoom + 
				   viewport->center_y;
	float draw_radius = ent->scale*zoom;
	float size = ent->sprite->getWidth() * draw_radius / 2;
	
	if(draw_x+size < 0 || draw_x-size > viewport->right || draw_y+size < 0 || draw_y-size > viewport->bottom)
		return;
	
	//if this is a bot, color it if it's about to collide with something
	if (ent->shouldChangeColor)
		graphics.setColor(ent->color);
	else
		graphics.setColor(Color(255, 255, 255));
	
	//our new rotation argument automatically converts from radians to degrees
	graphics.drawColoredImage(
		ent->sprite, 
		draw_x, draw_y, 
		ent->position.getR(), 
		draw_radius);

    //draw an icon to show that the bot is allied with the human player
/*    if (ent->isAlly)
    {
        graphics.drawImage(
            images.getImage("images/waypoint.png"), 
            draw_x, 
            draw_y - 50 * zoom, 
            0.0f, 
            0.2f);
    }*/
	
	if(ent->enginesSet)
		particlePool.engineExhaust(ent->getID(), ent->radius, ent->enginesSet);
	
	switch(ent->effect)
	{
		case effect_sparking: particlePool.spark(ent->getPosition()); break;
		case effect_trail:    particlePool.trail(ent->getPosition(), ent->scale*ent->sprite->getWidth()/2); break;
		case effect_missile_trail: particlePool.missileTrail(ent->getPosition(), ent->scale*ent->sprite->getWidth()/2); break;
		case effect_shot:     drawShot(1, draw_x, draw_y); break;
		case effect_shot2:    drawShot(2, draw_x, draw_y); break;
		case effect_shot3:    drawShot(3, draw_x, draw_y); break;
		case effect_shot4:    drawShot(4, draw_x, draw_y); break;
		case effect_money:   particlePool.moneyTrail(ent->getPosition(), ent->scale*ent->sprite->getWidth()/2); break;
		default: break;
	}
	
	graphics.drawTextCentered(ent->label.c_str(), draw_x, draw_y+size+10);
	
	if(selectedEntId == ent->entID)
	{
		Color selectedColor(0, 255, 0);
		graphics.drawLine(selectedColor, draw_x-size, draw_y-size, draw_x+size, draw_y-size);
		graphics.drawLine(selectedColor, draw_x+size, draw_y-size, draw_x+size, draw_y+size);
		graphics.drawLine(selectedColor, draw_x+size, draw_y+size, draw_x-size, draw_y+size);
		graphics.drawLine(selectedColor, draw_x-size, draw_y+size, draw_x-size, draw_y-size);
	}
}

const float shot_size   = 6.5;

static void drawShot(int level, float x, float y)
{
	float jitter = 0.7;
	
	glPointSize(11 * zoom);
	glEnable(GL_POINT_SMOOTH);
	
	switch(level)
	{
		default:
		case 1:
			glBegin(GL_POINTS);
				glColor4f(0.0, 1.0, 0.0, 0.6);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
				glColor4f(0.3, 0.0, 1.0, 0.6);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
				glColor4f(1.0, 1.0, 1.0, 0.8);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
			glEnd();
			break;
		case 2:
			glBegin(GL_POINTS);
				glColor4f(1.0, 0.0, 0.0, 1.0);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
				glColor4f(1.0, 1.0, 0.0, 0.8);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
			glEnd();
			break;
		case 3:
			jitter = 2.0;
			glBegin(GL_POINTS);
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
				glColor4f(0.7, 0.7, 1.0, 1.0);
				glVertex2f(x+randFloat(-jitter,jitter), y+randFloat(-jitter,jitter));
			glEnd();
			break;
	}
}

