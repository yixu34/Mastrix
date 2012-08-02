#include "mastrix.hpp"

//#define SCREEN_WIDTH 1024
//#define SCREEN_HEIGHT 768

//TODO:  get the viewports to change resolutions correctly
static int screenWidth  = 1024;//graphics.getScreenWidth();
static int screenHeight = 768;//graphics.getScreenHeight();

const ClientViewport viewport_full  (0, -1, 0, -1, screenWidth, screenHeight);

static const int viewportMargin = 1;



//initialize the viewports to default resolution of 1024x768
static ClientViewport viewport_top(
							 0, 
							-1, 
							 0, 
							 screenHeight / 2 - viewportMargin, 
						 	 screenWidth, 
							 screenHeight);

static ClientViewport viewport_bottom(
							 0, 
							-1, 
							 screenHeight / 2 + viewportMargin, 
							-1, 
							 screenWidth, 
							 screenHeight);

static ClientViewport viewport_left(
							 0, 
							 screenWidth / 2 - viewportMargin, 
							 0, 
							-1, 
							 screenWidth, 
							 screenHeight);
 
static ClientViewport viewport_right(
							 screenWidth / 2 +viewportMargin, 
							-1, 
							 0, 
							-1, 
							 screenWidth, 
							 screenHeight);

static ClientViewport viewport_topleft(
							 0, 
							 screenWidth / 2 - viewportMargin, 
							 0, 
							 screenHeight / 2 - viewportMargin, 
							 screenWidth, 
							 screenHeight);

static ClientViewport viewport_topright(
							 screenWidth / 2 + viewportMargin, 
							-1, 
							 0, 
							 screenHeight / 2 - viewportMargin, 
							 screenWidth, 
							 screenHeight);

static ClientViewport viewport_bottomleft(
							 0, 
							 screenWidth / 2 - viewportMargin, 
							 screenHeight / 2 + viewportMargin, 
							-1, 
							 screenWidth, 
							 screenHeight);

static ClientViewport viewport_bottomright(
							 screenWidth / 2 + viewportMargin, 
							 -1, 
							 screenHeight / 2 + viewportMargin, 
							 -1, 
							 screenWidth, 
							 screenHeight);

ClientViewport::ClientViewport(
	float left, 
	float right, 
	float top, 
	float bottom, 
	int   width, 
	int   height)
{
	reset(
		left, 
		right, 
		top, 
		bottom, 
		width, 
		height);
}

void ClientViewport::setClip(void) const
{
	//GameX.SetView(left, top, right+1, bottom+1);
	glViewport(0, 0, right+1,bottom+1);
}

void ClientViewport::reset(
	float left, 
	float right, 
	float top, 
	float bottom, 
	int   width, 
	int   height)
{
	if(left < 0)   left   += width;
	if(right < 0)  right  += width;
	if(top < 0)    top    += height;
	if(bottom < 0) bottom += height;
	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;
	this->center_x = (this->left+right)/2;
	this->center_y = (top+bottom)/2;
}

void updateViewports(bool vertical) { assignViewports(); }
ClientConsoleVar<bool> splitVertically ("split_vertically", true, updateViewports);
ClientConsoleVar<bool> viewAdvantage  ("view_advantage", false, updateViewports);

void Client::resetViewports(int width, int height)
{
	viewport_top.reset(
		viewport_top.left, 
		viewport_top.right, 
		viewport_top.top, 
		viewport_top.bottom, 
		width, 
		height);

	viewport_left.reset(
		viewport_left.left, 
		viewport_left.right, 
		viewport_left.top, 
		viewport_left.bottom,
		width, 
		height);

	viewport_right.reset(
		viewport_right.left, 
		viewport_right.right,
		viewport_right.top,
		viewport_right.bottom,
		width,
		height);

	viewport_bottom.reset(
		viewport_bottom.left,
		viewport_bottom.right, 
		viewport_bottom.top,
		viewport_bottom.bottom,
		width, 
		height);

	viewport_topleft.reset(
		viewport_topleft.left, 
		viewport_topleft.right,
		viewport_topleft.top, 
		viewport_topleft.bottom,
		width,
		height);

	viewport_topright.reset(
		viewport_topright.left,
		viewport_topright.right,
		viewport_topright.top, 
		viewport_topright.bottom, 
		width,
		height);

	viewport_bottomleft.reset(
		viewport_bottomleft.left,
		viewport_bottomleft.right, 
		viewport_bottomleft.top, 
		viewport_bottomleft.bottom,
		width,
		height);

	viewport_bottomright.reset(
		viewport_bottomright.left, 
		viewport_bottomright.right,
		viewport_bottomright.top, 
		viewport_bottomright.bottom,
		width,
		height);
}

void assignViewports(void)
{
	ClientPool::iterator ii = clients.begin();
	
	switch(clients.size())
	{
		case 1:
			ii->second->setViewport(&viewport_full);
			break;
		case 2:
			if(splitVertically) {
				ii->second->setViewport(&viewport_left); ii++;
				ii->second->setViewport(&viewport_right); ii++;
			} else {
				ii->second->setViewport(&viewport_top); ii++;
				ii->second->setViewport(&viewport_bottom); ii++;
			}
			break;
		case 3:
			ii->second->setViewport(&viewport_topleft); ii++;
			ii->second->setViewport(&viewport_topright); ii++;
			if(viewAdvantage) {
				ii->second->setViewport(&viewport_bottom); ii++;
			} else {
				ii->second->setViewport(&viewport_bottomleft); ii++;
			}
			break;
		case 4:
			ii->second->setViewport(&viewport_topleft); ii++;
			ii->second->setViewport(&viewport_topright); ii++;
			ii->second->setViewport(&viewport_bottomleft); ii++;
			ii->second->setViewport(&viewport_bottomright); ii++;
			break;
	}
}
