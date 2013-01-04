//
//  Parameters.h
//  BioSphere
//
//  Created by Scott Schafer on 12/24/12.
//
//

#ifndef __BioSphere__Parameters__
#define __BioSphere__Parameters__


/**
 * All the tweakable parameters
 */

class Parameters
{
public:
    static float getMoveDistance();
    
    // global
    static int speed;
    static int mutationPercent;
    static float cellSize;
    
    // energy cost / gain
    static float cycleEnergyCost;
    static float moveAndEatEnergyCost;
    static float moveEnergyCost;
    static float photoSynthesizeEnergyGain;
    
    // spawn energy
    static float baseSpawnEnergy;
    static float extraSpawnEnergyPerSegment;
    
    // lifespan
    static float baseLifespan;
    static float extraLifespanPerSegmnet;
    
    // moving
    static int cyclesForMove;
    static bool allowSelfOverlap;
    
    // looking
    static int lookDistance;
};
#endif /* defined(__BioSphere__Parameters__) */
