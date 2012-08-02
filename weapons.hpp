#ifndef WEAPONS_HPP
#define WEAPONS_HPP

class Weapon;

enum AmmoType
{
	ammo_unlimited,
	ammo_missile,
	ammo_homing_missile,
	ammo_fireball,
};

class WeaponPool
{
public:
	WeaponPool();
	~WeaponPool();
	
	void setOwner(Player *plr) { this->owner = plr; }
	void addPrimary(Weapon *w);
	void addWeapon(Weapon *w);
	void selectBestWeapon();
	void selectNext();
	void selectPrevious();
	void updateWeapons(void);
	void upgradeLaser(void);
	void upgradeLaserLevel();
	void upgradeLaserNum();
	void checkDepletion(void);
	//void setClient(int c) {client = c;}
	
	bool giveAmmo(AmmoType type, int amt); // Returns true iff some weapon uses this ammo type
	
	Weapon *primary(void);
	Weapon *secondary(void);
	
protected:
	void  selectWeapon(unsigned index);
	
	Player *owner;
	int selection;
	Weapon *primaryWeapon;
	typedef std::vector<Weapon*> WeaponList;
	WeaponList secondaryWeapons;
};


extern ServerConsoleVar<float> shotSpeed;
extern ServerConsoleVar<float> fireballShotSpeed;
extern ServerConsoleVar<float> missileSpeed;
//extern ServerConsoleVar<float> clusterBombSpeed;
extern ServerConsoleVar<int>   numExplosions;

class Weapon
{
public:
	Weapon() {};
    virtual ~Weapon() {};

	void setOwner(Player *owner) { this->owner = owner; }
	virtual void startShooting() = 0;
	virtual void stopShooting() = 0;
	virtual void timepass() = 0;
	
	virtual int getAmmo() = 0;
	virtual void giveAmmo(int amt) = 0;
	virtual AmmoType getAmmoType() { return ammo_unlimited; }
	
	virtual void select() {}
	virtual void deselect() {}
	
	virtual void upgradeIfLaser(void) {}

	virtual void upgradeLevelIfLaser() {}
	virtual void upgradeNumIfLaser() {}

	virtual std::string getHUDicon() = 0;

	virtual float getSpeed() = 0;
	
	// Autoselect priority for this weapon. Highest value gets selected automatically.
	virtual int priority() { return 1; }
	virtual Player *getOwner() {return owner;}

    virtual Weapon *clone() const = 0;
	
protected:
	Player *owner;
};

class BurstWeapon :public Weapon
{
public:
	BurstWeapon();
	
	virtual void startShooting();
	virtual void stopShooting();
	virtual void timepass();
	virtual void deselect();
	virtual void fireBurst() = 0;
	virtual float getCooldown() = 0;
	virtual std::string getHUDicon() = 0;
	
	int getAmmo(void) { return ammo; }
	void giveAmmo(int amt) { ammo += amt; }
	
protected:
	int ammo;
	bool active;
	float nextShotTime;
};

class ProjectileWeapon :public BurstWeapon
{
public:
	ProjectileWeapon() { fireOrder = 0; }
	void fireBurst();
	virtual void createProjectile(Position *pos) = 0;
	virtual int numProjectiles(void) { return 1; }
	virtual std::string getHUDicon() = 0;
	virtual bool linkedFire(void) { return true; }
	virtual void doSound(void) {}
	
protected:
	int fireOrder;
};

class RedPeaShooter :public ProjectileWeapon
{
public:
	RedPeaShooter(int numShots, int level, float cooldown, float baseSpeed)
		{this->numShots=numShots; this->level=level; this->cooldown=cooldown; this->speed=baseSpeed;}
	void createProjectile(Position *pos);
	virtual float getCooldown();
	std::string getHUDicon();
	float getSpeed();
	int numProjectiles(void);
	void upgradeIfLaser(void);
	virtual void upgradeLevelIfLaser();
	virtual void upgradeNumIfLaser();
	void doSound(void);

    virtual RedPeaShooter *clone() const
    { 
        return new RedPeaShooter(*this); 
    }

protected:
	float cooldown, speed;
	int numShots;
	int level;
};


extern ServerConsoleVar<float> missileSpeed;

class MissileLauncher :public ProjectileWeapon
{
public:
	MissileLauncher() {}
	void createProjectile(Position *pos);
	virtual float getCooldown();
	std::string getHUDicon();
	float getSpeed();
	int numProjectiles(void) { return 2; }
	bool linkedFire(void) { return false; }
	AmmoType getAmmoType() { return ammo_missile; }

    virtual MissileLauncher *clone() const
    {
        return new MissileLauncher(*this);
    }
};

class HomingMissileLauncher :public ProjectileWeapon
{
public:
	HomingMissileLauncher() {}
	void createProjectile(Position *pos);
	virtual float getCooldown();
	std::string getHUDicon();
	float getSpeed();
	int numProjectiles(void) { return 2; }
	bool linkedFire(void) { return false; }
	AmmoType getAmmoType() { return ammo_homing_missile; }

    virtual HomingMissileLauncher *clone() const
    {
        return new HomingMissileLauncher(*this);
    }
};

class SlowMissileLauncher :public MissileLauncher
{
public:
	float getCooldown() { return 2.5; }

    virtual SlowMissileLauncher *clone() const
    {
        return new SlowMissileLauncher(*this);
    }
};

class SlowHomingMissileLauncher : public HomingMissileLauncher
{
	float getCooldown()	{ return 2.5; }

	virtual SlowHomingMissileLauncher *clone() const
	{
		return new SlowHomingMissileLauncher(*this);
	}
};

class FireballLauncher :public ProjectileWeapon
{
public:
	FireballLauncher() {}
	void createProjectile(Position *pos);
	float getCooldown();
	float getSpeed();
	std::string getHUDicon();
	AmmoType getAmmoType() { return ammo_fireball; }

    virtual FireballLauncher *clone() const
    {
        return new FireballLauncher(*this);
    }
};

//class ClusterBombLauncher: public ProjectileWeapon
//{
//public:
//    ClusterBombLauncher() {}
//
//	void  createProjectile(Position *pos);
//	float getCooldown();
//	float getSpeed();
//    std::string getHUDicon();
//};

#endif
