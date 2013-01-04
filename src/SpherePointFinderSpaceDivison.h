//
//  SpherePointFinderSpaceDivison.h
//  BioSphere
//
//  Created by Scott Schafer on 12/23/12.
//
//

#ifndef __BioSphere__SpherePointFinderSpaceDivison__
#define __BioSphere__SpherePointFinderSpaceDivison__

#include "BaseSpherePointFinder.h"
#include "constants.h"
#include <set>

using namespace gameplay;
using namespace std;

typedef set<SphereEntityPtr> SetEntity, * SetEntityPtr;


class SpherePointFinderSpaceDivision : public BaseSpherePointFinder
{
public:
    SpherePointFinderSpaceDivision();
    
    void insert(SphereEntity *);
    void remove(SphereEntity *);
    void moveEntity(SphereEntity *, Vector3);
    
    int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16);
    int getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults = 16);
    
private:
    int getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults, SphereEntity *pToExclude);

    void convertToIntVector(Vector3 &v) const;
    SetEntityPtr getEntitySet(int x, int y, int z, bool createIfNULL = true);

private:
    SetEntityPtr mPointsInSubspace[NUM_SUBDIVISIONS][NUM_SUBDIVISIONS][NUM_SUBDIVISIONS];
};

#endif /* defined(__BioSphere__SpherePointFinderSpaceDivison__) */
