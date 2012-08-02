#include "mastrix.hpp"

ClientConsoleVar<int> radarSize("radar_size", 150);
ClientConsoleVar<bool> showRadar("show_radar", true);

Radar::Radar()
{
	radarColor = Color(0, 64, 0, 200);
}

void Radar::setView(int l, int r, int t, int b)
{
	if (dimhor > dimvert) {
		xleft   = r - 5 - radarSize;
		xright  = r - 5;
		ytop    = t + 5;
		ybottom = t + 5 + (dimvert/dimhor)*radarSize;
	}
	else {
		xleft   = r - 5 - (dimhor/dimvert)*radarSize;
		xright  = r - 5;
		ytop    = t + 5;
		ybottom = t + 5 + radarSize;
	}
}

void Radar::setBounds(float l, float r, float t, float b)
{
	boundleft   = l;
	boundright  = r;
	boundtop    = t;
	boundbottom = b;
	dimhor = boundright - boundleft;
	dimvert = boundbottom - boundtop;
}

void Radar::draw()
{
	if(!showRadar)
		return;

	graphics.drawRect(
		radarColor, 
		xleft, 
		ytop, 
		xright, 
		ybottom);
}

void Radar::draw(const Position &pos, int type, int team, float size, int ID, int effect, int avatar)
{
	if(!showRadar)
		return;
	
	RadarBlip blip;

	switch (type) {
		case ent_player_ship:
			if(effect==effect_money && ((int)(getTime()*3))%2) {
				blip.r = 255;
				blip.g = 255;
				blip.b = 255;
				size=2.0f;
			}
			else if (ID == avatar) {
				blip.r = 64;
				blip.g = 255;
				blip.b = 64;
				size=1.0f;
			}
			else if (team == team_red) {
				blip.r = 255;
				blip.g = 64;
				blip.b = 64;
				size=1.0f;
			} else if (team == team_blue){
				blip.r = 64;
				blip.g = 64;
				blip.b = 255;
				size=1.0f;
			}
			else {
				blip.r = 255;
				blip.g = 128;
				blip.b = 64;
				size=1.0f;
			}
            break;

		case ent_ai_ally_ship:
			blip.r = 255;
			blip.g = 255;
			blip.b = 64;
			size=1.0f;
			break;
        case ent_ai_enemy_ship:
            blip.r = 255;
			blip.g = 64;
			blip.b = 64;
			size=1.0f;
            break;
		case ent_large_obstacle:
		case ent_rectangular:
			blip.r = 0;
			blip.g = 24;
			blip.b = 0;
			break;
		case ent_flag:
			blip.r = blip.g = blip.b = 220;
			break;
		case ent_powerup:
		case ent_invisible:
		default:
			return;
	}
	//ColorX c(blip.r, blip.g, blip.b);
	Color blipColor(blip.r, blip.g, blip.b);
	float adjustx = pos.getX();
	float adjusty = pos.getY();
	if (dimhor > dimvert) {
		adjustx = ((adjustx - boundleft)/(boundright - boundleft))*radarSize;
		adjusty = ((adjusty - boundtop)/(boundbottom - boundtop))*(dimvert/dimhor)*radarSize;
	} else {
		adjustx = ((adjustx - boundleft)/(boundright - boundleft))*(dimhor/dimvert)*radarSize;
		adjusty = ((adjusty - boundtop)/(boundbottom - boundtop))*radarSize;
	}
	if(type==ent_large_obstacle){
		glEnable(GL_POINT_SMOOTH);
		graphics.drawPoint(
			blipColor, 
			xleft+adjustx,
			ytop + adjusty,
			(dimhor > dimvert) ? 
			(size*2/(boundbottom - boundtop))*(dimvert/dimhor)*radarSize :
			(size*2/(boundright - boundleft))*(dimhor/dimvert)*radarSize);
		glDisable(GL_POINT_SMOOTH);

	}
	else if (type == ent_rectangular) {
		float rectSize = size/radarSize;
		if (abs(pos.getR()) < M_PI/4) {
			graphics.drawRect(
				blipColor,
				xleft+adjustx-rectSize,
				ytop+adjusty-(rectSize/4),
				xleft+adjustx+rectSize,
				ytop+adjusty+(rectSize/4));
		} else {
			graphics.drawRect(
				blipColor,
				xleft+adjustx-(rectSize/4),
				ytop+adjusty-rectSize,
				xleft+adjustx+(rectSize/4),
				ytop+adjusty+rectSize);
		}
	} else {
		graphics.drawRect(
			blipColor, 
			xleft + adjustx -1, 
			ytop + adjusty -1, 
			xleft + adjustx +1, 
			ytop + adjusty +1);
/*
		glEnable(GL_POINT_SMOOTH);
		graphics.drawPoint(
			blipColor,
			xleft + adjustx,
			ytop + adjusty,
			2.0);
*/
	}
}

