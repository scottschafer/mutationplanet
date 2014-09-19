//
//  SphereEntity.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/11/12.
//  Copyright (c) 2012 SlideRocket. All rights reserved.
//

#ifndef MutationPlanet_SphereEntity_h
#define MutationPlanet_SphereEntity_h

#include "gameplay.h"
using namespace gameplay;
using namespace std;

class SphereWorld;
class Agent;
class SpherePointFinderSpaceDivision;

class SphereEntityPoint3d {
public:
	SphereEntityPoint3d() {
		x = y = z = 0;
	}

	void reset() {
		x = y = z = 0;
	}

	const bool operator == (const SphereEntityPoint3d & rhs) { return x == rhs.x && y == rhs.y && z == rhs.z; }
	const bool operator != (const SphereEntityPoint3d & rhs) { return x != rhs.x || y != rhs.y || z != rhs.z; }

	int x, y, z;
};

class SphereEntity
{
public:
    SphereEntity() {
		mSpherePrev = mSphereNext = NULL;
		mAgent = NULL;
		mWorld = NULL;
	}
    
    SphereEntity(Vector3 location, char type, Agent *pAgent = NULL)
    {
        mLocation = location;
        mType = type;
        mAgent = pAgent;
		mSpherePrev = mSphereNext = NULL;
		mAgent = NULL;
		mWorld = NULL;
    }
    
    Agent * mAgent;
    int     mSegmentIndex;
    Vector3 mLocation;
    char    mType;
    bool mIsOccluded;
    bool mIsAnchored;
    SphereWorld *mWorld;

	SphereEntity * mSpherePrev, *mSphereNext;
	SphereEntityPoint3d mSpherePoint;
};

typedef SphereEntity * SphereEntityPtr;

// use the cheap "Manhattan distance" method....
inline float calcDistance(Vector3 v1, Vector3 v2)
{
    float x = v1.x - v2.x;
    float y = v1.y - v2.y;
    float z = v1.z - v2.z;
    
    return max(fabs(x),max(fabs(y),fabs(z)));
}


#endif
