#include "mastrix.hpp"

ServerConsoleVar<float> shotdist("shotdist", 40);
ServerConsoleVar<bool>  shotsGetShipVel("shots_get_ship_vel", false);

ServerConsoleVar<float> fireballShotSpeed("fireball_shotspeed", 1500);
ServerConsoleVar<float> missileSpeed("missile_speed", 1300);
//ServerConsoleVar<float> clusterBombSpeed("clusterBomb_speed", 1000);
ServerConsoleVar<int>   numExplosions("num_explosions", 3);


WeaponPool::WeaponPool()
{
	selection     = 0;
    owner         = 0;
    primaryWeapon = 0;
}
WeaponPool::~WeaponPool()
{
    delete primaryWeapon;
	for(unsigned ii=0; ii<secondaryWeapons.size(); ii++)
		delete secondaryWeapons[ii];
}

void WeaponPool::addPrimary(Weapon *w)
{
	w->setOwner(owner);
	primaryWeapon = w;
}
void WeaponPool::addWeapon(Weapon *w)
{
	w->setOwner(owner);
	secondaryWeapons.push_back(w);
	updateWeapons();
}

void WeaponPool::updateWeapons(void) {
	if(!owner->isHuman()) return;
	
	SendMessage msg(message_weapons);
		msg.putInt(secondaryWeapons.size());
		msg.putInt(selection);
		for (unsigned ii=0; ii<secondaryWeapons.size(); ii++) {
			if(secondaryWeapons[ii]->getAmmoType() == ammo_unlimited)
				msg.putInt(-1);
			else
				msg.putInt(secondaryWeapons[ii]->getAmmo());
			msg.putString((secondaryWeapons[ii]->getHUDicon()).c_str());
		}
	msg.sendToClient(owner->getClientId());
}

void WeaponPool::selectBestWeapon()
{
	int best_weight = 0, best_index = 0;
	for(unsigned ii=0; ii<secondaryWeapons.size(); ii++) {
		if(secondaryWeapons[ii]->priority() >= best_weight) {
			best_weight = secondaryWeapons[ii]->priority();
			best_index = ii;
		}
	}
	selection = best_index;
}

void WeaponPool::selectNext()
{
	//make sure we don't select something if the weapon pool is empty
	if (secondaryWeapons.empty())
		return;

	if((unsigned)selection+1 >= secondaryWeapons.size())
		selectWeapon(0);
	else
		selectWeapon(selection+1);
}

void WeaponPool::selectPrevious()
{
	//again, an error check if the pool is empty
	if (secondaryWeapons.empty())
		return;

	if(selection==0)
		selectWeapon(secondaryWeapons.size()-1);
	else
		selectWeapon(selection-1);
}

void WeaponPool::checkDepletion(void)
{
	if(secondaryWeapons.size()>0 &&
	   secondaryWeapons[selection]->getAmmo() == 0 &&
	   secondaryWeapons[selection]->getAmmoType() != ammo_unlimited)
	{
		delete secondaryWeapons[selection];
		
		for(unsigned ii=selection; ii<secondaryWeapons.size()-1; ii++)
			secondaryWeapons[ii] = secondaryWeapons[ii+1];
		secondaryWeapons.resize(secondaryWeapons.size()-1);
		selectBestWeapon();
		updateWeapons();
	}
}

Weapon *WeaponPool::primary(void)
{
	return primaryWeapon;
}
Weapon *WeaponPool::secondary(void)
{
	if(secondaryWeapons.size())
		return secondaryWeapons[selection];
	else
		return NULL;
}

void WeaponPool::selectWeapon(unsigned index)
{
	if(selection==index)
		return;
	secondaryWeapons[selection]->deselect();
	secondaryWeapons[index]->select();
	selection = index;
	updateWeapons();
}

void WeaponPool::upgradeLaser(void)
{
	primaryWeapon->upgradeIfLaser();
	for(unsigned ii=0; ii<secondaryWeapons.size(); ii++)
		secondaryWeapons[ii]->upgradeIfLaser();
}

void WeaponPool::upgradeLaserLevel()
{
	primaryWeapon->upgradeLevelIfLaser();
	for (unsigned ii=0; ii<secondaryWeapons.size(); ii++)
		secondaryWeapons[ii]->upgradeIfLaser();
}

void WeaponPool::upgradeLaserNum()
{
	primaryWeapon->upgradeNumIfLaser();
	for (unsigned ii=0; ii<secondaryWeapons.size(); ii++)
		secondaryWeapons[ii]->upgradeIfLaser();
}

bool WeaponPool::giveAmmo(AmmoType type, int amt)
{
	for(unsigned ii=0; ii<secondaryWeapons.size(); ii++) {
        AmmoType currentType = secondaryWeapons[ii]->getAmmoType();

		if(secondaryWeapons[ii]->getAmmoType() == type) {
			secondaryWeapons[ii]->giveAmmo(amt);
			updateWeapons();
			return true;
		}
	}

	if (primaryWeapon->getAmmoType() == type)
	{
		primaryWeapon->giveAmmo(amt);
		return true;
	}

	return false;
}


BurstWeapon::BurstWeapon()
{
	active = false;
	nextShotTime = 0;
	ammo = 0;
}

void BurstWeapon::startShooting()
{
	active = true;
	if(nextShotTime <= getTime()) {
		fireBurst();
		nextShotTime = getTime() + getCooldown();
	}
}

void BurstWeapon::stopShooting()
{
	active = false;
}

void BurstWeapon::timepass()
{
	if(active && nextShotTime <= getTime()) {
		fireBurst();
		nextShotTime = getTime() + getCooldown();
	}
}

void BurstWeapon::deselect()
{
	active = false;
}


void ProjectileWeapon::fireBurst()
{
	if(getAmmoType()!=ammo_unlimited)
	{
		if(ammo==0)
			return;
		ammo--;
		owner->weapons.updateWeapons();
	}
	
	Position ownerPos(owner->getPosition());
	const Position *shotOffsets = owner->type->getWeaponMounts(numProjectiles());
	doSound();
	
	for(unsigned ii=0; ii<numProjectiles(); ii++)
	if(linkedFire() || ii==fireOrder)
	{
		Position shotPosition(ownerPos);

		shotPosition.setX(ownerPos.getX() + shotOffsets[ii].getX()*cos(ownerPos.getR()) + shotOffsets[ii].getY()*sin(ownerPos.getR()));
		shotPosition.setY(ownerPos.getY() - shotOffsets[ii].getX()*sin(ownerPos.getR()) + shotOffsets[ii].getY()*cos(ownerPos.getR()));
		
		shotPosition.setR_vel(0);
		if(!shotsGetShipVel) {
			shotPosition.setX_vel(0);
			shotPosition.setY_vel(0);
		}
		shotPosition.impulse( getSpeed()*cos(shotPosition.getR()), -getSpeed()*sin(shotPosition.getR()) );
		
		createProjectile(&shotPosition);
	}
	fireOrder++;
	if(fireOrder >= numProjectiles())
		fireOrder = 0;
}

void RedPeaShooter::createProjectile(Position *pos)
{
	new RedPea(*pos, owner, level);
}
void RedPeaShooter::doSound(void)
{
	server->playSoundAt("sounds/Laser3.wav", owner->getPosition() );
}
float RedPeaShooter::getCooldown()
{
	return cooldown;
}
float RedPeaShooter::getSpeed()
{
	switch(level)
	{
		default: return speed;
		case 2: return speed * 1.2;
		case 3: return speed * 1.5;
	}
}
int RedPeaShooter::numProjectiles(void)
{
	return numShots;
}
void RedPeaShooter::upgradeIfLaser(void)
{
	if(numShots<2)
		numShots++;
	else if(level<2)
		level++;
	else if(numShots<3)
		numShots++;
	else if(level < 3)
		level++;
}

void RedPeaShooter::upgradeLevelIfLaser()
{
	if (level<3) level++;
}

void RedPeaShooter::upgradeNumIfLaser()
{
	if( numShots<3) numShots++;
}


std::string RedPeaShooter::getHUDicon() {
	return "images/Shot.png";
}


void MissileLauncher::createProjectile(Position *pos)
{
	new Missile(*pos, owner);
}
float MissileLauncher::getCooldown()
{
	return 0.3;
}
float MissileLauncher::getSpeed()
{
	return missileSpeed;
}
std::string MissileLauncher::getHUDicon() {
	return "images/icon-missile.png";
}

void HomingMissileLauncher::createProjectile(Position *pos)
{
	new HomingMissile(*pos, owner);
}
float HomingMissileLauncher::getCooldown()
{
	return 0.5;
}
float HomingMissileLauncher::getSpeed()
{
	return missileSpeed;
}
std::string HomingMissileLauncher::getHUDicon() {
	return "images/icon-homing.png";
}


void FireballLauncher::createProjectile(Position *pos)
{
	new Fireball(*pos, owner);
}
float FireballLauncher::getCooldown()
{
	return 0.7;
}
float FireballLauncher::getSpeed()
{
	return fireballShotSpeed;
}
std::string FireballLauncher::getHUDicon() {
    return "images/fireballicon.png";
}

//
//void ClusterBombLauncher::createProjectile(Position *pos)
//{
//	new ClusterBomb(
//		*pos, 
//		numExplosions, 
//		owner);
//}
//float ClusterBombLauncher::getCooldown()
//{
//	return 0.7;
//}
//float ClusterBombLauncher::getSpeed()
//{
//	return clusterBombSpeed;
//}
//
//std::string ClusterBombLauncher::getHUDicon()
//{
//    return "images/Star1.png";
//}
//
