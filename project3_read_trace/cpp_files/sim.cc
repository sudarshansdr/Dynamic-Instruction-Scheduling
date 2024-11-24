#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <queue>
#include <cmath>
#include "sim.h"
bool end = true;
int ROBHEAD = 0;
int ROBTAIL = 0;
int priority = 0;

void WakeupDIorRR(std::queue<instructions> &DIorRR, unsigned long ReadyDest)
{
    std::queue<instructions> tempQueue; // Temporary queue to hold updated instructions

    while (!DIorRR.empty())
    {
        instructions currentInst = DIorRR.front(); // Get the front instruction
        DIorRR.pop();                              // Remove it from the original queue

        // Check if the pc matches the target PC
        if (currentInst.src1 == ReadyDest)
        {
            currentInst.src1ready = true; // Set src1ready to true
        }
        if (currentInst.src2 == ReadyDest)
        {
            currentInst.src2ready = true; // Set src1ready to true
        }

        tempQueue.push(currentInst); // Push the instruction to the temporary queue
    }

    // Replace the original queue with the updated queue
    DIorRR = std::move(tempQueue);
}

void WakeupIssueQ(issueQ *IssueQ, int iq_size, int PCnumToSearch)
{
    for (int i = 0; i < iq_size; ++i)
    {
        if (IssueQ[i].valid && IssueQ[i].PCnum == PCnumToSearch)
        {
            IssueQ[i].src1ready = true; // Set src1ready to true
            IssueQ[i].src2ready = true; // Set src2ready to true
            break;                      // Exit loop after finding and updating the matching PCnum
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

int getNumberofValidIndex(executelist *execlist, int width)
{
    int NumberofInValidIndex = 0;
    for (int i = 0; i < width * 5; ++i)
    {
        if (execlist[i].valid)
        {
            NumberofInValidIndex++;
        }
    }
    return NumberofInValidIndex;
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
    return end;
}

void FetchInstr(FILE *FP, proc_params params)
{
    int op_type, dest, src1, src2;
    unsigned long pc;
    if (DE.empty() || fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) == EOF)
    {
        for (int i = 0; i < params.width; i++)
        {
            if (fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) == EOF)
            {
                end = false;
                return;
            }
            else
            {
                instructions instr;
                instr.pc = pc;
                instr.op_type = op_type;
                instr.dest = dest;
                instr.src1 = src1;
                instr.src2 = src2;
                DE.push(instr);
                end = true;
            }
        }
    }
    return;
}

void Retire(ROB *rob, int WIDTH, int rob_size)
{
    int retiredCount = 0; // Counter for retired instructions

    // Loop to retire up to WIDTH instructions
    while (retiredCount < WIDTH && ROBHEAD != ROBTAIL)
    {
        // Check if the instruction at the ROBHEAD is ready
        if (rob[ROBHEAD].ready)
        {
            rob[ROBHEAD].robpc = 0;
            rob[ROBHEAD].dst = -1;
            rob[ROBHEAD].value = -1;
            rob[ROBHEAD].ready = false;
            rob[ROBHEAD].exception = 0;
            rob[ROBHEAD].miss = false;
            ROBHEAD = (ROBHEAD + 1) % rob_size;
            retiredCount++;
        }
        else
        {
            // If the instruction is not ready, stop retiring further instructions
            break;
        }
    }
}

void Writeback(ROB *rob, int rob_size)
{
    std::queue<instructions> tempWB = WB;
    while (!tempWB.empty())
    {
        instructions currentInst = tempWB.front();
        tempWB.pop();
        for (int i = 0; i < rob_size; ++i)
        {
            if (rob[i].dst == currentInst.dest)
            {
                rob[i].ready = true;
                break;
            }
        }
    }
}

void Execute(executelist *execlist, int width, issueQ *IssueQ, int iq_size)
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
                WB.push(instr);
                execlist[i].valid = false;

                WakeupDIorRR(DI, execlist[i].dest);
                WakeupDIorRR(RR, execlist[i].dest);
                WakeupIssueQ(IssueQ, iq_size, execlist[i].PCnum);
            }
            --execlist[i].OpCounter; // Decrement OpCounter by 1
        }
    }
}

void Issue(issueQ *IssueQ, int width, int iq_size, executelist *execlist)
{
    int N = getNumberofValidIndex(IssueQ, iq_size);
    int bundleN = N / width;
    int availsize = width * 5 - getNumberofValidIndex(execlist, width);
    int availbundle = availsize / width;
    int Gonnapassbundle = std::abs(bundleN - availbundle);
    if (availbundle >= 1)
    {
        if (N >= width)
        {
            for (int i = 0; i < width * Gonnapassbundle; ++i)
            {
                int j = getOldestValidIndex(IssueQ, iq_size);
                if (IssueQ[j].src1ready && IssueQ[j].src2ready && IssueQ[j].valid && j != -1)
                {
                    int executelistindex = getInValidIndex(execlist, width);
                    execlist[executelistindex].PCnum = IssueQ[j].PCnum;
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
                    execlist[executelistindex].valid = true;
                }
            }
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
    if (!DI.empty() && countInvalid >= DI.size())
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
            DI.push(RR.front());
            RR.pop();
        }
    }
}

void Rename(unsigned long int ROBSIZE, RMT rmt[67], ROB *rob)
{

    if (RR.empty() && (ROBTAIL - ROBHEAD != ROBSIZE) && !RN.empty())
    {
        while (!RN.empty())
        {
            // Step1, checking for source registers rrom the RMT table
            RN.front().src1ready = true;
            RN.front().src2ready = true;
            if (rmt[RN.front().src1].valid == true)
            {
                RN.front().src1 = rmt[RN.front().src1].robtag;
                RN.front().src1ready = false;
            }
            if (rmt[RN.front().src2].valid == true)
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
            }
            RR.push(RN.front()); // Push the front element of DE to RN
            RN.pop();            // Remove the front element from DE
            ROBTAIL++;
        }
    }
}

void Decode()
{
    if (RN.empty() && !DE.empty())
    {
        while (!DE.empty())
        {
            RN.push(DE.front()); // Push the front element of DE to RN
            DE.pop();            // Remove the front element from DE
        }
    }
}

void Fetch(FILE *FP, proc_params params)
{
    // Variables are read from trace file
    FetchInstr(FP, params);
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
    FILE *FP;                      // File handler
    char *trace_file;              // Variable that holds trace file name;
    proc_params params;            // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2; // Variables are read from trace file
    uint64_t pc;                   // Variable holds the pc read from input file

    argv[0] = strdup("C:\\NCSU\\563\\Project 3\\Dynamic-Instruction-Scheduling\\project3_read_trace\\cpp_files\\sim.cc");
    argv[1] = strdup("16");
    argv[2] = strdup("8");
    argv[3] = strdup("1");
    argv[4] = strdup("C:\\NCSU\\563\\Project 3\\Dynamic-Instruction-Scheduling\\project3_read_trace\\cpp_files\\gcc_trace.txt");
    argc = 5;

    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    params.rob_size = strtoul(argv[1], NULL, 10);
    params.iq_size = strtoul(argv[2], NULL, 10);
    params.width = strtoul(argv[3], NULL, 10);
    trace_file = argv[4];
    printf("rob_size:%lu "
           "iq_size:%lu "
           "width:%lu "
           "tracefile:%s\n",
           params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if (FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    // while (fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //     printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); // Print to check if inputs have been read correctly

    RMT rmt[67];
    for (int i = 0; i < 67; ++i)
    {
        rmt[i].valid = false; // Initialize valid bit to false
        rmt[i].robtag = -1;   // Initialize tag to a default value, e.g., -1
    }

    ROB *rob = new ROB[params.rob_size];
    for (int i = 0; i < params.rob_size; ++i)
    {
        rob[i].ready = false;
        rob[i].value = -1;
        rob[i].dst = -1;
        rob[i].robindex = 100 + i;
        rob[i].robpc = 0;
    }

    issueQ *IssueQ = new issueQ[params.iq_size];
    for (int i = 0; i < params.iq_size; ++i)
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
    for (int i = 0; i < params.width * 5; ++i)
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
        Retire(rob, params.width, params.rob_size);              // Retire instructions from ROB
        Writeback(rob, params.rob_size);                         // Mark instructions as ready in ROB
        Execute(execlist, params.width, IssueQ, params.iq_size); // Check instructions finishing execution
        Issue(IssueQ, params.width, params.iq_size, execlist);   // Issue oldest ready instructions from IQ
        Dispatch(IssueQ, params.iq_size);                        // Dispatch instructions to IQ
        RegRead();                                               // Read registers and update DI
        Rename(params.rob_size, rmt, rob);                       // Rename registers and allocate ROB entries
        Decode();                                                // Decode fetched instructions
        Fetch(FP, params);                                       // Fetch new instructions from the trace

    } while (Advance_Cycle(end));

    while (!DE.empty())
    {
        instructions current = DE.front();
        DE.pop();

        printf("DEpc: %lu, op_type: %d, dest: %d, src1: %d, src2: %d\n",
               current.pc, current.op_type, current.dest, current.src1, current.src2);
    }
    while (!RN.empty())
    {
        instructions current = RN.front();
        RN.pop();

        printf("RNpc2: %lu, op_type: %d, dest: %d, src1: %d, src2: %d\n",
               current.pc, current.op_type, current.dest, current.src1, current.src2);
    }

    while (!RR.empty())
    {
        instructions current = RR.front();
        RR.pop();

        printf("RRpc3: %lu, op_type: %d, dest: %d, src1: %d, src2: %d\n",
               current.pc, current.op_type, current.dest, current.src1, current.src2);
    }

    while (!DI.empty())
    {
        instructions current = DI.front();
        DI.pop();

        printf("DIpc4: %lu, op_type: %d, dest: %d, src1: %d, src2: %d\n",
               current.pc, current.op_type, current.dest, current.src1, current.src2);
    }

    for (int i = 0; i < params.iq_size; ++i)
    {
        printf("IQ[%d]: pc = %x, dst = %d, src1 = %d, src1ready? = %d, src2 = %d, src2ready? = %d, Valid = %d\n", i, IssueQ[i].PCnum, IssueQ[i].dest, IssueQ[i].src1, IssueQ[i].src1ready, IssueQ[i].src2, IssueQ[i].src2ready, IssueQ[i].valid);
    }

    for (int i = 0; i < params.width * 5; ++i)
    {
        printf("ExecuteList[%d]: pc = %x, dst = %d, src1 = %d, src1ready? = %d, src2 = %d, src2ready? = %d,Valid = %d,optype = %d, Counter = %d\n", i, execlist[i].PCnum, execlist[i].dest, execlist[i].src1, execlist[i].src1ready, execlist[i].src2, execlist[i].src2ready, execlist[i].valid, execlist[i].OPtype, execlist[i].OpCounter);
    }

    for (int i = 0; i < 67; ++i)
    {
        printf("rmt[%d]: valid = %d, robtag = %d\n", i, rmt[i].valid, rmt[i].robtag);
    }

    for (int i = 0; i < params.rob_size; ++i)
    {
        printf("rob[%d]: dst = %d, robindex = %d, pcindex = %lu\n", i, rob[i].dst, rob[i].robindex, rob[i].robpc);
    }

    return 0;
}
