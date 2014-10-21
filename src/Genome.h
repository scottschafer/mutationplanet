#ifndef MutationPlanet_Genome_h
#define MutationPlanet_Genome_h

#include "Constants.h"
#include "InstructionSet.h"
//#include "Instruction.h"


class Genome {
public:
	Genome();

	int initialize(const char *pInstructions);

	Genome mutate();
    
    eSegmentExecutionType getExecType(int i) {
        return (eSegmentExecutionType) (mInstructions[i] & eExecTypeMask);
    }
    
    char getInstruction(int i) {
        return (mInstructions[i] & eInstructionMask);
    }
    
    operator const char *() const { return mInstructions; }
    
    Genome& operator =(const Genome &rhs) {
        memcpy(mInstructions,rhs.mInstructions, sizeof(mInstructions));
        return *this;
    }
    
private:
    char mInstructions[MAX_GENOME_LENGTH + 1];
};

inline const bool operator == (Genome const& lhs, Genome const& rhs) { return strcmp((const char *)lhs, (const char *)rhs) == 0; }
inline const bool operator != (Genome const& lhs, Genome const& rhs) { return strcmp((const char *)lhs, (const char *)rhs) != 0; }
inline const bool operator < (Genome const& lhs, Genome const& rhs)  { return strcmp((const char *)lhs, (const char *)rhs) == -1; }

#endif