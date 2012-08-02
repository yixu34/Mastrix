#include "mastrix.hpp"

WeaponInfo::WeaponInfo() {
	currWeapon = 0;
}

void WeaponInfo::setView(int l, int r, int t, int b)
{
	x = r - (48*weapons.size()) - 4;
	xmax = r - 4;
	y = b - 52;
}

void WeaponInfo::draw()
{
	/*
	Color backColor(200,200,200,80);
	graphics.drawRect(
		backColor,
		x, y,
		xmax,
		y + 48);
		*/

	Image *currImage;
	for (unsigned i = 0; i < weapons.size(); i++) {
		if (i == currWeapon) {
			Color backColor(0,128,32,100);
			graphics.drawRect(
				backColor,
				x + i*48, y,
				x + (i+1)*48,
				y + 48);
		}

		currImage = images.getImage(weapons[i].name.c_str());
		if (currImage->getHeight() <= 48) {
			graphics.drawImage(currImage,x+24 + i*48,y+24);
		}
		graphics.drawText(retprintf("%i", weapons[i].ammo).c_str(), x+24+i*48, y+48);
	}

}

void WeaponInfo::updateWeapons(RecvMessage &msg)
{
	int total = msg.getInt();
	currWeapon = msg.getInt();
	
	weapons.resize(total);
	
	for (int ii = 0; ii < total; ii++) {
		weapons[ii].ammo = msg.getInt();
		weapons[ii].name = msg.getString();
	}
}

