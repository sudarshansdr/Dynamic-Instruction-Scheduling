#ifndef SIM_PROC_H
#define SIM_PROC_H

typedef struct proc_params
{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
} proc_params;

typedef struct RMT
{
    bool valid; // Valid bit
    int robtag; // Tag number
} RMT;

typedef struct ROB
{
    int robindex;
    int value; // Tag number
    int dst;
    bool ready;
    int exception;
    bool miss;
    int robpc;
} ROB;

typedef struct instructions
{
    unsigned long pc;
    int op_type;
    int dest;
    int src1;
    bool src1ready;
    int src2;
    bool src2ready;
} instructions;

typedef struct issueQ
{
    bool valid;
    int dest;
    bool src1ready;
    int src1;
    bool src2ready;
    int src2;
    int prior;
    int OPtype;
    int PCnum;
} issueQ;

typedef struct executelist
{
    bool valid;
    int dest;
    bool src1ready;
    int src1;
    bool src2ready;
    int src2;
    int prior;
    int OPtype;
    int PCnum;
    int OpCounter;
} executelist;

std::queue<instructions> DE;
std::queue<instructions> RN;
std::queue<instructions> RR;
std::queue<instructions> DI;
std::queue<instructions> WB;

// std::queue<instructions> ROB;

// Put additional data structures here as per your requirement

#endif
