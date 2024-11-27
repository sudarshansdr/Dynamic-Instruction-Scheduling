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

typedef struct Cycles
{
    int fetch_first, fetch_time,
        decode_first, decode_time,
        rename_first, rename_time,
        regRead_first, regRead_time,
        dispatch_first, dispatch_time,
        issue_first, issue_time,
        execute_first, execute_time,
        writeback_first, writeback_time,
        retire_first, retire_time;

    int Src1Prime, Src2Prime, DestPrime, OperationType;

    // Constructor to initialize all members to -1
    Cycles()
        : fetch_first(-1), fetch_time(-1),
          decode_first(-1), decode_time(-1),
          rename_first(-1), rename_time(-1),
          regRead_first(-1), regRead_time(-1),
          dispatch_first(-1), dispatch_time(-1),
          issue_first(-1), issue_time(-1),
          execute_first(-1), execute_time(-1),
          writeback_first(-1), writeback_time(-1),
          retire_first(-1), retire_time(-1),
          Src1Prime(-1), Src2Prime(-1), DestPrime(-1), OperationType(-1) {}
} Cycles;

typedef struct ROB
{
    int robindex;
    int value; // Tag number
    int dst;
    bool ready;
    int exception;
    bool miss;
    int robpc;
    Cycles cycles;
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
    Cycles cycles;
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
    Cycles cycles;
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
    Cycles cycles;
} executelist;

std::queue<instructions> DE;
std::queue<instructions> RN;
std::queue<instructions> RR;
std::queue<instructions> DI;
std::queue<instructions> WB;

#endif
