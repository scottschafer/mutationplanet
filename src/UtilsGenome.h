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
//#include "Instruction.h"

using namespace std;

class UtilsGenome
{
public:
    static void generateRandomly(int length, Instruction *pOut);
    static void mutate(const Instruction *pIn, Instruction *pOut);
};

#endif /* defined(__MutationPlanet__UtilsGenome__) */
