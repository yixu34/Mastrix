#if 0

#include "mastrix.hpp"


void Client::initBackground()
{
	//Star Densities
	numSmallStars = 1000;
	numLargeStars = 50;

	//// XY coordinates and base brightness of stars
	//smallStarsX = new int[numSmallStars];
	//smallStarsY = new int[numSmallStars];
	//smallStarsB = new int[numSmallStars];
	//largeStarsX = new int[numLargeStars];
	//largeStarsY = new int[numLargeStars];
	//largeStarsB = new int[numLargeStars];

	//// Randomly select base positions for stars
	//for(int i = 0; i < numSmallStars; i++) {smallStarsX[i] = randInt(0, screenWidth);}
	//for(int i = 0; i < numSmallStars; i++){smallStarsY[i] = randInt(0, screenHeight);}
	//for(int i = 0; i < numSmallStars; i++){smallStarsB[i] = randInt(135, 235);}

	//for(int i = 0; i < numLargeStars; i++){largeStarsX[i] = randInt(0, screenWidth);}
	//for(int i = 0; i < numLargeStars; i++){largeStarsY[i] = randInt(0, screenHeight);}
	//for(int i = 0; i < numLargeStars; i++){largeStarsB[i] = randInt(80, 160);}
	int screenWidth  = graphics.getScreenWidth();
	int screenHeight = graphics.getScreenHeight();

	//clear the star data out
	smallStarsX.clear();
	smallStarsY.clear();
	smallStarsB.clear();

	largeStarsX.clear();
	largeStarsY.clear();
	largeStarsB.clear();

	//resize the vectors to appropriate sizes
	smallStarsX.resize(numSmallStars);
	smallStarsY.resize(numSmallStars);
	smallStarsB.resize(numSmallStars);

	largeStarsX.resize(numLargeStars);
	largeStarsY.resize(numLargeStars);
	largeStarsB.resize(numLargeStars);

	for(int i = 0; i < numSmallStars; i++) 
	{
		smallStarsX[i] = randInt(0, screenWidth);
		smallStarsY[i] = randInt(0, screenHeight);
		smallStarsB[i] = randInt(135, 235);
	}

	for(int i = 0; i < numLargeStars; i++)
	{
		largeStarsX[i] = randInt(0, screenWidth);
		largeStarsY[i] = randInt(0, screenHeight);
		largeStarsB[i] = randInt(80, 160);
	}
}


ClientConsoleVar<bool> enableStars("enable_stars", true);

void Client::drawBackground(const Position &camera, const ClientViewport *viewport)
{
	int screenWidth  = graphics.getScreenWidth();
	int screenHeight = graphics.getScreenHeight();

	int offsetx = floor(camera.getX()*zoom);
	int offsety = floor(camera.getY()*zoom);
	int modx = screenWidth;
	int mody = screenHeight;
	int viewleft = viewport->left;
	int viewright = viewport->right;
	int viewtop = viewport->top;
	int viewbotm = viewport->bottom;

	// flicker offsets
	int z, zdim, zbright;
	
	//GameX.FillScreen(ColorX(0,0,0));
	
	if(!enableStars)
		return;
	
	//for drawing the group of points all at once
	glPointSize(1.0f);
	glBegin(GL_POINTS);	
		for (int i = 0; i < numSmallStars; i++)
		{
			int px = (smallStarsX[i]-offsetx + (modx<<7))%modx;
			int py = (smallStarsY[i]-offsety + (mody<<7))%mody;
			if (px >= viewleft && px <= viewright && py >= viewtop && py <= viewbotm) {
				z = randInt(-20, 20) + smallStarsB[i];
				//GameX.DrawPoint(ColorX(z,z,z), px, py);
				/*graphics.drawPoint(
					Color(z, z, z), 
					px, 
					py);*/
				graphics.setColor(Color(z, z, z));
				graphics.drawVertex(
					px, 
					py);
			}
		}

		for (int i = 0; i < numLargeStars; i++)
		{
			zdim = randInt(-20, 20) + largeStarsB[i];
			zbright = randInt(215,255);

			//ColorX c(zdim, zdim, zdim);
			Color c(zdim, zdim, zdim);
			graphics.setColor(c);

			int px1 = ((largeStarsX[i] - 1) - offsetx + (modx<<7)) % modx;
			int py1 = ((largeStarsY[i] - 1) - offsety + (mody<<7)) % mody;
			int px2 = ((largeStarsX[i] + 1) - offsetx + (modx<<7)) % modx;
			int py2 = ((largeStarsY[i] + 1) - offsety + (mody<<7)) % mody;

			int px = (largeStarsX[i]-offsetx + (modx<<7))%modx;
			int py = (largeStarsY[i]-offsety + (mody<<7))%mody;
			// Dimmer surrounding points
			if (px1 >= viewleft && px1 < viewright) {
				if (py1 >= viewtop && py1 <= viewbotm) 
				{
					//GameX.DrawPoint(c, px1, py1);
					//graphics.drawPoint(c, px1, py1);
					graphics.drawVertex(px1, py1);
				}
				if (py2 >= viewtop && py2 <= viewbotm) 
				{
					//GameX.DrawPoint(c, px1, py2);
					//graphics.drawPoint(c, px1, py2);
					graphics.drawVertex(px1, py2);
				}
			}
			if (px2 >= viewleft && px2 < viewright) {
				if (py1 >= viewtop && py1 <= viewbotm) 
				{
					//GameX.DrawPoint(c, px2, py1);
					//graphics.drawPoint(c, px2, py1);
					graphics.drawVertex(px2, py1);
				}
				if (py2 >= viewtop && py2 <= viewbotm) 
				{
					//GameX.DrawPoint(c, px2, py2);
					//graphics.drawPoint(c, px2, py2);
					graphics.drawVertex(px2, py2);
				}
			}

			// "Core" bright point
			if (px >= viewleft && px <= viewright && py >= viewtop && py <= viewbotm) {
				//GameX.DrawPoint(ColorX(zbright,zbright,zbright), px, py);
				//graphics.drawPoint(Color(zbright, zbright, zbright), px, py);
				graphics.setColor(Color(zbright, zbright, zbright));
				graphics.drawVertex(px, py);
			}
		}
	glEnd();	//GL_POINTS
}

#endif

