/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer, scott.schafer@gmail.com
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/


/**
 SpherePointFinderLinkedList
 
 A quick and dirty method of finding points on a sphere. The method is to carve up the cube
 containing the sphere into subdivisions (in this case, 80 x 80 x 80). Each subdivision (which
 may or may not intersect with the sphere) has a set of the entities within it. We search for
 nearby entities by looking in nearby subdivisions.
 
 A kdtree might be a better option, but this is actually pretty fast. I think the advantage of this
 approach is that when a segment moves within the same subdivision, there's almost no processing that
 needs to happen.
 **/

#include "SpherePointFinderLinkedList.h"

#define ENTITY_INDEX(x,y,z) (x + y*NUM_SUBDIVISIONS + z*NUM_SUBDIVISIONS*NUM_SUBDIVISIONS)
static inline int toIntCoordinate(float v) { return max(min(NUM_SUBDIVISIONS - 1, int ((v + 1) / 2 * NUM_SUBDIVISIONS + .5f)),0); }

SpherePointFinderLinkedList::SpherePointFinderLinkedList()
{
	mSphereEntities = new SphereEntityPtr[NUM_SUBDIVISIONS*NUM_SUBDIVISIONS*NUM_SUBDIVISIONS];
	memset(mSphereEntities, 0, sizeof(SphereEntityPtr)*NUM_SUBDIVISIONS*NUM_SUBDIVISIONS*NUM_SUBDIVISIONS);
}

void SpherePointFinderLinkedList::clear()
{
	memset(mSphereEntities, 0, sizeof(SphereEntityPtr)*NUM_SUBDIVISIONS*NUM_SUBDIVISIONS*NUM_SUBDIVISIONS);
}

void SpherePointFinderLinkedList:: insert(SphereEntity * pEntity)
{
	if (pEntity->mSpherePrev || pEntity->mSphereNext)
		remove(pEntity);

    Vector3 v = pEntity->mLocation;
    SphereEntityPoint3d spherePoint(v);
	if (spherePoint != pEntity->mSpherePoint) {
		pEntity->mSpherePoint = spherePoint;

		int entityIndex = ENTITY_INDEX(spherePoint.x,spherePoint.y,spherePoint.z);
		pEntity->mSpherePrev = NULL;
		pEntity->mSphereNext = mSphereEntities[entityIndex];
		if (pEntity->mSphereNext != NULL) {
			pEntity->mSphereNext->mSpherePrev = pEntity;
		}
		mSphereEntities[entityIndex] = pEntity;
	}
}

void SpherePointFinderLinkedList:: remove(SphereEntity * pEntity)
{
    Vector3 v = pEntity->mLocation;
    SphereEntityPoint3d spherePoint(v);

	int entityIndex = ENTITY_INDEX(spherePoint.x,spherePoint.y,spherePoint.z);

#ifdef _DEBUG
	if (pEntity->mSpherePoint != spherePoint)
		throw "Entity does not have mSpherePoint set correctly";

	SphereEntity *pTest = mSphereEntities[entityIndex];

	if (pTest->mSpherePrev != NULL)
		throw "Head should have no prev";

	bool inSpace = false;
	while (pTest != NULL) {
		if (pTest == pEntity) {
			inSpace = true;
		}
		pTest = pTest->mSphereNext;
	}
	if (! inSpace)
		throw "Not in space";
#endif

	SphereEntity * pNext = pEntity->mSphereNext;
	SphereEntity * pPrev = pEntity->mSpherePrev;

	if (mSphereEntities[entityIndex] == pEntity)
		mSphereEntities[entityIndex] = pNext;

	if (pNext != NULL)
		pNext->mSpherePrev = pPrev;
	if (pPrev != NULL)
		pPrev->mSphereNext = pNext;

	pEntity->mSpherePrev = pEntity->mSphereNext = NULL;
	pEntity->mSpherePoint.reset();
}

//result.x = max(min(NUM_SUBDIVISIONS - 1, int ((v.x + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);

/*
SphereEntityPoint3d SpherePointFinderLinkedList :: convertToIntVector(Vector3 &v) const
{
	SphereEntityPoint3d result;
    result.x = max(min(NUM_SUBDIVISIONS - 1, int ((v.x + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);
    result.y = max(min(NUM_SUBDIVISIONS - 1, int ((v.y + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);
    result.z = max(min(NUM_SUBDIVISIONS - 1, int ((v.z + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);
	return result;
}
*/

void SpherePointFinderLinkedList:: moveEntity(SphereEntity * pEntity, Vector3 newLoc)
{
    SphereEntityPoint3d oldV(pEntity->mLocation);
    SphereEntityPoint3d newV(newLoc);
    
    if (oldV != newV)
    {
		remove(pEntity);
	    pEntity->mLocation = newLoc;
		insert(pEntity);
    }
	else
	{
	    pEntity->mLocation = newLoc;
	}
}

/*
int SpherePointFinderLinkedList:: getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults )
{
	return getNearbyEntities(pNearEntity->mLocation, distance, pResultArray, maxResults, pNearEntity->mAgent);
}

int SpherePointFinderLinkedList::getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults)
{
    return getNearbyEntities(location, distance, pResultArray, maxResults, NULL);
}
*/

int SpherePointFinderLinkedList::getNearbyEntities(const Vector3 &pt, float distance, SphereEntity **pResultArray, int maxResults, Agent *pAgentToExclude)
{
    int result = 0;
    
    // detemine the cube to search
    
//    Vector3 v1
//    Vector3 v2
//	SphereEntityPoint3d c1(pt.x-distance, pt.y-distance,pt.z-distance);
//	SphereEntityPoint3d c2(pt.x+distance, pt.y+distance,pt.z+distance);
    
    int fX = toIntCoordinate(pt.x - distance); //c1.x;
    int tX = toIntCoordinate(pt.x + distance); //c2.x;
    int fY = toIntCoordinate(pt.y - distance); //c1.y;
    int tY = toIntCoordinate(pt.y + distance); //c2.y;
    int fZ = toIntCoordinate(pt.z - distance); //c1.z;
    int tZ = toIntCoordinate(pt.z + distance); //c2.z;
    
    for (int x = fX; x <= tX; x++)
        for (int y = fY; y <= tY; y++)
            for (int z = fZ; z <= tZ; z++)
            {
				int entityIndex = ENTITY_INDEX(x,y,z);
				SphereEntityPtr pEntity = mSphereEntities[entityIndex];
				while (pEntity != NULL) {
                
					if (pAgentToExclude == NULL || pAgentToExclude != pEntity->mAgent) {
						Vector3 testPt = pEntity->mLocation;
                    
						float d = calcDistance(pt, testPt);
						if (d < distance)
						{
							*pResultArray++ = pEntity;
							++result;
                        
							if (result >= maxResults)
								return result;
						}
					}
					pEntity = pEntity->mSphereNext;
                }
            }

    
    return result;
}
