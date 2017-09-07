/*
 * Andrew Sosa
 * as12x
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUMMEMORY 16 /* Maximum number of data words in memory */
#define NUMREGS 8    /* Number of registers */

/* Opcode values for instructions */
#define R 0
#define LW 35
#define SW 43
#define BNE 4
#define HALT 63

/* Funct values for R-type instructions */
#define ADD 32
#define SUB 34

/* Branch Prediction Buffer Values */
#define STRONGLYTAKEN 3
#define WEAKLYTAKEN 2
#define WEAKLYNOTTAKEN 1
#define STRONGLYNOTTAKEN 0

/* Flag for Debugging prints */
#define DEBUG 0
#define DEBUGPASS 0
#define DEBUGBRANCH 0
#define DEBUGINSERT 0

typedef struct IFIDStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
} IFIDType;

typedef struct IDEXStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
  int readData1;                   /* Contents of rs register */
  int readData2;                   /* Contents of rt register */
  int immed;                       /* Immediate field */
  int rsReg;                       /* Number of rs register */
  int rtReg;                       /* Number of rt register */
  int rdReg;                       /* Number of rd register */
  int branchTarget;                /* Branch target, obtained from immediate field */
} IDEXType;

typedef struct EXMEMStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int aluResult;                   /* Result of ALU operation */
  int writeDataReg;                /* Contents of the rt register, used for store word */
  int writeReg;                    /* The destination register */
} EXMEMType;

typedef struct MEMWBStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int writeDataMem;                /* Data read from memory */
  int writeDataALU;                /* Result from ALU operation */
  int writeReg;                    /* The destination register */
} MEMWBType;

typedef struct stateStruct {
  int PC;                                 /* Program Counter */
  unsigned int instrMem[NUMMEMORY];       /* Instruction memory */
  int dataMem[NUMMEMORY];                 /* Data memory */
  int regFile[NUMREGS];                   /* Register file */
  IFIDType IFID;                          /* Current IFID pipeline register */
  IDEXType IDEX;                          /* Current IDEX pipeline register */
  EXMEMType EXMEM;                        /* Current EXMEM pipeline register */
  MEMWBType MEMWB;                        /* Current MEMWB pipeline register */
  int cycles;                             /* Number of cycles executed so far */
  int stalls;
  int branches;
  int mispredictions;
} stateType;


void run();
void printState(stateType*);
void initState(stateType*);
unsigned int instrToInt(char*, char*);
int get_opcode(unsigned int);
void printInstruction(unsigned int);

// Andrew's prototypes
int get_rs(unsigned int);
int get_rt(unsigned int);
int get_rd(unsigned int);
int get_funct(unsigned int);
int get_immed(unsigned int);
int get_opcode(unsigned int);

int main(){
    run();
    return(0);
}

int RegWrite(unsigned int instr) {

    if(instr == 0) return 0;
    switch (get_opcode(instr)) {
        case R:
        case LW:
            return 1;
        case SW:
        case BNE:
        case HALT:
        default:
            return 0;
    }
}

int WBMultiplexor(stateType* state) {
    if(get_opcode(state->MEMWB.instr) == R) {
        return state->MEMWB.writeDataALU;
    } else {
        return state->MEMWB.writeDataMem;
    }
}

void writeBack(stateType* newState, stateType* state) {
    if(RegWrite(state->MEMWB.instr) == 1) {
        newState->regFile[state->MEMWB.writeReg] = WBMultiplexor(state);
        if(DEBUG) printf("Wrote %d to reg %d in WB stage. \n",
                            WBMultiplexor(state), state->MEMWB.writeReg);
    }
}

int fromWord(int i) {
    return i/4;
}

int bufferIndex(int i) {
    return fromWord(i) - 1;
}


/* Hazard detection: returns 1 if hazard. */ // these go in the EX stage
int hazard1A(stateType* state) {
    return (
        //state->EXMEM.writeReg != 0    &&
        RegWrite(state->EXMEM.instr)  &&
        state->EXMEM.writeReg == state->IDEX.rsReg
    );
}

int hazard1B(stateType* state) {
    return (
        //state->EXMEM.writeReg != 0    &&
        RegWrite(state->EXMEM.instr)  &&
        state->EXMEM.writeReg == state->IDEX.rtReg
    );
}

int hazard2A(stateType* state) {
    return (
        RegWrite(state->MEMWB.instr) &&
        //state->MEMWB.writeReg != 0   &&
        state->MEMWB.writeReg == state->IDEX.rsReg
    );
}

int hazard2B(stateType* state) {
    return (
        RegWrite(state->MEMWB.instr) &&
        //state->MEMWB.writeReg != 0   &&
        state->MEMWB.writeReg == state->IDEX.rtReg
    );
}

void run(){

    stateType state;            /* Contains the state of the entire pipeline before the cycle executes */
    stateType newState;         /* Contains the state of the entire pipeline after the cycle executes */
    initState(&state);          /* Initialize the state of the pipeline */

    //int btb[16];                /* Branch Target Buffer */
    int bpb[16];                /* Branch Prediction Buffer */
    for(int i = 0; i < 16; ++i) {
        bpb[i] = WEAKLYNOTTAKEN;
    }

    while (1) {

        printState(&state);

        /* If a halt instruction is entering its WB stage, then all of the legitimate */
        /* instruction have completed. Print the statistics and exit the program. */
        if (get_opcode(state.MEMWB.instr) == HALT || (DEBUG && (newState.cycles > 50))) {
            printf("Total number of cycles executed: %d\n", state.cycles);
            /* Remember to print the number of stalls, branches, and mispredictions! */
            printf("Total number of stalls: %d\n", state.stalls);
            printf("Total number of branches: %d\n", state.branches);
            printf("Total number of mispredicted branches: %d\n", state.mispredictions);

            exit(0);
        }

        newState = state;     /* Start by making newState a copy of the state before the cycle */
        newState.cycles++;

    	/* Modify newState stage-by-stage below to reflect the state of the pipeline after the cycle has executed */
        /* --------------------- IF stage --------------------- */

        // Load instruction for given PC
        newState.IFID.instr = state.instrMem[fromWord(state.PC)];

        // Increment the PC, then store it
        newState.PC = state.PC + 4;
        newState.IFID.PCPlus4 = state.PC + 4;

        if(DEBUGPASS) printf("Passed IF Stage.\n");

        /* --------------------- ID stage --------------------- */

        // Perform writeBack before reading from register.
        writeBack(&newState, &state);

        /* If previous instruction (currently in EX phase) is LW AND rsReg
         * or rdReg is being written to by the LW, we need to insert 1
         * noop into the pipeline.
         */

        if(get_opcode(state.IDEX.instr) == LW &&
          (get_rt(state.IDEX.instr) == get_rs(state.IFID.instr) ||
           get_rt(state.IDEX.instr) == get_rt(state.IFID.instr))) {

            // By removing PC increment, next cycle it will load the same
            // instruction into the IF phase as it did this cycle.
            newState.PC = state.PC;

            // Write current IFID instr into newState to preserve update
            // made during IF phase.
            newState.IFID.instr = state.IFID.instr;
            newState.IFID.PCPlus4 = state.IFID.PCPlus4;

            // Now, we need to insert a noop into the newState.IDEX
            newState.IDEX.instr = 0;
            newState.IDEX.PCPlus4 = 0;
            newState.IDEX.branchTarget = 0;
            newState.IDEX.readData1 = 0;
            newState.IDEX.readData2 = 0;
            newState.IDEX.immed = 0;
            newState.IDEX.rsReg = 0;
            newState.IDEX.rtReg = 0;
            newState.IDEX.rdReg = 0;

            newState.stalls++;

            if(DEBUGINSERT)printf("Inserting 1 stall in ID due to LW dependancy.\n");

        } else { // If not LW -> R type hazard, do the usual thing.

            // Pass instruction forward
            newState.IDEX.instr = state.IFID.instr;

            // Pass PC
            newState.IDEX.PCPlus4 = state.IFID.PCPlus4;

            // Identify register values
            newState.IDEX.rsReg = get_rs(state.IFID.instr);
            newState.IDEX.rtReg = get_rt(state.IFID.instr);
            newState.IDEX.rdReg = get_rd(state.IFID.instr);

            // Read register and immedate fields; use newState.regFile
            // since it has not been modified except for nessessary writeBack
            // changes.
            newState.IDEX.readData1 = newState.regFile[newState.IDEX.rsReg];
            newState.IDEX.readData2 = newState.regFile[newState.IDEX.rtReg];
            newState.IDEX.immed = get_immed(state.IFID.instr);
            newState.IDEX.branchTarget = get_immed(state.IFID.instr);

            // If BNE, check BPB to see if we should take.
            // If BNE taken, update PC to predict we took the jump,
            // meaning queue jumped to instruction with PC. Don't forget
            // to store BNE's PCPlus4, in case we need to revert our decision.
            // Else, do the other thing.

            if(get_opcode(state.IFID.instr) == BNE && (
                bpb[bufferIndex(state.IFID.PCPlus4)] == STRONGLYTAKEN ||
                bpb[bufferIndex(state.IFID.PCPlus4)] == WEAKLYTAKEN
            )) {
                if(DEBUGBRANCH) printf("Predicting %d taken on Instruction %d.\n", bpb[bufferIndex(state.IFID.PCPlus4)], state.IFID.PCPlus4);
                newState.PC = get_immed(state.IFID.instr);
                newState.branches++;

                // Convert current IFID contents into NOOP
                newState.IFID.instr = 0;
                newState.IFID.PCPlus4 = 0;
                newState.stalls++;
                if(DEBUGINSERT)printf("Inserting 1 stall in ID due to taken BNE.\n");

            }
            else if(get_opcode(state.IFID.instr) == BNE && (
                bpb[bufferIndex(state.IFID.PCPlus4)] == STRONGLYNOTTAKEN ||
                bpb[bufferIndex(state.IFID.PCPlus4)] == WEAKLYNOTTAKEN
            )) {
                if(DEBUGBRANCH) printf("Predicting %d not taken on Instruction %d.\n", bpb[bufferIndex(state.IFID.PCPlus4)], state.IFID.PCPlus4);
                // Increment the PC, then store it

            }
        }

        if(DEBUGPASS) printf("Passed ID Stage.\n");

        /* --------------------- EX stage --------------------- */

        // Pass instruction forward
        newState.EXMEM.instr = state.IDEX.instr;

        // Check for 1st Parameter hazard;
        if(hazard1A(&state)) {
            // IF Type 1 && Next instruction
            state.IDEX.readData1 = state.EXMEM.aluResult;
            if(DEBUG) printf("Hazard 1A detected, wrote %d to readData1. \n", state.IDEX.readData1);
        } else if(hazard2A(&state)) { //
            state.IDEX.readData1 = WBMultiplexor(&state);
            if(DEBUG) printf("Hazard 2A detected, wrote %d to readData1. \n", state.IDEX.readData1);
        }

        // Check for 2nd Parameter hazard:
        if(hazard1B(&state)) {
            state.IDEX.readData2 = state.EXMEM.aluResult;
            if(DEBUG) printf("Hazard 1B detected, wrote %d to readData2. \n", state.IDEX.readData2);
        } else if(hazard2B(&state)) { //
            state.IDEX.readData2 = WBMultiplexor(&state);
            if(DEBUG) printf("Hazard 2B detected, wrote %d to readData2. \n", state.IDEX.readData2);
        }

        // Control ALU operation
        switch (get_opcode(state.IDEX.instr)) {
            case BNE:
                // If RS and RT are not equal (we should have jumped)...
                if(state.IDEX.readData1 != state.IDEX.readData2) {

                    // ...but if we didn't jump...
                    if(bpb[bufferIndex(state.IDEX.PCPlus4)] == STRONGLYNOTTAKEN ||
                        bpb[bufferIndex(state.IDEX.PCPlus4)] == WEAKLYNOTTAKEN) {

                        if(DEBUGBRANCH)printf("Didn't jump on %d, should have.\n", state.IDEX.PCPlus4 - 4);

                        // ...we need to fix things and go jump.
                        newState.mispredictions++;

                        // Set PC to our target
                        newState.PC = state.IDEX.branchTarget;

                        // Convert current IFID contents into NOOP
                        newState.IFID.instr = 0;
                        newState.IFID.PCPlus4 = 0;

                        newState.stalls++;


                        // Convert the contents of IDEX into NOOP
                        newState.IDEX.instr = 0;
                        newState.IDEX.PCPlus4 = 0;
                        newState.IDEX.branchTarget = 0;
                        newState.IDEX.readData1 = 0;
                        newState.IDEX.readData2 = 0;
                        newState.IDEX.immed = 0;
                        newState.IDEX.rsReg = 0;
                        newState.IDEX.rtReg = 0;
                        newState.IDEX.rdReg = 0;

                        newState.stalls++;

                        if(DEBUGINSERT)printf("Inserting 2 stalls in EX due to untaken BNE.\n");


                        // Update Prediction Buffer for taken.
                        if(bpb[bufferIndex(state.IDEX.PCPlus4)] < STRONGLYTAKEN) {
                            // Update BPB Value
                            if(DEBUGBRANCH)printf("Incrementing bpb for instruction %d\n", bufferIndex(state.IDEX.PCPlus4));
                            bpb[bufferIndex(state.IDEX.PCPlus4)] = bpb[bufferIndex(state.IDEX.PCPlus4)] + 1;
                        }

                    } else {    // We did jump, we should have jumped. (Right Choice)
                        // Update Prediction Buffer for taken.
                        if(bpb[bufferIndex(state.IDEX.PCPlus4)] < STRONGLYTAKEN) {
                            if(DEBUGBRANCH)printf("Right choice to jump. Incrementing bpb for instruction %d\n", bufferIndex(state.IDEX.PCPlus4));
                            // Update BPB Value
                            bpb[bufferIndex(state.IDEX.PCPlus4)] = bpb[bufferIndex(state.IDEX.PCPlus4)] + 1;
                        }
                    }
                } else {
                    // We jumped when we should not have.  (Wrong Choice)
                    if(bpb[bufferIndex(state.IDEX.PCPlus4)] == STRONGLYTAKEN ||
                        bpb[bufferIndex(state.IDEX.PCPlus4)] == WEAKLYTAKEN) {

                        if(DEBUGBRANCH)printf("Jumped on %d, shouldn't have.\n", state.IDEX.PCPlus4 - 4);
                        newState.mispredictions++;

                        // Set PC to BNE's PCPlus4
                        newState.PC = state.IDEX.PCPlus4;

                        // Convert current IFID contents into NOOP
                        newState.IFID.instr = 0;
                        newState.IFID.PCPlus4 = 0;

                        //newState.stalls++;
                        newState.branches++;

                        // Convert the contents of IDEX into NOOP
                        newState.IDEX.instr = 0;
                        newState.IDEX.PCPlus4 = 0;
                        newState.IDEX.branchTarget = 0;
                        newState.IDEX.readData1 = 0;
                        newState.IDEX.readData2 = 0;
                        newState.IDEX.immed = 0;
                        newState.IDEX.rsReg = 0;
                        newState.IDEX.rtReg = 0;
                        newState.IDEX.rdReg = 0;

                        newState.stalls++;

                        if(DEBUGINSERT)printf("Inserting 1 stall in EX due to mistaken BNE.\n");


                        // Update Prediction Buffer to not taken.
                        if(bpb[bufferIndex(state.IDEX.PCPlus4)] > STRONGLYNOTTAKEN) {
                            // Update BPB Value
                            bpb[bufferIndex(state.IDEX.PCPlus4)] = bpb[bufferIndex(state.IDEX.PCPlus4)] - 1;
                        }

                    } else { // We didn't jumped, we should not have. (Right Choice)
                        // Update Prediction Buffer for taken.
                        if(bpb[bufferIndex(state.IDEX.PCPlus4)] > STRONGLYNOTTAKEN) {
                            // Update BPB Value
                            bpb[bufferIndex(state.IDEX.PCPlus4)] = bpb[bufferIndex(state.IDEX.PCPlus4)] - 1;
                        }
                    }
                }
                newState.EXMEM.aluResult = 0;
                break;
            case R:
                if(get_funct(state.IDEX.instr) == ADD) {
                    newState.EXMEM.aluResult = state.IDEX.readData1 + state.IDEX.readData2;
                } else {
                    newState.EXMEM.aluResult = state.IDEX.readData1 - state.IDEX.readData2;
                }
                break;

            case LW:
            case SW:
                newState.EXMEM.aluResult = state.IDEX.readData1 + state.IDEX.immed;
                break;
            default:
                newState.EXMEM.aluResult = 0;
                break;

        }

        // writeDataReg: used strictly in store word, always from readData2
        newState.EXMEM.writeDataReg = state.IDEX.readData2;

        // writeReg: destination register. Either rt or rd based on instr.
        if(get_opcode(state.IDEX.instr) == R) {
            newState.EXMEM.writeReg = state.IDEX.rdReg;
        } else {
            newState.EXMEM.writeReg = state.IDEX.rtReg;
        }

        if(DEBUGPASS) printf("Passed EX Stage.\n");

        /* --------------------- MEM stage --------------------- */

        // Pass instruction forward
        newState.MEMWB.instr = state.EXMEM.instr;

        // Read or write data memory based on address (aluResult)
        if(get_opcode(state.EXMEM.instr) == SW) {
            // Only write if SW
            newState.dataMem[fromWord(state.EXMEM.aluResult)] = state.EXMEM.writeDataReg;
            if(DEBUG) printf("Stored %d into memory %d \n", state.EXMEM.writeDataReg, fromWord(state.EXMEM.aluResult));
        } else if (get_opcode(state.EXMEM.instr) == LW){
            // Load if we need to.
            newState.MEMWB.writeDataMem = state.dataMem[fromWord(state.EXMEM.aluResult)];
            if(DEBUG) printf("Loaded %d from memory %d into writeDataMem \n", newState.MEMWB.writeDataMem, fromWord(state.EXMEM.aluResult));
        } else {
            newState.MEMWB.writeDataMem = 0;
        }

        // Move forward ALU result
        newState.MEMWB.writeDataALU = state.EXMEM.aluResult;

        // Move forward writeReg
        if(RegWrite(newState.MEMWB.instr)) newState.MEMWB.writeReg = state.EXMEM.writeReg;
        else newState.MEMWB.writeReg = 0;

        if(DEBUGPASS) printf("Passed EX Stage.\n");

        /* --------------------- WB stage --------------------- */
        //writeBack(&newState, &state);

        if(DEBUGPASS) printf("Passed WB Stage.\n");

        state = newState;    /* The newState now becomes the old state before we execute the next cycle */
    }
}


/******************************************************************/
/* The initState function accepts a pointer to the current        */
/* state as an argument, initializing the state to pre-execution  */
/* state. In particular, all registers are zero'd out. All        */
/* instructions in the pipeline are NOOPS. Data and instruction   */
/* memory are initialized with the contents of the assembly       */
/* input file.                                                    */
/*****************************************************************/
void initState(stateType *statePtr)
{
    unsigned int dec_inst;
    int data_index = 0;
    int inst_index = 0;
    char line[130];
    char instr[5];
    char args[130];
    char* arg;

    statePtr->PC = 0;
    statePtr->cycles = 0;
    statePtr->stalls = 0;
    statePtr->mispredictions = 0;
    statePtr->branches = 0;

    /* Zero out data, instructions, and registers */
    memset(statePtr->dataMem, 0, 4*NUMMEMORY);
    memset(statePtr->instrMem, 0, 4*NUMMEMORY);
    memset(statePtr->regFile, 0, 4*NUMREGS);

    /* Parse assembly file and initialize data/instruction memory */
    while(fgets(line, 130, stdin)){
        if(sscanf(line, "\t.%s %s", instr, args) == 2){
            arg = strtok(args, ",");
            while(arg != NULL){
                statePtr->dataMem[data_index] = atoi(arg);
                data_index += 1;
                arg = strtok(NULL, ",");
            }
        }
        else if(sscanf(line, "\t%s %s", instr, args) == 2){
            dec_inst = instrToInt(instr, args);
            statePtr->instrMem[inst_index] = dec_inst;
            inst_index += 1;
        }
    }

    /* Zero-out all registers in pipeline to start */
    statePtr->IFID.instr = 0;
    statePtr->IFID.PCPlus4 = 0;
    statePtr->IDEX.instr = 0;
    statePtr->IDEX.PCPlus4 = 0;
    statePtr->IDEX.branchTarget = 0;
    statePtr->IDEX.readData1 = 0;
    statePtr->IDEX.readData2 = 0;
    statePtr->IDEX.immed = 0;
    statePtr->IDEX.rsReg = 0;
    statePtr->IDEX.rtReg = 0;
    statePtr->IDEX.rdReg = 0;

    statePtr->EXMEM.instr = 0;
    statePtr->EXMEM.aluResult = 0;
    statePtr->EXMEM.writeDataReg = 0;
    statePtr->EXMEM.writeReg = 0;

    statePtr->MEMWB.instr = 0;
    statePtr->MEMWB.writeDataMem = 0;
    statePtr->MEMWB.writeDataALU = 0;
    statePtr->MEMWB.writeReg = 0;
 }


 /***************************************************************************************/
 /*              You do not need to modify the functions below.                         */
 /*                They are provided for your convenience.                              */
 /***************************************************************************************/


/*************************************************************/
/* The printState function accepts a pointer to a state as   */
/* an argument and prints the formatted contents of          */
/* pipeline register.                                        */
/* You should not modify this function.                      */
/*************************************************************/
void printState(stateType *statePtr)
{
    int i;
    printf("\n********************\nState at the beginning of cycle %d:\n", statePtr->cycles+1);
    printf("\tPC = %d\n", statePtr->PC);
    printf("\tData Memory:\n");
    for (i=0; i<(NUMMEMORY/2); i++) {
        printf("\t\tdataMem[%d] = %d\t\tdataMem[%d] = %d\n",
            i, statePtr->dataMem[i], i+(NUMMEMORY/2), statePtr->dataMem[i+(NUMMEMORY/2)]);
    }
    printf("\tRegisters:\n");
    for (i=0; i<(NUMREGS/2); i++) {
        printf("\t\tregFile[%d] = %d\t\tregFile[%d] = %d\n",
            i, statePtr->regFile[i], i+(NUMREGS/2), statePtr->regFile[i+(NUMREGS/2)]);
    }
    printf("\tIF/ID:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IFID.PCPlus4);
    printf("\tID/EX:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IDEX.PCPlus4);
    printf("\t\tbranchTarget: %d\n", statePtr->IDEX.branchTarget);
    printf("\t\treadData1: %d\n", statePtr->IDEX.readData1);
    printf("\t\treadData2: %d\n", statePtr->IDEX.readData2);
    printf("\t\timmed: %d\n", statePtr->IDEX.immed);
    printf("\t\trs: %d\n", statePtr->IDEX.rsReg);
    printf("\t\trt: %d\n", statePtr->IDEX.rtReg);
    printf("\t\trd: %d\n", statePtr->IDEX.rdReg);
    printf("\tEX/MEM:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\taluResult: %d\n", statePtr->EXMEM.aluResult);
    printf("\t\twriteDataReg: %d\n", statePtr->EXMEM.writeDataReg);
    printf("\t\twriteReg:%d\n", statePtr->EXMEM.writeReg);
    printf("\tMEM/WB:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteDataMem: %d\n", statePtr->MEMWB.writeDataMem);
    printf("\t\twriteDataALU: %d\n", statePtr->MEMWB.writeDataALU);
    printf("\t\twriteReg: %d\n", statePtr->MEMWB.writeReg);
}

/*************************************************************/
/*  The instrToInt function converts an instruction from the */
/*  assembly file into an unsigned integer representation.   */
/*  For example, consider the add $0,$1,$2 instruction.      */
/*  In binary, this instruction is:                          */
/*   000000 00001 00010 00000 00000 100000                   */
/*  The unsigned representation in decimal is therefore:     */
/*   2228256                                                 */
/*************************************************************/
unsigned int instrToInt(char* inst, char* args){

    int opcode, rs, rt, rd, shamt, funct, immed;
    unsigned int dec_inst;

    if((strcmp(inst, "add") == 0) || (strcmp(inst, "sub") == 0)){
        opcode = 0;
        if(strcmp(inst, "add") == 0)
            funct = ADD;
        else
            funct = SUB;
        shamt = 0;
        rd = atoi(strtok(args, ",$"));
        rs = atoi(strtok(NULL, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + (rd << 11) + (shamt << 6) + funct;
    } else if((strcmp(inst, "lw") == 0) || (strcmp(inst, "sw") == 0)){
        if(strcmp(inst, "lw") == 0)
            opcode = LW;
        else
            opcode = SW;
        rt = atoi(strtok(args, ",$"));
        immed = atoi(strtok(NULL, ",("));
        rs = atoi(strtok(NULL, "($)"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;
    } else if(strcmp(inst, "bne") == 0){
        opcode = 4;
        rs = atoi(strtok(args, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        immed = atoi(strtok(NULL, ","));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;
    } else if(strcmp(inst, "halt") == 0){
        opcode = 63;
        dec_inst = (opcode << 26);
    } else if(strcmp(inst, "noop") == 0){
        dec_inst = 0;
    }
    return dec_inst;
}

int get_rs(unsigned int instruction){
    return( (instruction>>21) & 0x1F);
}

int get_rt(unsigned int instruction){
    return( (instruction>>16) & 0x1F);
}

int get_rd(unsigned int instruction){
    return( (instruction>>11) & 0x1F);
}

int get_funct(unsigned int instruction){
    return(instruction & 0x3F);
}

int get_immed(unsigned int instruction){
    return(instruction & 0xFFFF);
}

int get_opcode(unsigned int instruction){
    return(instruction>>26);
}

/*************************************************/
/*  The printInstruction decodes an unsigned     */
/*  integer representation of an instruction     */
/*  into its string representation and prints    */
/*  the result to stdout.                        */
/*************************************************/
void printInstruction(unsigned int instr)
{
    char opcodeString[10];
    if (instr == 0){
      printf("NOOP\n");
    } else if (get_opcode(instr) == R) {
        if(get_funct(instr)!=0){
            if(get_funct(instr) == ADD)
                strcpy(opcodeString, "add");
            else
                strcpy(opcodeString, "sub");
            printf("%s $%d,$%d,$%d\n", opcodeString, get_rd(instr), get_rs(instr), get_rt(instr));
        }
        else{
            printf("NOOP\n");
        }
    } else if (get_opcode(instr) == LW) {
        printf("%s $%d,%d($%d)\n", "lw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == SW) {
        printf("%s $%d,%d($%d)\n", "sw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == BNE) {
        printf("%s $%d,$%d,%d\n", "bne", get_rs(instr), get_rt(instr), get_immed(instr));
    } else if (get_opcode(instr) == HALT) {
        printf("%s\n", "halt");
    }
}
