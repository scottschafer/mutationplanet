#include "Genome.h"
#include "UtilsRandom.h"

Genome::Genome()
{
}

int Genome :: initialize(const char *pInstructions)
{
    strcpy(mInstructions, pInstructions);
    return strlen(mInstructions);
}

static eSegmentExecutionType getRandomExecutionType()
{
	int r = rand() % 3;
	switch (r) {
	case 0:
		return eIf;
	case 1:
		return eNotIf;
	default:
		return eAlways;
	}
}

static char randomInstruction()
{
	char result = InstructionSet::getRandomInstruction();
	if (InstructionSet::instructionSupportsConditions(result))
        result |= (char)getRandomExecutionType();

//    if (UtilsRandom::getRangeRandom(0, 3) == 0)
//        result |= eFixed;
    
    return result;
}

Genome Genome :: mutate()
{
	Genome result;
    char * pIn = mInstructions;
    char * pOut = result.mInstructions;
    

    int length = strlen(pIn);

    int mutationType = (int) UtilsRandom::getRangeRandom(0, 3);

    switch (mutationType)
    {
        case 0: {
            if (length < (MAX_GENOME_LENGTH-1))
            {
                // insert a new random instruction
                char insertInstruction = randomInstruction();
                int insertPosition = (int) UtilsRandom::getRangeRandom(0,length);
                while (length-- > 0)
                {
                    if (insertPosition-- == 0)
                        *pOut++ = insertInstruction;
                    *pOut++ = *pIn++;
                }
                if (! insertPosition)
                    *pOut++ = insertInstruction;
				(*pOut) = 0;
            }
            else
            {
				memcpy(pOut, pIn, sizeof(mInstructions));
            }
            break; }
            
        case 1: {
            // modify an instruction randomly
            char mutateInstruction = randomInstruction();
            int charPosition = (int) UtilsRandom::getRangeRandom(0,length-1);
			memcpy(pOut, pIn, sizeof(mInstructions));

            char existing = pOut[charPosition];
            if (InstructionSet::instructionSupportsConditions(existing) && UtilsRandom::getRangeRandom(0, 1)) {
                pOut[charPosition] = (existing & eInstructionMask) | (mutateInstruction & eExecTypeMask);
            }
            else {
                pOut[charPosition] = (existing & eExecTypeMask) | (mutateInstruction & eInstructionMask);
            }

            break; }
            
        case 2: {
            // delete an instruction randomly
            if (length == 1)
            {
				memcpy(pOut, pIn, sizeof(mInstructions));
            }
            else
            {
                int charPosition = (int) UtilsRandom::getRangeRandom(0,length-1);
				while ((*pIn))
                {
                    if (!charPosition)
                        ++pIn;
                    --charPosition;
                    *pOut++ = *pIn++;
                }
				(*pOut) = 0;
            }
            break; }
            
        default: {
            memcpy(pOut, pIn, sizeof(mInstructions));

            // swap two instructions randomly
            if (length > 1) {

                int firstPos = (length == 2) ? 0 : UtilsRandom::getRangeRandom(0, length - 2);
                if ((firstPos+1) >= length)
                    throw "x";
                char save = pOut[firstPos];
                pOut[firstPos] = pOut[firstPos+1];
                pOut[firstPos+1] = save;
            }
            break; }
    }
	return result;
}
