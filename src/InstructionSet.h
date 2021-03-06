//
//  InstructionSet.h
//  MutationPlanet
//
//  Created by Scott Schafer on 12/11/12.
//
//

#ifndef __MutationPlanet__InstructionSet__
#define __MutationPlanet__InstructionSet__

#include <iostream>
#include <set>

enum eSegmentExecutionType {
    eExecTypeMask = 192,
    
    eIf = 128,
    eNotIf = 64,
	eAlways = 0
};

enum eInstructions
{
    eInstructionMask = 31,
    
    eBarrier1 = 5,
    eBarrier2,
    eBarrier3,
    eBarrier4,
    
    eInstructionPhotosynthesize,
    eInstructionMoveAndEat,
    eInstructionMove,
	eInstructionSetAnchored,
    eInstructionHyper,
    eInstructionSleep,
    eInstructionTurnLeft,
    eInstructionTurnRight,
    eInstructionHardTurnLeft,
    eInstructionHardTurnRight,
    eInstructionTestSeeFood,
    eInstructionTestBlocked,
    eInstructionTestPreyedOn,
    eInstructionTestOccluded,
	eInstructionTestFacingSibling,

	eInstructionOrientTowardsPole,
	
	eLastInstruction
};


class InstructionSet
{
public:
    static void reset();
    static char getRandomInstruction();
    static void setInstructionAvailable(char instruction, bool isAvailable);
	static bool instructionSupportsConditions(char instruction);

    static std::set<char> getAllAvailableInstructions();
};

#endif /* defined(__MutationPlanet__InstructionSet__) */
