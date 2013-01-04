//
//  BaseSpherePointFinder.h
//  BioSphere
//
//  Created by Scott Schafer on 12/22/12.
//
//

#ifndef __BioSphere__BaseSpherePointFinder__
#define __BioSphere__BaseSpherePointFinder__

#include "gameplay.h"
#include "SphereEntity.h"

using namespace gameplay;

/**
 BaseSpherePointFinder
 
 An abstract base class (too bad C++ doesn't have interfaces!) for registering and locating entities on a sphere.
 **/
 
class BaseSpherePointFinder {
public:
    virtual void insert(SphereEntity *) = 0;
    virtual void remove(SphereEntity *) = 0;
    virtual void moveEntity(SphereEntity *, Vector3) = 0;
    
    virtual int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16) = 0;
    virtual int getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults = 16) = 0;

};
#endif /* defined(__BioSphere__BaseSpherePointFinder__) */
