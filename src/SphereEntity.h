//
//  SphereEntity.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/11/12.
//  Copyright (c) 2012 Scott Schafer. All rights reserved.
//

#ifndef MutationPlanet_SphereEntity_h
#define MutationPlanet_SphereEntity_h

#include "gameplay.h"
#include "Constants.h"

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
    
    static inline int toIntCoordinate(float v) { return max(min(NUM_SUBDIVISIONS - 1, int ((v + 1) / 2 * NUM_SUBDIVISIONS + .5f)),0); }

    
    SphereEntityPoint3d(float x, float y, float z) {
        this->x = toIntCoordinate(x); //max(min(kNumSubdivisions - 1, int ((x + 1) / 2 * kNumSubdivisions + .5)),0);
        this->y = toIntCoordinate(y); //max(min(kNumSubdivisions - 1, int ((y + 1) / 2 * kNumSubdivisions + .5)),0);
        this->z = toIntCoordinate(z); //max(min(kNumSubdivisions - 1, int ((z + 1) / 2 * kNumSubdivisions + .5)),0);
    }
    
    SphereEntityPoint3d(const Vector3 & v) {
        this->x = toIntCoordinate(v.x); //max(min(kNumSubdivisions - 1, int ((x + 1) / 2 * kNumSubdivisions + .5)),0);
        this->y = toIntCoordinate(v.y); //max(min(kNumSubdivisions - 1, int ((y + 1) / 2 * kNumSubdivisions + .5)),0);
        this->z = toIntCoordinate(v.z); //max(min(kNumSubdivisions - 1, int ((z + 1) / 2 * kNumSubdivisions + .5)),0);
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
