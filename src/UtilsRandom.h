//
//  UtilsRandom.h
//  MutationPlanet
//
//  Created by Scott Schafer on 11/28/12.
//
//

#ifndef __MutationPlanet__UtilsRandom__
#define __MutationPlanet__UtilsRandom__

class UtilsRandom
{
public:
    static int getRandom();
    static float getUnitRandom();
    static int getRangeRandom(int minVal, int maxVal);
    static float getRangeRandom(float minVal, float maxVal);
};
#endif /* defined(__MutationPlanet__UtilsRandom__) */
