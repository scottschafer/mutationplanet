//
//  UtilsGenome.h
//  MutationPlanet
//
//  Created by Scott Schafer on 12/10/12.
//
//

#ifndef __MutationPlanet__UtilsGenome__
#define __MutationPlanet__UtilsGenome__

#include <iostream>

#include "Constants.h"

using namespace std;

class UtilsGenome
{
public:
    static void generateRandomly(int length, char *pOut);
    static void mutate(const char *pIn, char *pOut);
};

#endif /* defined(__MutationPlanet__UtilsGenome__) */
