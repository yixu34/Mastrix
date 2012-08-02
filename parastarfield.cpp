#include "mastrix.hpp"

Starfield::Starfield()
{
	reinit();
}

void Starfield::reinit(void)
{
	//Star Densities
	numSmallStars = 2000;
	numLargeStars = 200;

	int screenWidth  = graphics.getScreenWidth();
	int screenHeight = graphics.getScreenHeight();

	//clear the star data out
	smallStars.clear();
	largeStars.clear();

	//resize the vectors to appropriate sizes
	smallStars.resize(numSmallStars);
	largeStars.resize(numLargeStars);

	for (int i = 0; i < numSmallStars; i++) {smallStars[i].x = randFloat(0.0, screenWidth);}
	for (int i = 0; i < numSmallStars; i++) {smallStars[i].y = randFloat(0.0, screenHeight);}
	for (int i = 0; i < numSmallStars; i++) {
		smallStars[i].depth = randFloat(0.1, 0.9);
		smallStars[i].bright = (int)(/*randInt(155,235)*/ 255*smallStars[i].depth);
	}
	//for (int i = 0; i < numSmallStars; i++) {smallStars[i].bright = randInt(135,235);}

	for (int i = 0; i < numLargeStars; i++) {largeStars[i].x = randFloat(0, screenWidth);}
	for (int i = 0; i < numLargeStars; i++) {largeStars[i].y = randFloat(0, screenHeight);}
	for (int i = 0; i < numLargeStars; i++) {
		largeStars[i].depth = randFloat(0.1, 1.0);
		largeStars[i].bright = (int)(randInt(80,160)*largeStars[i].depth);
	}
	//for (int i = 0; i < numLargeStars; i++) {largeStars[i].bright = randInt(80,160);}
}


ClientConsoleVar<bool> enableStars("enable_stars", true);

void Starfield::draw(const Position &camera, const ClientViewport *viewport)
{
	int modx  = graphics.getScreenWidth();
	int mody = graphics.getScreenHeight();

	float offsetx = camera.getX()*zoom;
	float offsety = camera.getY()*zoom;
	float viewleft = viewport->left;
	float viewright = viewport->right;
	float viewtop = viewport->top;
	float viewbotm = viewport->bottom;

	// flicker offsets
	int z, zdim, zbright;
	
	if(!enableStars)
		return;
	
	//for drawing the group of points all at once
	glPointSize(1.0f);
	glBegin(GL_POINTS);	
		for (int i = 0; i < numSmallStars; i++)
		{
			float basex = smallStars[i].x-(offsetx*smallStars[i].depth);
			float basey = smallStars[i].y-(offsety*smallStars[i].depth);
			//int px = (int)(smallStars[i].x-(offsetx*smallStars[i].depth) + (modx<<7))%modx;
			//int py = (int)(smallStars[i].y-(offsety*smallStars[i].depth) + (mody<<7))%mody;
			float px = basex - modx*floor(basex/modx);
			float py = basey - mody*floor(basey/mody);
			if (px >= viewleft && px <= viewright && py >= viewtop && py <= viewbotm) {
				z = /*randInt(-20, 20) +*/ smallStars[i].bright;
				graphics.setColor(Color(z, z, z));
				glVertex2f(px, py);
			}
		}

		for (int i = 0; i < numLargeStars; i++)
		{
//			zdim = randInt(-20, 20) + largeStars[i].bright;
//			zbright = (int)(randInt(215,255)*largeStars[i].depth);
			zdim = largeStars[i].bright;
			zbright = largeStars[i].bright;

			//ColorX c(zdim, zdim, zdim);
			Color c(zdim, zdim, zdim);
			graphics.setColor(c);

			int px1 = (int)((largeStars[i].x - 1) - (offsetx*largeStars[i].depth) + (modx<<7)) % modx;
			int py1 = (int)((largeStars[i].y - 1) - (offsety*largeStars[i].depth) + (mody<<7)) % mody;
			int px2 = (int)((largeStars[i].x + 1) - (offsetx*largeStars[i].depth) + (modx<<7)) % modx;
			int py2 = (int)((largeStars[i].y + 1) - (offsety*largeStars[i].depth) + (mody<<7)) % mody;

			int px = (int)(largeStars[i].x-(offsetx*largeStars[i].depth) + (modx<<7))%modx;
			int py = (int)(largeStars[i].y-(offsety*largeStars[i].depth) + (mody<<7))%mody;
			// Dimmer surrounding points
			/*
			if (px1 >= viewleft && px1 < viewright) {
				if (py1 >= viewtop && py1 <= viewbotm) 
					graphics.drawVertex(px1, py1);
				if (py2 >= viewtop && py2 <= viewbotm) 
					graphics.drawVertex(px1, py2);
			}
			if (px2 >= viewleft && px2 < viewright) {
				if (py1 >= viewtop && py1 <= viewbotm) 
					graphics.drawVertex(px2, py1);
				if (py2 >= viewtop && py2 <= viewbotm) 
					graphics.drawVertex(px2, py2);
			}
			*/

			// "Core" bright point
			if (px >= viewleft && px <= viewright && py >= viewtop && py <= viewbotm) {
				//GameX.DrawPoint(ColorX(zbright,zbright,zbright), px, py);
				//graphics.drawPoint(Color(zbright, zbright, zbright), px, py);
				//graphics.setColor(Color(zbright, zbright, zbright));
				//graphics.drawVertex(px, py);
			}
		}
	glEnd();	//GL_POINTS
}

