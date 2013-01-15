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
 UtilsRandom
 
 Simple random number utility class.
 **/

#include "UtilsRandom.h"
#include <stdlib.h>

int UtilsRandom :: getRandom()
{
    return rand();
}

float UtilsRandom :: getUnitRandom()
{
    float result = -1.0 + 2.0 * getRandom() / RAND_MAX;
    return result;
}

int UtilsRandom :: getRangeRandom(int minVal, int maxVal)
{
    if (minVal >= maxVal)
        return minVal;
    
    return minVal + rand() % (maxVal - minVal + 1);
}

float UtilsRandom :: getRangeRandom(float minVal, float maxVal)
{
    float result = minVal + float(getRandom()) / float(RAND_MAX) * (maxVal - minVal);
    if ((result < minVal) || (result > maxVal))
        throw "what?";
    
    return result;
}