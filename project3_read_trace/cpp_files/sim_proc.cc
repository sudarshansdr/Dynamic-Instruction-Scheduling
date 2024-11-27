#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <queue>
#include <cmath>
#include "sim_proc.h"
#include <climits>
bool end = true;
int ROBHEAD = 0;
int ROBTAIL = 0;
int priority = 0;
int clock = 1;
int seq = 0;
int lastseq = 0;
int lastcycle = 0;

void WakeupDIorRR(std::queue<instructions> &DIorRR, int ReadyDest)
{
    std::queue<instructions> tempQueue; // Temporary queue to hold updated instructions
    while (!DIorRR.empty())
    {
        instructions currentInst = DIorRR.front();
        DIorRR.pop();
        if (currentInst.src1 == ReadyDest)
        {
            currentInst.src1ready = true;
        }
        if (currentInst.src2 == ReadyDest)
        {
            currentInst.src2ready = true;
        }
        tempQueue.push(currentInst);
    }
    DIorRR = std::move(tempQueue);
}

void WakeupIssueQ(issueQ *IssueQ, int iq_size, int PCnumToSearch)
{
    for (int i = 0; i < iq_size; ++i)
    {
        if (IssueQ[i].valid)
        {
            if (IssueQ[i].src1 == PCnumToSearch)
            {
                IssueQ[i].src1ready = true; // Set src1ready to true
            }
            if (IssueQ[i].src2 == PCnumToSearch)
            {
                IssueQ[i].src2ready = true; // Set src1ready to true
            }
        }
    }
}

int getNumberofValidIndex(issueQ *IssueQ, int iq_size)
{
    int NumberofValidIndex = 0;
    for (int i = 0; i < iq_size; ++i)
    {
        if (IssueQ[i].src1ready && IssueQ[i].src2ready && IssueQ[i].valid)
        {
            NumberofValidIndex++;
        }
    }
    return NumberofValidIndex;
}

int getNumberofExeValidIndex(executelist *execlist, int width)
{
    int NumberofValidIndex = 0;
    for (int i = 0; i < width * 5; ++i)
    {
        if (execlist[i].valid)
        {
            NumberofValidIndex++;
        }
    }
    return NumberofValidIndex;
}

int getInValidIndex(executelist *execlist, int width)
{

    for (int i = 0; i < width * 5; ++i)
    {
        if (!execlist[i].valid)
        {
            return i;
        }
    }
    return -1;
}

int getOldestValidIndex(issueQ *IssueQ, int iq_size)
{
    int oldestIndex = -1;
    int oldestPrior = INT_MAX;
    for (int i = 0; i < iq_size; ++i)
    {
        if (IssueQ[i].src1ready && IssueQ[i].src2ready && IssueQ[i].prior < oldestPrior && IssueQ[i].valid)
        {
            oldestIndex = i;
            oldestPrior = IssueQ[i].prior;
        }
    }
    return oldestIndex;
}

bool Advance_Cycle(bool end)
{
    clock++;
    return end;
}

bool IsROBEmpty(int robSize, ROB *rob)
{
    for (int i = 0; i < robSize; i++)
    {
        if (rob[i].robpc != 0)
        {
            return false;
        }
    }
    return true;
}

void FetchInstr(FILE *FP, proc_params params, ROB *rob)
{
    int op_type, dest, src1, src2;
    unsigned long pc;
    if (DE.empty())
    {
        for (unsigned long int i = 0; i < params.width; i++)
        {
            if (fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) == EOF)
            {
                if (IsROBEmpty(params.rob_size, rob))
                {
                    end = false;
                    return;
                }
                else
                {
                    end = true;
                    instructions instr;
                    instr.pc = 0;
                    instr.op_type = 0;
                    instr.dest = instr.cycles.DestPrime = 0;
                    instr.src1 = instr.cycles.Src1Prime = 0;
                    instr.src2 = instr.cycles.Src2Prime = 0;
                    instr.cycles.fetch_first = 100000;
                    instr.cycles.fetch_time = 1;
                    instr.cycles.decode_first = clock;
                    DE.push(instr);
                }
            }
            else
            {
                instructions instr;
                instr.pc = pc;
                instr.op_type = instr.cycles.OperationType = op_type;
                instr.dest = instr.cycles.DestPrime = dest;
                instr.src1 = instr.cycles.Src1Prime = src1;
                instr.src2 = instr.cycles.Src2Prime = src2;
                instr.cycles.fetch_first = clock - 1;
                instr.cycles.fetch_time = 1;
                instr.cycles.decode_first = clock;
                DE.push(instr);
                end = true;
            }
        }
    }
    return;
}

void Retire(ROB *rob, int WIDTH, int rob_size)
{
    int retiredCount = 0;
    while (retiredCount < WIDTH && (ROBHEAD != ROBTAIL || rob[ROBHEAD].robpc != 0))
    {
        if (ROBHEAD == ROBTAIL && rob[ROBHEAD].robpc == 0)
        {
            ROBTAIL--;
        }
        if (rob[ROBHEAD].ready)
        {
            rob[ROBHEAD].cycles.retire_time = (clock - rob[ROBHEAD].cycles.retire_first);

            if (rob[ROBHEAD].robpc != 0 && rob[ROBHEAD].cycles.fetch_first != 100000)
            {
                printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} RR{%d,%d} DI{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d} RT{%d,%d}\n", seq, rob[ROBHEAD].cycles.OperationType, rob[ROBHEAD].cycles.Src1Prime, rob[ROBHEAD].cycles.Src2Prime, rob[ROBHEAD].cycles.DestPrime, rob[ROBHEAD].cycles.fetch_first, rob[ROBHEAD].cycles.fetch_time, rob[ROBHEAD].cycles.decode_first, rob[ROBHEAD].cycles.decode_time, rob[ROBHEAD].cycles.rename_first, rob[ROBHEAD].cycles.rename_time, rob[ROBHEAD].cycles.regRead_first, rob[ROBHEAD].cycles.regRead_time, rob[ROBHEAD].cycles.dispatch_first, rob[ROBHEAD].cycles.dispatch_time, rob[ROBHEAD].cycles.issue_first, rob[ROBHEAD].cycles.issue_time, rob[ROBHEAD].cycles.execute_first, rob[ROBHEAD].cycles.execute_time, rob[ROBHEAD].cycles.writeback_first, rob[ROBHEAD].cycles.writeback_time, rob[ROBHEAD].cycles.retire_first, rob[ROBHEAD].cycles.retire_time);
                lastseq = seq + 1;
                lastcycle = rob[ROBHEAD].cycles.retire_first + rob[ROBHEAD].cycles.retire_time;
            }
            rob[ROBHEAD].robpc = 0;
            rob[ROBHEAD].dst = -1;
            rob[ROBHEAD].value = -1;
            rob[ROBHEAD].ready = false;
            rob[ROBHEAD].exception = 0;
            rob[ROBHEAD].miss = false;
            ROBHEAD = (ROBHEAD + 1) % rob_size;
            retiredCount++;
            seq++;
        }
        else
        {
            break;
        }
    }
}

void Writeback(ROB *rob, int rob_size)
{
    while (!WB.empty())
    {
        instructions currentInst = WB.front();
        WB.pop();
        for (int i = 0; i < rob_size; ++i)
        {
            if (rob[i].robindex == currentInst.dest)
            {
                rob[i].ready = true;
                rob[i].cycles = currentInst.cycles;
                rob[i].cycles.writeback_time = (clock - currentInst.cycles.writeback_first);
                rob[i].cycles.retire_first = clock;
                break;
            }
        }
    }
}

void Execute(executelist *execlist, int width, issueQ *IssueQ, int iq_size, ROB *rob, RMT rmt[67])
{
    for (int i = 0; i < width * 5; ++i)
    {
        if (execlist[i].valid) // Check if valid is true
        {
            if (execlist[i].OpCounter == 1)
            {
                instructions instr;
                instr.dest = execlist[i].dest;
                instr.op_type = execlist[i].OPtype;
                instr.pc = execlist[i].PCnum;
                instr.src1 = execlist[i].src1;
                instr.src1ready = true;
                instr.src2ready = true;
                instr.src2 = execlist[i].src2;
                instr.cycles = execlist[i].cycles;
                instr.cycles.execute_time = (clock - execlist[i].cycles.execute_first);
                instr.cycles.writeback_first = clock;
                WB.push(instr);
                execlist[i].valid = false;
                WakeupDIorRR(DI, execlist[i].dest);
                WakeupDIorRR(RR, execlist[i].dest);
                WakeupIssueQ(IssueQ, iq_size, execlist[i].dest);
                int in = execlist[i].dest;
                for (int j = 0; j < 67; j++)
                {
                    if (rmt[j].robtag == in)
                    {
                        rmt[j].valid = false;
                    }
                }
            }
            execlist[i].OpCounter--; // Decrement OpCounter by 1
        }
    }
}

void Issue(issueQ *IssueQ, int width, int iq_size, executelist *execlist)
{
    int issuedInstructions = 0;
    for (int i = 0; i < width; ++i)
    {
        if (issuedInstructions >= width)
        {
            break;
        }
        int j = getOldestValidIndex(IssueQ, iq_size);
        if (IssueQ[j].src1ready && IssueQ[j].src2ready && IssueQ[j].valid && j != -1)
        {
            int executelistindex = getInValidIndex(execlist, width);
            execlist[executelistindex].PCnum = IssueQ[j].PCnum;
            execlist[executelistindex].valid = true;
            execlist[executelistindex].OPtype = IssueQ[j].OPtype;
            execlist[executelistindex].dest = IssueQ[j].dest;
            execlist[executelistindex].src1 = IssueQ[j].src1;
            execlist[executelistindex].src2 = IssueQ[j].src2;
            execlist[executelistindex].src1ready = IssueQ[j].src1ready;
            execlist[executelistindex].src2ready = IssueQ[j].src2ready;
            execlist[executelistindex].OpCounter = (IssueQ[j].OPtype == 0) ? 1 : (IssueQ[j].OPtype == 1) ? 2
                                                                             : (IssueQ[j].OPtype == 2)   ? 5
                                                                                                         : execlist[executelistindex].OpCounter;

            IssueQ[j].valid = false;
            execlist[executelistindex].cycles = IssueQ[j].cycles;
            execlist[executelistindex].cycles.issue_time = (clock - IssueQ[j].cycles.issue_first);
            execlist[executelistindex].cycles.execute_first = clock;
            execlist[executelistindex].valid = true;
            issuedInstructions++;
        }
    }
}

void Dispatch(issueQ *IssueQ, int iq_size)
{
    int countInvalid = 0;
    int IQindex = 0;
    for (int i = 0; i < iq_size; ++i)
    {
        if (!IssueQ[i].valid) // Check if valid is false
        {
            countInvalid++;
        }
    }
    if (!DI.empty() && (countInvalid >= static_cast<int>(DI.size())))
    {
        while (!DI.empty())
        {
            for (int i = 0; i < iq_size; ++i)
            {
                IQindex = i;
                if (!IssueQ[i].valid) // Check if valid is false
                {
                    break;
                }
            }
            IssueQ[IQindex].src1ready = true;
            IssueQ[IQindex].src2ready = true;
            IssueQ[IQindex].valid = true;
            IssueQ[IQindex].dest = DI.front().dest;
            IssueQ[IQindex].src1 = DI.front().src1;
            IssueQ[IQindex].src2 = DI.front().src2;
            IssueQ[IQindex].OPtype = DI.front().op_type;
            IssueQ[IQindex].PCnum = DI.front().pc;
            IssueQ[IQindex].src1ready = DI.front().src1ready;
            IssueQ[IQindex].src2ready = DI.front().src2ready;
            IssueQ[IQindex].cycles = DI.front().cycles;
            IssueQ[IQindex].cycles.dispatch_time = (clock - DI.front().cycles.dispatch_first);
            IssueQ[IQindex].cycles.issue_first = clock;
            IssueQ[IQindex].prior = priority;
            priority++;
            DI.pop(); // Remove the front element from RR
        }
    }
}

void RegRead()
{
    if (DI.empty() && !RR.empty())
    {
        while (!RR.empty())
        {
            RR.front().cycles.regRead_time = (clock - RR.front().cycles.regRead_first);
            RR.front().cycles.dispatch_first = clock;
            DI.push(RR.front());
            RR.pop();
        }
    }
}

void Rename(unsigned long int ROBSIZE, unsigned long int WIDTH, RMT rmt[67], ROB *rob)
{
    unsigned long int freeEntries = (ROBTAIL >= ROBHEAD && rob[ROBHEAD].robpc == 0) ? (ROBSIZE - (ROBTAIL - ROBHEAD)) : (ROBHEAD - ROBTAIL);
    if (RR.empty() && (freeEntries >= WIDTH) && !RN.empty())
    {
        while (!RN.empty())
        {
            // Step1, checking for source registers rrom the RMT table
            RN.front().src1ready = true;
            RN.front().src2ready = true;

            if (rmt[RN.front().src1].valid == true && RN.front().src1 != -1)
            {
                RN.front().src1 = rmt[RN.front().src1].robtag;
                RN.front().src1ready = false;
            }
            if (rmt[RN.front().src2].valid == true && RN.front().src2 != -1)
            {
                RN.front().src2 = rmt[RN.front().src2].robtag;
                RN.front().src2ready = false;
            }
            // Step2: Allocate new rob from the robtable to  the RMT table
            if (RN.front().dest != -1)
            {
                rob[ROBTAIL].robpc = RN.front().pc;
                rob[ROBTAIL].dst = RN.front().dest;
                rob[ROBTAIL].ready = false;
                rob[ROBTAIL].exception = 0;
                rob[ROBTAIL].miss = false;
                rob[ROBTAIL].exception = 0;
                rob[ROBTAIL].value = -1;
                rmt[RN.front().dest].robtag = rob[ROBTAIL].robindex;
                rmt[RN.front().dest].valid = true;
                RN.front().dest = rmt[RN.front().dest].robtag;
            }
            else
            {
                rob[ROBTAIL].robpc = RN.front().pc;
                rob[ROBTAIL].dst = -1;
                rob[ROBTAIL].ready = false;
                rob[ROBTAIL].exception = 0;
                rob[ROBTAIL].miss = false;
                rob[ROBTAIL].value = -1;
                RN.front().dest = rob[ROBTAIL].robindex;
            }

            RN.front().cycles.rename_time = (clock - RN.front().cycles.rename_first);
            RN.front().cycles.regRead_first = clock;
            RR.push(RN.front()); // Push the front element of DE to RN
            RN.pop();            // Remove the front element from DE
            ROBTAIL = (ROBTAIL + 1) % ROBSIZE;
        }
    }
}

void Decode()
{
    if (RN.empty() && !DE.empty())
    {
        while (!DE.empty())
        {
            DE.front().cycles.decode_time = (clock - DE.front().cycles.decode_first);
            DE.front().cycles.rename_first = clock;
            RN.push(DE.front()); // Push the front element of DE to RN
            DE.pop();            // Remove the front element from DE
        }
    }
}

void Fetch(FILE *FP, proc_params params, ROB *rob)
{
    FetchInstr(FP, params, rob);
}
/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main(int argc, char *argv[])
{
    FILE *FP;           // File handler
    char *trace_file;   // Variable that holds trace file name;
    proc_params params; // look at sim_bp.h header file for the the definition of struct proc_params
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }
    params.rob_size = strtoul(argv[1], NULL, 10);
    params.iq_size = strtoul(argv[2], NULL, 10);
    params.width = strtoul(argv[3], NULL, 10);
    trace_file = argv[4];
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if (FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    RMT rmt[67];
    for (int i = 0; i < 67; ++i)
    {
        rmt[i].valid = false; // Initialize valid bit to false
        rmt[i].robtag = -1;   // Initialize tag to a default value, e.g., -1
    }

    ROB *rob = new ROB[params.rob_size];
    for (unsigned long int i = 0; i < params.rob_size; ++i)
    {
        rob[i].ready = false;
        rob[i].value = -1;
        rob[i].dst = -1;
        rob[i].robindex = 100 + i;
        rob[i].robpc = 0;
    }

    issueQ *IssueQ = new issueQ[params.iq_size];
    for (unsigned long int i = 0; i < params.iq_size; ++i)
    {
        IssueQ[i].valid = false;
        IssueQ[i].dest = -1;
        IssueQ[i].src1ready = false;
        IssueQ[i].src1 = -1;
        IssueQ[i].src2ready = false;
        IssueQ[i].src2 = -1;
        IssueQ[i].PCnum = 0;
    }
    executelist *execlist = new executelist[params.width * 5];
    for (unsigned long int i = 0; i < params.width * 5; ++i)
    {
        execlist[i].valid = false;
        execlist[i].dest = -1;
        execlist[i].src1ready = false;
        execlist[i].src1 = -1;
        execlist[i].src2ready = false;
        execlist[i].src2 = -1;
        execlist[i].PCnum = 0;
        execlist[i].OpCounter = 0;
    }

    do
    {
        Retire(rob, params.width, params.rob_size);                        // Retire instructions from ROB
        Writeback(rob, params.rob_size);                                   // Mark instructions as ready in ROB
        Execute(execlist, params.width, IssueQ, params.iq_size, rob, rmt); // Check instructions finishing execution
        Issue(IssueQ, params.width, params.iq_size, execlist);             // Issue oldest ready instructions from IQ
        Dispatch(IssueQ, params.iq_size);                                  // Dispatch instructions to IQ
        RegRead();                                                         // Read registers and update DI
        Rename(params.rob_size, params.width, rmt, rob);                   // Rename registers and allocate ROB entries
        Decode();                                                          // Decode fetched instructions
        Fetch(FP, params, rob);                                            // Fetch new instructions from the trace

    } while (Advance_Cycle(end));

    printf("# === Simulator Command =========\n");
    printf("# ./sim %d %d %d %s\n", params.rob_size, params.iq_size, params.width, trace_file); // Replace with appropriate variables
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %d\n", params.rob_size);
    printf("# IQ_SIZE  = %d\n", params.iq_size);
    printf("# WIDTH    = %d\n", params.width);
    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count    = %d\n", lastseq);
    printf("# Cycles                       = %d\n", lastcycle);
    float ipc = (float)lastseq / (float)(lastcycle);
    printf("# Instructions Per Cycle (IPC) = %.2f\n", ipc);
    return 0;
}
