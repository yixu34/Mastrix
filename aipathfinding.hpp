#ifndef AIPATHFINDING_HPP
#define AIPATHFINDING_HPP

#include <stack>
#include <queue>
#include <deque>
#include <vector>

#include "aiutil.hpp"
#include "mapmarker.hpp"

class AIPathfinding
{
public:
	AIPathfinding();
	~AIPathfinding();

	void constructShortestPathTo(
		WaypointMarker *source, 
		WaypointMarker *destination);

	void clearTraversalSequence();

    bool doesPathExistTo(WaypointMarker *destination);

	WaypointMarker *getNextWaypoint();
	
	inline int getNumRemainingWaypoints() const
	{
		return traversalSequence.size();
	}

private:
	void resetWaypoints(WaypointMarker *source);
	void updateEdge(
		WaypointMarker *parent, 
		const WaypointMarker::Edge &currentEdge);

	void constructTraversalSequenceFrom(WaypointMarker *destination);

	//traversal data structures
	typedef std::deque<WaypointMarker *> TraversalSequence;
	TraversalSequence traversalSequence;

	//container for nodes that have not yet been visited
	class UnvisitedNodes
	{
	public:
		UnvisitedNodes()  {};
		~UnvisitedNodes() {};

		void addNode(WaypointMarker *node);
		WaypointMarker *getNearestNode();
		bool isEmpty() const;
		void clear();
	    
	private:
		typedef std::priority_queue<
			WaypointMarker *, 
			std::vector<WaypointMarker *>, 
			NearerWaypoint<WaypointMarker *> > UnvisitedPriorityQueue;

		UnvisitedPriorityQueue unvisitedPQ;
	};

	UnvisitedNodes unvisitedNodes;
};

#endif	//AIPATHFINDING_HPP