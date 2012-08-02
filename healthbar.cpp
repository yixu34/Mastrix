#include "mastrix.hpp"

Healthmeter::Healthmeter()
{
	length = 1.0;
	tempLength = 1.0;
	img = images.getImage("images/Sphere1.png");
}

void Healthmeter::setView(int l, int r, int t, int b)
{
	x = l + 36;
	y = t + 41;
}

void Healthmeter::draw()
{
	/*
	Color bgcol = Color(100,100,100,200);
	glEnable(GL_POINT_SMOOTH);
		graphics.drawPoint(bgcol, x, y, 64);
	glDisable(GL_POINT_SMOOTH);
	*/

	if (tempLength > length) {
		tempLength -= getDt()*0.5;
		updateColor();
	}
	else if(tempLength != length) {
		tempLength = length;
		updateColor();
	}

	glColor4ub(100, 100, 100, 200);
	graphics.drawColoredImage(img, x, y, 0, 1);

	 // update: by scaling .4 by health amount, 
	 // the circle gets smaller as you die. 
	glColor3ub(r,g,b);
	if(length>0)
		graphics.drawColoredImage(img, x, y, 0, tempLength);
	else { tempLength = 1.0;}
	
	// old thing
	//glColor3f(0,0,0);
	//graphics.drawColoredImage(img, x, y, 0, 0.4 - 0.4*(currlength/maxlength));
}

void Healthmeter::draw(float length)
{
	this->length = length;
	updateColor();
	draw();
}

void Healthmeter::updateColor(void)
{
	b = 0;
	if(tempLength > 0.5)
		g = 255;
	else
		g = 255 * (tempLength*2);
	if(tempLength > 0.5)
		r = 255 * (1-tempLength)*2;
	else
		r = 255;
}