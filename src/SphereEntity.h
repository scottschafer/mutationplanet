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

class SphereEntity
{
public:
    SphereEntity() {}
    
    SphereEntity(Vector3 location, char type, Agent *pAgent = NULL)
    {
        mLocation = location;
        mType = type;
        mAgent = pAgent;
    }
    
    Agent * mAgent;
    int     mSegmentIndex;
    Vector3 mLocation;
    char    mType;
    bool mIsOccluded;
    bool mIsAnchored;
    SphereWorld *mWorld;
};

typedef SphereEntity * SphereEntityPtr;

#endif
