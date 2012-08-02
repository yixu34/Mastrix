#ifndef MAPMARKER_HPP
#define MAPMARKER_HPP

#include "sv_entity.hpp"
#include <vector>

class SpawnpointMarker :public Entity
{
public:
	SpawnpointMarker(float x, float y, float r, team_type team);
	~SpawnpointMarker();
	int getCollisionEffect(void) { return collide_skip | collide_no_gravity | collide_immobile; }
	float getMass(void) { return 0; }
	
	team_type team;
};

//used to keep track of visited waypoints in graph traversals
enum visit_status
{
	status_unvisited = 0, 
	status_enqueued, 
	status_visited
};

class WaypointMarker : public Entity
{
public:
	WaypointMarker(float x, float y, float r);
	int getCollisionEffect(void) { return collide_skip | collide_no_gravity | collide_immobile; }
	bool isWaypointMarker()	{return true;}

	WaypointMarker *getRandomNeighbor();

	//adding and removing are symmetric
	void addEdgeTo(WaypointMarker *target);
	void removeEdgeTo(WaypointMarker *target);
	void calculateEdgeWeights();

	//this is not symmetric!
	bool hasEdgeTo(WaypointMarker *target) const;

	inline void setVisitStatus(visit_status status)
	{
		visitStatus = status;
	}

	inline visit_status getVisitStatus() const
	{
		return visitStatus;
	}

	void deleteMe();

	//wrapper for a waypoint marker's edge
	struct Edge
	{
		WaypointMarker *target;
		float			length;
	};

	typedef std::vector<Edge> EdgePool;
	inline EdgePool &getEdges()
	{
		return edgesOut;
	}

	inline float getWeight() const
	{
		return weight;
	}

	inline void setWeight(float newWeight)
	{
		weight = newWeight;
	}

	inline WaypointMarker *getParent() const
	{
		return parent;
	}

	inline void setParent(WaypointMarker *newParent)
	{
		parent = newParent;
	}

private:
	visit_status visitStatus;

	float weight;
	WaypointMarker *parent;

	EdgePool edgesOut;
};

#endif	//MAPMARKER_HPP
