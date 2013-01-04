/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer
 
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
 SpherePointFinderSpaceDivision
 
 A quick and dirty method of finding points on a sphere. The method is to carve up the cube
 containing the sphere into subdivisions (in this case, 80 x 80 x 80). Each subdivision (which
 may or may not intersect with the sphere) has a set of the entities within it. We search for
 nearby entities by looking in nearby subdivisions.
 
 A kdtree might be a better option, but this is actually pretty fast. I think the advantage of this
 approach is that when a segment moves within the same subdivision, there's almost no processing that
 needs to happen.
 **/

#include "SpherePointFinderSpaceDivison.h"

SpherePointFinderSpaceDivision::SpherePointFinderSpaceDivision()
{
    for (int x = 0; x < NUM_SUBDIVISIONS; x++)
        for (int y = 0; y < NUM_SUBDIVISIONS; y++)
            for (int z = 0; z < NUM_SUBDIVISIONS; z++)
                mPointsInSubspace[x][y][z] = 0;
}

void SpherePointFinderSpaceDivision:: insert(SphereEntity * pEntity)
{
    Vector3 v = pEntity->mLocation;
    convertToIntVector(v);
    getEntitySet(v.x,v.y,v.z)->insert(pEntity);
}

void SpherePointFinderSpaceDivision:: remove(SphereEntity * pEntity)
{
    Vector3 v = pEntity->mLocation;
    convertToIntVector(v);
    SetEntityPtr pEntitySet = getEntitySet(v.x,v.y,v.z, false);
    if (pEntitySet)
        pEntitySet->erase(pEntity);
}

void SpherePointFinderSpaceDivision :: convertToIntVector(Vector3 &v) const
{
    v.x = max(min(NUM_SUBDIVISIONS - 1, int ((v.x + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);
    v.y = max(min(NUM_SUBDIVISIONS - 1, int ((v.y + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);
    v.z = max(min(NUM_SUBDIVISIONS - 1, int ((v.z + 1) / 2 * NUM_SUBDIVISIONS + .5)),0);
}

SetEntityPtr SpherePointFinderSpaceDivision :: getEntitySet(int x, int y, int z, bool createIfNULL /*= true */)
{
    if ((x < 0) || (x >= NUM_SUBDIVISIONS) || (y < 0) || (y >= NUM_SUBDIVISIONS) || (z < 0) || (z >= NUM_SUBDIVISIONS))
        throw "argh";
    
    SetEntityPtr result = mPointsInSubspace[x][y][z];
    
    if (result == NULL && createIfNULL)
        result = mPointsInSubspace[x][y][z] = new SetEntity();
    return result;
}


void SpherePointFinderSpaceDivision:: moveEntity(SphereEntity * pEntity, Vector3 newLoc)
{
    Vector3 oldV = pEntity->mLocation;
    Vector3 newV = newLoc;
    
    convertToIntVector(oldV);
    convertToIntVector(newV);
    
    if (oldV != newV)
    {
        SetEntityPtr pOldEntitySet = getEntitySet(oldV.x,oldV.y,oldV.z, false);
        if (pOldEntitySet)
            pOldEntitySet->erase(pEntity);
        SetEntityPtr pNewEntitySet = getEntitySet(newV.x,newV.y,newV.z);
        if (! pNewEntitySet)
            throw "what?";
        pNewEntitySet->insert(pEntity);
    }
    
    pEntity->mLocation = newLoc;
}

// use the cheap "Manhattan distance" method....
inline float calcDistance(Vector3 v1, Vector3 v2)
{
    float x = v1.x - v2.x;
    float y = v1.y - v2.y;
    float z = v1.z - v2.z;

    return max(fabs(x),max(fabs(y),fabs(z)));
}

int SpherePointFinderSpaceDivision:: getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults /*= 16 */)
{
    return getNearbyEntities(pNearEntity->mLocation, distance, pResultArray, maxResults, pNearEntity);
}

int SpherePointFinderSpaceDivision::getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults)
{
    return getNearbyEntities(location, distance, pResultArray, maxResults, NULL);
}

int SpherePointFinderSpaceDivision::getNearbyEntities(Vector3 pt, float distance, SphereEntity **pResultArray, int maxResults, SphereEntity *pToExclude)
{
    int result = 0;
    Vector3 v = pt;
    convertToIntVector(v);
    
    // detemine the cube to search
    Vector3 c1(pt.x-distance, pt.y-distance,pt.z-distance);
    convertToIntVector(c1);
    
    Vector3 c2(pt.x+distance, pt.y+distance,pt.z+distance);
    convertToIntVector(c2);
    
    int fX = c1.x;
    int tX = c2.x;
    int fY = c1.y;
    int tY = c2.y;
    int fZ = c1.z;
    int tZ = c2.z;
    
    for (int x = fX; x <= tX; x++)
        for (int y = fY; y <= tY; y++)
            for (int z = fZ; z <= tZ; z++)
            {
                SetEntityPtr setEntityPtr = getEntitySet(x,y,z, false);
                if (! setEntityPtr)
                    continue;
                
                SetEntity & entitySet = * setEntityPtr;
                for (SetEntity::iterator i = entitySet.begin(); i != entitySet.end(); i++)
                {
                    SphereEntity * pEntity = *i;
                    
                    if (pToExclude == pEntity)
                        continue;
                    
                    Vector3 testPt = pEntity->mLocation;
                    
                    float d = calcDistance(pt, testPt);
                    if (d ==0)
                        continue;
                    if (d < distance)
                    {
                        *pResultArray++ = pEntity;
                        ++result;
                        
                        if (result >= maxResults)
                            return result;
                    }
                }
            }
    
    return result;
}
