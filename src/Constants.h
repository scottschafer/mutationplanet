//
//  Header.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/21/12.
//
//

#ifndef MutationPlanet_Header_h
#define MutationPlanet_Header_h


enum {
    MAX_SEGMENTS = 20,  
    MAX_GENOME_LENGTH = 15,
    MAX_AGENTS = 30000,
	MAX_SIMPLE_PHOTOSYNTHESIZE = 20000,
//    NUM_SUBDIVISIONS = 80,
    NUM_SUBDIVISIONS = 128,
    TURN_ANGLE = 15,
    HARD_TURN_ANGLE = 90,

	//TAPER_ENERGY_AFTER_NUM_SEGMENTS = 20000,
	MAX_TOTAL_SEGMENTS = 15000,
	MIN_TURNS_PER_SEC = 100,
	MIN_FPS = 10
};

const float CYCLE_ENERGY_COST = 1.0f;
#endif
