//
//  SpherePointFinderSpaceDivison.h
//  BioSphere
//
//  Created by Scott Schafer on 12/23/12.
//
//

#ifndef __BioSphere__SpherePointFinderLinkedList__
#define __BioSphere__SpherePointFinderLinkedList__

#include "BaseSpherePointFinder.h"
#include "constants.h"
#include <set>

using namespace gameplay;
using namespace std;


class SpherePointFinderLinkedList : public BaseSpherePointFinder
{
public:
    SpherePointFinderLinkedList();
    
	void clear();
    void insert(SphereEntity *);
    void remove(SphereEntity *);
    void moveEntity(SphereEntity *, Vector3);
    
    int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16);
    int getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults = 16);
    int getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults, Agent *pExclude);

private:
	SphereEntityPoint3d convertToIntVector(Vector3 &v) const;
    //SetEntityPtr getEntitySet(int x, int y, int z, bool createIfNULL = true);

private:
    SphereEntityPtr *mSphereEntities;
};

#endif /* defined(__BioSphere__SpherePointFinderSpaceDivison__) */
