#ifndef SHIPTYPE_HPP
#define SHIPTYPE_HPP

#include <string>
#include "weapons.hpp"

class Position;
class Weapon;

class ShipType
{
public:
	ShipType(
		const std::string &sprite,
		float drawScale,
		float boundingRadius,
		float turnSpeed,
		float thrustStrength,
		float brakeStrength,
		float strafeStrength,
		float maxSpeed,
		float hitpoints,
		bool onHumanTeam,
		bool mobile,
		bool weapons,
		bool afterBurners, 
		bool playerShipType);
	virtual ~ShipType() {};

	const char *getSprite(void)   { return sprite.c_str(); }
	float getDrawScale(void)      { return drawScale;      }
	float getBoundingRadius(void) { return boundingRadius; }
	float getTurnSpeed(void)      { return turnSpeed;      }
	float getThrustStrength(void) { return thrustStrength; }
	float getBrakeStrength(void)  { return brakeStrength;  }
	float getStrafeStrength(void) { return strafeStrength; }
	float getMaxSpeed(void)       { return maxSpeed;       }
	bool  isOnHumanTeam(void)     { return onHumanTeam;    }
	float getHitpoints(void)      { return hitpoints;      }
	bool  isMobile(void)          { return mobile;         }
	bool  hasWeapons(void)        { return weapons;        }
	bool  hasAfterburners(void)   { return afterBurners;   }
	bool  isPlayerShipType(void)  { return playerShipType; }
	void  giveDefaultWeapons(Player *plr);
	void  useBossMounts(bool useBossMounts)
	{
		usesBossMounts = useBossMounts;
	}
	
	const Position *getWeaponMounts(unsigned numMounts);

    struct WeaponAmmoInfo
    {
        WeaponAmmoInfo()
        {
            weapon     = 0;
            ammoType   = ammo_unlimited;
            ammoAmount = 0;
        }

        Weapon *weapon;
        AmmoType ammoType;
        int ammoAmount;
    };

	void setPrimaryWeapon(const WeaponAmmoInfo &primaryInfo);
	void addSecondaryWeapon(const WeaponAmmoInfo &secondaryInfo);

	WeaponAmmoInfo &getPrimaryWeaponInfo() 
	{
		return primaryWeapon;
	}

	typedef std::vector<WeaponAmmoInfo> SecondaryWeaponPool;
	SecondaryWeaponPool &getSecondaryWeapons()
	{
		return secondaryWeapons;
	}

private:
	const Position *getDefaultMounts(unsigned numMounts);
	const Position *getBossMounts(unsigned numMounts);

	std::string sprite;

	float drawScale;
	float boundingRadius;
	float turnSpeed;
	float thrustStrength;
	float brakeStrength;
	float strafeStrength;
	float maxSpeed;
	float hitpoints;

	bool onHumanTeam;
	bool mobile;
	bool weapons;
	bool afterBurners;
	bool usesBossMounts;
	bool playerShipType;

    WeaponAmmoInfo primaryWeapon;

	SecondaryWeaponPool secondaryWeapons;
};

ShipType *getPlayerShipType(void);
ShipType *getBotShipType(void);
ShipType *shipTypeByName(const std::string &name);

#endif	//SHIPTYPE_HPP