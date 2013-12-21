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

enum eInstructions
{
    eBarrier1 = '.',
    eBarrier2 = ',',
    eBarrier3 = '_',
    eInstructionMoveAndEat = 'M',
    eInstructionMove = 'n',
    eInstructionTurnLeft = '<',
    eInstructionTurnRight = '>',
    eInstructionHardTurnLeft = '[',
    eInstructionHardTurnRight = ']',
    eInstructionSleep = 'Z',
    eInstructionTestSeeFood = 'F',
    eInstructionTestBlocked = 'B',
    eInstructionTestNotSeeFood = 'f',
    eInstructionTestNotBlocked = 'b',
    eInstructionTestPreyedOn = 'p',
    eInstructionTestOccluded = 'o',
    eInstructionPhotosynthesize = '*',
    eInstructionFakePhotosynthesize = '+',
	eInstructionEndIf = '|',
	eInstructionHyper = 'H',
	eInstructionPhotosynthesizeLess = 'x',
	eInstructionClearCondition = '0'

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
