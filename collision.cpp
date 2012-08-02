#include "mastrix.hpp"


bool Server::checkCollisionBetween(CollisionData *ap, CollisionData *bp)
{
	CollisionData a=*ap, b=*bp;
	Entity *e1 = entities[ a.entid ],
		   *e2 = entities[ b.entid ];

	//prevent collisions between teammates
	// FIXME: This is absolutely not the right way to do this
	/*
	if (e1->isPlayer() && e2->isPlayer())
	{
		Player *player1 = (Player *)e1;
		Player *player2 = (Player *)e2;

		if (player1->getTeam() == player2->getTeam())
			return false;
	}
	*/
	
	if(!a.rectangular && !b.rectangular) // Circle-circle
	{
		float dx = a.x - b.x;
		float dy = a.y - b.y;
		float radius = a.radius + b.radius;
		float distsq = dx*dx + dy*dy;
		if(distsq > radius*radius)
			return false;
	}
	else if(a.rectangular && b.rectangular) // Rectangle-rectangle
	{
		// Quick test: if sum of radii is less than the distance, no collision is possible.
		bool intersect = false;
		float radius = a.radius + b.radius;
		float x = a.x - b.x,
			    y = a.y - b.y;
		if(x*x + y*y > radius)
			return false;
		// Accurate test: R1 intersects R2 iff one of R1's vertices is inside R2 or one of
		// R2's vertices is inside R1.
		CollisionData *r1 = &a,
		              *r2 = &b;
		for(int iter=0; iter<=1; iter++)
		{
			// Test whether r2 has any points inside r1.
			// Transform r2 to a fixed position relative to r1
			float x = r2->x - r1->x,
				  y = r2->y - r1->y;
			float r1_angle = r1->rotation;
			// Now, translate this according to the rectangle's rotation
			x = x* cos(r1_angle) + y*-sin(r1_angle);
			y = x*-sin(r1_angle) + y*cos(r1_angle);
			float rotation = r2->rotation - r1->rotation;
			
			// Calculate position of the four corners
			float dx = r2->width*cos(rotation) - r2->height*sin(rotation),
				    dy = r2->width*sin(rotation) - r2->height*cos(rotation);
			float xcoords[4] = { x+dx, x+dx, x-dx, x-dx };
			float ycoords[4] = { y+dy, y-dy, y+dy, y-dy };
			
			// Now test for intersection
			for(int kk=0; kk<4; kk++)
			{
				if(abs(xcoords[kk]) < r1->width/2 && abs(ycoords[kk]) < r2->height/2) {
					return true;
				}
			}
			
			// Now swap the two rectangles
			r1=&b;
			r2=&a;
		}
		return false;
	}
	else // Rectangle-circle
	{
		CollisionData *rect, *circ;
		if(a.rectangular) {
			rect = &a;
			circ = &b;
		} else {
			rect = &b;
			circ = &a;
		}
		// Position of the circle relative to the rectangle
		float dx = circ->x - rect->x;
		float dy = circ->y - rect->y;
		// Now, translate this according to the rectangle's rotation
		float dist = sqrt(dx*dx + dy*dy);
		float rotation = rect->rotation;
		float heading = atan2(dy, dx) + rotation;
		dx = abs(dist*cos(heading));
		dy = abs(dist*sin(heading));
		// Next, 'flatten' this together - remove the rectangle's width and height.
		if(dx < rect->width/2)  dx = 0;
		else                    dx -= rect->width/2;
		if(dy < rect->height/2) dy = 0;
		else                    dy -= rect->height/2;
		if(dx*dx + dy*dy > circ->radius*circ->radius)
			return false;
	}
	if(!e1->shouldDelete() && !e2->shouldDelete())
		return true;
	else
		return false;
}

struct CollisionDataNode {
	CollisionData *data;
	CollisionDataNode *next;
};
template<class T> class FreeingAllocator
{
public:
	FreeingAllocator() {
		index = 0;
		pagesize = 8176/sizeof(T);
		currentPool = new T[pagesize];
	}
	~FreeingAllocator() {
		delete[] currentPool;
		for(unsigned ii=0; ii<usedPools.size(); ii++)
			delete[] usedPools[ii];
	}
	T *allocate(void) {
		if(index >= pagesize) {
			usedPools.push_back(currentPool);
			currentPool = new T[pagesize];
			index = 0;
		}
		return &currentPool[index++];
	}
	int pagesize;
	int index;
	T *currentPool;
	std::vector<T*> usedPools;
};

void Server::getCollisionData(Entity *ent, CollisionData *dat)
{
	CollisionData addition = {
		ent->getEntId(), ent->getRadius(),
		ent->getPosition().getX(), ent->getPosition().getY(),
		false, 0, 0, 0
		};
	if(ent->isRectangular()) {
		addition.rectangular = true;
		ent->getDimensions(&addition.width, &addition.height);
		addition.rotation = ent->getPosition().getR();
		addition.radius = sqrt(addition.width*addition.width + addition.height*addition.height);
	}
	*dat = addition;
}

void Server::checkCollisions(void)
{
	std::vector<CollisionData> ents;
	
	for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii++)
	{
		Entity *ent = ii->second;
		if(ent->getCollisionEffect() & collide_skip)
			continue;
		if(ent->shouldDelete())
			continue;
		CollisionData addition;
		getCollisionData(ent, &addition);
		ents.push_back(addition);
	}
	
	unsigned partitionsX = 5,
	         partitionsY = 5;
	// While the ents-per-region load factor is more than 1
	while(partitionsX*partitionsY < ents.size()) {
			// If region is wider than tall
		if((rightBorder-leftBorder)/partitionsX > (bottomBorder-topBorder)/partitionsY)
			partitionsX *= 2;
		else
			partitionsY *= 2;
	}
	float regionWidth = (rightBorder-leftBorder)/partitionsX,
	      regionHeight = (bottomBorder-topBorder)/partitionsY;
	std::vector< std::vector<CollisionDataNode*> > regions(partitionsY);
	for(unsigned ii=0; ii<regions.size(); ii++) {
		regions[ii].resize(partitionsX);
		for(unsigned jj=0; jj<partitionsX; jj++)
			regions[ii][jj] = NULL;
	}
	FreeingAllocator<CollisionDataNode> alloc;
	
	// Partition entities into regions
	for(unsigned ii=0; ii<ents.size(); ii++)
	{
		int region_min_x = (int)((ents[ii].x-ents[ii].radius-leftBorder)/regionWidth);
		int region_max_x = (int)ceil((ents[ii].x+ents[ii].radius-leftBorder)/regionWidth);
		int region_min_y = (int)((ents[ii].y-ents[ii].radius-topBorder)/regionHeight);
		int region_max_y = (int)ceil((ents[ii].y+ents[ii].radius-topBorder)/regionHeight);
		if(region_min_x<0) region_min_x = 0;
		if(region_max_x>=(int)partitionsX) region_max_x = partitionsX;
		if(region_min_y<0) region_min_y = 0;
		if(region_max_y>=(int)partitionsY) region_max_y = partitionsY;
		for(int jj=region_min_y; jj<region_max_y; jj++)
		for(int kk=region_min_x; kk<region_max_x; kk++)
		{
			CollisionDataNode *newNode = alloc.allocate();
			newNode->data = &ents[ii];
			newNode->next = regions[jj][kk];
			regions[jj][kk] = newNode;
		}
	}
	
	std::vector<CollisionData*> region_ents;
	for(unsigned ii=0; ii<partitionsY; ii++)
	for(unsigned jj=0; jj<partitionsX; jj++)
	{
		if(regions[ii][jj]==NULL) continue;
		CollisionDataNode *pos = regions[ii][jj];
		while(pos) {
			region_ents.push_back(pos->data);
			pos = pos->next;
		}
		for(unsigned a=0;   a<region_ents.size(); a++)
		for(unsigned b=a+1; b<region_ents.size(); b++)
		{
			if(checkCollisionBetween(region_ents[a], region_ents[b]))
				collide(entities[region_ents[a]->entid], entities[region_ents[b]->entid]);
		}
		
		region_ents.clear();
	}
}

bool Server::testSphereCollision(Entity *ent1, Entity *ent2)
{
	Position &ent1Pos = ent1->getPosition();
	Position &ent2Pos = ent2->getPosition();

	float radiusSum = ent1->getRadius() + ent2->getRadius();

	float relX = ent2Pos.getX() - ent1Pos.getX();
	float relY = ent2Pos.getY() - ent1Pos.getY();

	float pp = (relX * relX) +
			   (relY * relY);

	if (pp <= radiusSum*radiusSum)
		return true;
	else return false;
	// TODO: Handle really fast-moving objects.
}


void Server::inflict_gravity(void)
{
	
	for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii++)
	{
		Entity *e1 = ii->second;
		if(!e1->hasGravity()) continue;
		
		for(Entpool::iterator jj=entities.begin(); jj!=entities.end(); jj++)
		{
			Entity *e2 = jj->second;
			if(e1==e2) continue;
			
			if ((!(e1->getCollisionEffect() & collide_no_gravity)) && e2->hasGravity())
				e1->gravity_affect(e2);
			if ((!(e2->getCollisionEffect() & collide_no_gravity)) && e1->hasGravity())
				e2->gravity_affect(e1);
		}
	}
}

void Server::semiElasticBounce(Entity *a, Entity *b, float elasticity)
{
	/*
	 * 1. Collide inelastically.
	 * 2. Calculate the kinetic energy of the two ships, in the frame of reference
	 *    of their combined (inelastically-collided) velocity.
	 * 3. Calculate the impulse that should be applied for that KE
	 */
	
	float mass_a = a->getMass(),
	      mass_b = b->getMass();
	
	if(a->getCollisionEffect()&collide_immobile)
		mass_a = 100000;
	if(b->getCollisionEffect()&collide_immobile)
		mass_b = 100000;
	
	Position *pos_a = &a->getPosition(),
	         *pos_b = &b->getPosition();
	Vector2D v_a = pos_a->velocityVector(),
	         v_b = pos_b->velocityVector();
	
	// Find the normal vector
	Vector2D norm;
	
	if(a->isRectangular() || b->isRectangular())
	{
		Vector2D rect_pos, circ_pos;
		float width, height, angle;
		if(a->isRectangular()) {
			rect_pos = pos_a->positionVector();
			circ_pos = pos_b->positionVector();
			a->getDimensions(&width, &height);
			angle = pos_a->getR();
		} else {
			rect_pos = pos_b->positionVector();
			circ_pos = pos_a->positionVector();
			b->getDimensions(&width, &height);
			angle = pos_b->getR();
		}
		Vector2D off1 = Vector2D(width/2, -height/2),
		         off2 = Vector2D(-width/2, -height/2);
		off1.rotate(angle); off2.rotate(angle);
		
		/*
		 * 2<--1---1
		 * |       ^
		 * 2       4
		 * v       |
		 * 3---3-->4
		 */
		Vector2D corner1 = rect_pos+off1,
		         corner2 = rect_pos+off2,
		         corner3 = rect_pos-off1,
		         corner4 = rect_pos-off2;
		Vector2D side1 = corner2-corner1,
		         side2 = corner3-corner2,
		         side3 = corner4-corner3,
		         side4 = corner1-corner4;
		Vector2D diff = circ_pos-corner1;
		if(side1.isOnRight(circ_pos-corner1)) {
			norm = side4;
		} else if(side2.isOnRight(circ_pos-corner2)) {
			norm = side1;
		} else if(side3.isOnRight(circ_pos-corner3)) {
			norm = side2;
		} else if(side4.isOnRight(circ_pos-corner4)) {
			norm = side3;
		} else {
			norm = Vector2D(pos_a->getX()-pos_b->getX(), pos_a->getY()-pos_b->getY());
		}
		if(a->isRectangular()) norm = -norm;
	}
	else
	{
		norm = Vector2D(pos_a->getX()-pos_b->getX(), pos_a->getY()-pos_b->getY());
	}
	
	norm.normalize();
	
	// Inelastic collision: Velocities are the same, and are equal to the combined
	// momentum divided by the combined mass.
	Vector2D ie_v = (v_a*mass_a + v_b*mass_b) / (mass_a*mass_b);
	
	// Kinetic energy in this frame of reference is 1/2*m_a*v_a^2 + 1/2*m_b*v_b^2
	float KE = (v_a.x-ie_v.x)*(v_a.x-ie_v.x) + (v_a.y-ie_v.y)*(v_a.y-ie_v.y) +
	           (v_b.x-ie_v.x)*(v_b.x-ie_v.x) + (v_b.y-ie_v.y)*(v_b.y-ie_v.y);
	KE *= elasticity;
	
	// KE = 1/2 ma*va^2 + 1/2 mb*vb^2
	//   p = ma*va = mb*vb
	// 2*KE = p*va + p*vb
	//   va = p/ma, vb = p/mb
	// 2*KE = p^2/ma + p^2/mb
	// 2*KE = p^2(1/ma + 1/mb)
	// 2*KE / (1/ma + 1/mb) = p^2
	// 2*KE*ma*mb / (ma+mb) = p^2
	// p = sqrt(2*KE*ma*mb / (ma+mb))
	float p = sqrt( 2*KE*mass_a*mass_b / (mass_a+mass_b) );
	Vector2D p_vec = norm*p;
	
	// Set velocities
	if(!(a->getCollisionEffect()&collide_immobile)) {
		pos_a->setX_vel( ie_v.x + p_vec.x/mass_a );
		pos_a->setY_vel( ie_v.y + p_vec.y/mass_a );
	}
	if(!(b->getCollisionEffect()&collide_immobile)) {
		pos_b->setX_vel( ie_v.x - p_vec.x/mass_b );
		pos_b->setY_vel( ie_v.y - p_vec.y/mass_b );
	}
	
	Position *tweakPos;
	float tweakFactor;
	if(a->getCollisionEffect()&collide_immobile && b->getCollisionEffect()&collide_immobile)
		return;
	else if(a->getCollisionEffect()&collide_immobile) {
		tweakFactor = -1;
		tweakPos = pos_b;
	} else if(b->getCollisionEffect()&collide_immobile) {
		tweakFactor = 1;
		tweakPos = pos_a;
	} else if(pos_a->getVelocityMagnitude() > pos_b->getVelocityMagnitude()) {
		tweakFactor = 1;
		tweakPos = pos_a;
	} else {
		tweakFactor = -1;
		tweakPos = pos_b;
	}
	
	float tweakX = norm.x*3*tweakFactor,
	      tweakY = norm.y*3*tweakFactor;
	CollisionData collide_a, collide_b;
	getCollisionData(a, &collide_a);
	getCollisionData(b, &collide_b);
	
	// Un-embed entities by moving in small steps
	while(checkCollisionBetween(&collide_a, &collide_b))
	{
		tweakPos->setX( tweakPos->getX() + tweakX );
		tweakPos->setY( tweakPos->getY() + tweakY );
		getCollisionData(a, &collide_a);
		getCollisionData(b, &collide_b);
	}
}

ServerConsoleVar<float> elasticity("elasticity", 0.2);

void Server::collide(Entity *a, Entity *b)
{
	int mode_a = a->getCollisionEffect(),
	    mode_b = b->getCollisionEffect();
	if( (mode_a|mode_b) & collide_skip )
		return;
	
	if( !a->canCollideWith(b) ) return;
	if( !b->canCollideWith(a) ) return;
	
	if( mode_a & collide_custom) {
		a->collideWith(b);
		return;
	}
	if( mode_b & collide_custom) {
		b->collideWith(a);
		return;
	}
	
	
	if( mode_a & collide_damage )
		b->takeDamage( a->getCollisionDamage(), a->getDamageSource() );
	if( mode_b & collide_damage )
		a->takeDamage( b->getCollisionDamage(), b->getDamageSource() );
	
	if( (mode_a|mode_b)&collide_bounce )
		semiElasticBounce(a, b, elasticity);
	
	if(mode_a & collide_destroy)
		a->die(b->getDamageSource());
	if(mode_b & collide_destroy)
		b->die(a->getDamageSource());
}
