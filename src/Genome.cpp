#include "Genome.h"
#include "UtilsRandom.h"

Genome::Genome()
{
}

int Genome :: initialize(const char *pInstructions)
{
	memset(mInstructions, 0, sizeof(mInstructions));

	int i = 0;
	while (pInstructions[i])
	{
		mInstructions[i].instruction = pInstructions[i];
		mInstructions[i].executeType = eAlways;
		++i;
	}
	return i;
}

int Genome :: initializeHasExecutionType(const char *pInstructions)
{
	memset(mInstructions, 0, sizeof(mInstructions));

	int i = 0;
	while (*pInstructions)
	{
		mInstructions[i].instruction = *pInstructions++;
		mInstructions[i].executeType = (eSegmentExecutionType) *pInstructions++;
		++i;
	}
	return i;
}


int Genome :: initialize(const Instruction *pInstructions)
{
	memset(mInstructions, 0, sizeof(mInstructions));

	int i = 0;
	while (pInstructions[i].instruction)
	{
		mInstructions[i] = pInstructions[i];
		++i;
	}
	return i;
}


const std::string Genome::toString()
{
	std::string r;
	for (int i = 0; i <= MAX_SEGMENTS; i++)
	{
		if (! mInstructions[i].instruction)
			break;
		r += (char) mInstructions[i].executeType;
		r += mInstructions[i].instruction;
	}
	return r;
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

static Instruction randomInstruction()
{
	Instruction result;
	result.instruction = InstructionSet::getRandomInstruction();
	if (InstructionSet::instructionSupportsConditions(result.instruction))
		result.executeType = getRandomExecutionType();
	else
		result.executeType = eAlways;
	return result;
}

Genome Genome :: mutate()
{
	Genome result;
	result.initialize("");
	Instruction * pOut = result.mInstructions;
	Instruction * pIn = mInstructions;

	int length = 0;
	while (mInstructions[length].instruction)
		++length;

    int mutationType = (int) UtilsRandom::getRangeRandom(0, 2);

    switch (mutationType)
    {
        default:
        case 0: {
            if (length < (MAX_GENOME_LENGTH-1))
            {
                // insert a new random instruction
                Instruction insertInstruction = randomInstruction();
                int insertPosition = (int) UtilsRandom::getRangeRandom(0,length);
                while (length-- > 0)
                {
                    if (insertPosition-- == 0)
                        *pOut++ = insertInstruction;
                    *pOut++ = *pIn++;
                }
                if (! insertPosition)
                    *pOut++ = insertInstruction;
				(*pOut).instruction = 0;
            }
            else
            {
				memcpy(pOut, pIn, sizeof(mInstructions));
            }
            break; }
            
        case 1: {
            // modify an instruction randomly
            Instruction mutateInstruction = randomInstruction();
            int charPosition = (int) UtilsRandom::getRangeRandom(0,length-1);
			memcpy(pOut, pIn, sizeof(mInstructions));
			if (UtilsRandom::getRangeRandom(0, 1)) {
				pOut[charPosition].instruction = mutateInstruction.instruction;
				if (! InstructionSet::instructionSupportsConditions(pOut[charPosition].instruction))
					pOut[charPosition].executeType = eAlways;
			}
			else
				if (InstructionSet::instructionSupportsConditions(pOut[charPosition].instruction))
					pOut[charPosition].executeType = mutateInstruction.executeType;
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
				while ((*pIn).instruction)
                {
                    if (!charPosition)
                        ++pIn;
                    --charPosition;
                    *pOut++ = *pIn++;
                }
				(*pOut).instruction = 0;
            }
            break; }
    }
	return result;
}
