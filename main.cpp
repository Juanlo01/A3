#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>  // For getopt
#include <numeric>  // For accumulate
#include "pageTableLevel.h"
#include "log.h"
#include "tracereader.h"

#define BADEXIT 1

using namespace std;

// Declare the TLB vector as extern
extern vector<TLB> tlb;

void printUsage() {
    cerr << "Usage: ./pagingwithatc [-n N] [-c N] [-o mode] <trace_file> <level_bits>" << endl;
}

int main(int argc, char **argv) {
    int opt;
    unsigned int maxAccesses = 0;
    int tlbSize = 0;  // Default to no cache
    string outputMode = "summary";

    // Parse optional arguments
    while ((opt = getopt(argc, argv, "n:c:o:")) != -1) {
        switch (opt) {
            case 'n':
                maxAccesses = atoi(optarg);
                if (maxAccesses <= 0) {
                    cerr << "Number of memory accesses must be a number, greater than 0" << endl;
                    return BADEXIT;
                }
                break;
            case 'c':
                tlbSize = atoi(optarg);
                if (tlbSize < 0) {
                    cerr << "Cache capacity must be a number, greater than or equal to 0" << endl;
                    return BADEXIT;
                }
                if (tlbSize > 0) {
                    tlb.resize(tlbSize);
                }
                break;
            case 'o':
                outputMode = string(optarg);
                break;
            default:
                printUsage();
                return BADEXIT;
        }
    }

    if (optind >= argc) {
        printUsage();
        return BADEXIT;
    }

    char *traceFileName = argv[optind++];
    FILE *traceFile = fopen(traceFileName, "rb");
    if (!traceFile) {
        cerr << "Unable to open <<" << traceFileName << ">>" << endl;
        return BADEXIT;
    }

    vector<unsigned int> levelBits;

    // Parse level bits from remaining arguments
    while (optind < argc) {
        unsigned int bit;
        stringstream levelStream(argv[optind]);
        while (levelStream >> bit) {
            if (bit < 1) {
                cerr << "Level " << levelBits.size() << " page table must be at least 1 bit" << endl;
                fclose(traceFile);
                return BADEXIT;
            }
            levelBits.push_back(bit);
        }
        optind++;
    }

    unsigned int levelCount = levelBits.size();
    if (levelCount == 0 || accumulate(levelBits.begin(), levelBits.end(), 0) > 28) {
        cerr << "Too many bits used in page tables" << endl;
        fclose(traceFile);
        return BADEXIT;
    }

    vector<unsigned int> bitMaskAry(levelCount);
    vector<unsigned int> shiftAry(levelCount);
    vector<unsigned int> entryCount(levelCount);
    unsigned int totalBits = 32;
    unsigned int shift = totalBits;
    for (unsigned int i = 0; i < levelCount; i++) {
        shift -= levelBits[i];
        bitMaskAry[i] = ((1U << levelBits[i]) - 1) << shift;
        shiftAry[i] = shift;
        entryCount[i] = 1U << levelBits[i];
    }

    PageTable pageTable(levelCount, bitMaskAry.data(), shiftAry.data(), entryCount.data());

    if (outputMode == "bitmasks") {
        log_bitmasks(levelCount, bitMaskAry.data());
        fclose(traceFile);
        return 0;
    }

    p2AddrTr traceAddr;
    unsigned int numOfAccesses = 0;
    vector<unsigned int> pageIndices(levelCount);
    while ((maxAccesses == 0 || numOfAccesses < maxAccesses) && NextAddress(traceFile, &traceAddr)) {
        unsigned int address = traceAddr.addr;
        unsigned int vpn = address >> 12; // Example shift for VPN extraction
        unsigned int pfn;

        bool tlbhit = false;
        if (tlbSize > 0) {
            if (lookup_TLB(vpn, pfn)) {
                tlbhit = true; // TLB hit
            } else {
                // TLB miss, walk page table...
                pfn = pageTable.recordPageAccess(address);
                insert_TLB(vpn, pfn); // Insert into TLB
            }
            update_LRU();
        } else {
            // No TLB, directly walk page table
            pfn = pageTable.recordPageAccess(address);
        }

        if (outputMode == "va2pa") {
            log_virtualAddr2physicalAddr(address, pfn);
        } else if (outputMode == "vpn2pfn") {
            log_pagemapping(levelCount, pageIndices.data(), pfn);
        } else if (outputMode == "offset") {
            hexnum(address & ((1U << shiftAry[levelCount - 1]) - 1));
        } else if (outputMode == "va2pa_atc_ptwalk") {
            bool pthit = !tlbhit; // Assuming a page table hit if not a TLB hit (needs actual logic)
            log_va2pa_ATC_PTwalk(address, pfn, tlbhit, pthit); // Call your logging function here
        }

        numOfAccesses++;
    }

    if (outputMode == "summary") {
        // Implement the TLB simulation and log_summary
        // This should be completed based on your specific simulation logic.
        log_summary(4096, 0, 0, numOfAccesses, 0, 0); // Placeholder values, replace with your actual stats
    }

    fclose(traceFile);
    return 0;
}
