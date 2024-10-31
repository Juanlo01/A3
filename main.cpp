// Name: Juan Cota
// Date: 10/22/2024
// RedID: 827272362

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
unsigned int frameCounter = 0; // Initialize frame counter
unsigned int pageTableHits = 0;
unsigned int tlbHits = 0;
unsigned int pageSize = 0;

void printUsage() {
    cerr << "Usage: ./pagingwithatc [-n N] [-c N] [-o mode] <trace_file> <level_bits>" << endl;
}

unsigned long countPageTableEntries(Level* level) {
    if (!level) return 0;
    unsigned long count = 1; // Count the current level

    for (unsigned int i = 0; i < level->pageTablePtr->entryCount[level->depth]; ++i) {
        if (level->nextLevelPtr[i]) {
            count += countPageTableEntries(level->nextLevelPtr[i]);
        }
    }

    return count;
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

    pageSize = 1 << shift; // Calculate page size based on remaining bits

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
        unsigned int vpn = address >> shiftAry[levelCount - 1]; // Use the offset bits
        unsigned int pfn;
        bool tlbhit = false;
        bool pthit = false;

        // TLB Lookup
        if (tlbSize > 0) {
            if (lookup_TLB(vpn, pfn)) {
                tlbhit = true; // TLB hit
                pthit = true; // Assume page table hit if TLB hit
            } else {
                // TLB miss, perform page table lookup
                Map* mapEntry = lookup_vpn2pfn(&pageTable, vpn);
                if (mapEntry) {
                    pfn = mapEntry->pfn;
                    pthit = true; // Page table hit
                } else {
                    insert_vpn2pfn(&pageTable, vpn, frameCounter++);
                    mapEntry = lookup_vpn2pfn(&pageTable, vpn);
                    pfn = mapEntry->pfn;
                    pthit = false; // Page table miss
                }
                insert_TLB(vpn, pfn); // Insert into TLB
            }
            update_LRU(); // Update TLB LRU
        } else {
            // No TLB, directly walk page table
            Map* mapEntry = lookup_vpn2pfn(&pageTable, vpn);
            if (mapEntry) {
                pfn = mapEntry->pfn;
                pthit = true; // Page table hit
            } else {
                insert_vpn2pfn(&pageTable, vpn, frameCounter++);
                mapEntry = lookup_vpn2pfn(&pageTable, vpn);
                pfn = mapEntry->pfn;
                pthit = false; // Page table miss
            }
        }

        // Compute the physical address
        unsigned int offset = address & ((1U << shiftAry[levelCount - 1]) - 1);
        unsigned int physicalAddress = (pfn << shiftAry[levelCount - 1]) | offset;

        // Log the virtual to physical address translation
        if (outputMode == "va2pa") {
            log_virtualAddr2physicalAddr(address, physicalAddress);
        } else if (outputMode == "vpn2pfn") {
            p2AddrTr traceAddr;
    unsigned int numOfAccesses = 0;
    while ((maxAccesses == 0 || numOfAccesses < maxAccesses) && NextAddress(traceFile, &traceAddr)) {
        unsigned int address = traceAddr.addr;
        unsigned int vpn = address >> shiftAry[levelCount - 1]; // Adjust for correct VPN extraction

        // Insert and lookup to get the correct frame mapping
        Map* mapEntry = lookup_vpn2pfn(&pageTable, vpn);
        if (!mapEntry) {
            insert_vpn2pfn(&pageTable, vpn, frameCounter++);
            mapEntry = lookup_vpn2pfn(&pageTable, vpn);
        }

        // Extract page indices for each level
        for (unsigned int i = 0; i < levelCount; i++) {
            pageIndices[i] = (address & bitMaskAry[i]) >> shiftAry[i];
        }

        // Log the page mapping
        log_pagemapping(levelCount, pageIndices.data(), mapEntry->pfn);
        numOfAccesses++;
    
    }
        } else if (outputMode == "offset") {
            hexnum(offset);
        } else if (outputMode == "va2pa_atc_ptwalk") {
            log_va2pa_ATC_PTwalk(address, physicalAddress, tlbhit, pthit);
        }

        numOfAccesses++;
    }

    if (outputMode == "summary") {
        unsigned long totalPageTableEntries = countPageTableEntries(pageTable.rootNodePtr);
        log_summary(pageSize, tlbHits, pageTableHits, numOfAccesses, frameCounter, totalPageTableEntries);
    }

    fclose(traceFile);
    return 0;
}
