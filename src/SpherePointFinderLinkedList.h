//
//  SpherePointFinderSpaceDivison.h
//  BioSphere
//
//  Created by Scott Schafer on 12/23/12.
//
//

#ifndef __BioSphere__SpherePointFinderLinkedList__
#define __BioSphere__SpherePointFinderLinkedList__

#include "gameplay.h"
#include "SphereEntity.h"
#include "constants.h"
#include <set>

using namespace gameplay;
using namespace std;


class SpherePointFinderLinkedList{
public:
    SpherePointFinderLinkedList();
    
	void clear();
    void insert(SphereEntity *);
    void remove(SphereEntity *);
    void moveEntity(SphereEntity *, Vector3);

    int getNearbyEntities(const Vector3 &pt, float distance, SphereEntity **pResultArray, int maxResults = 16, Agent *pExclude = NULL);

    int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16) {
        return getNearbyEntities(pNearEntity->mLocation, distance, pResultArray, maxResults, pNearEntity->mAgent);
    }

private:
    SphereEntityPtr *mSphereEntities;
};

#endif /* defined(__BioSphere__SpherePointFinderSpaceDivison__) */
