// Name: Juan Cota
// Date: 10/22/2024
// RedID: 827272362

#include <iostream>
#include <vector>
#include "pageTableLevel.h"

using namespace std;

// Define the TLB vector
vector<TLB> tlb;

PageTable::PageTable(unsigned int levelCount, unsigned int* bitMaskAry, unsigned int* shiftAry, unsigned int* entryCount) {
    this->levelCount = levelCount;
    this->bitMaskAry = bitMaskAry;
    this->shiftAry = shiftAry;
    this->entryCount = entryCount;
    rootNodePtr = new Level(0, this);  // Initialize the root level
}

unsigned int PageTable::recordPageAccess(unsigned int address) {
    return rootNodePtr->recordPageAccess(address);
}

Level::Level(unsigned int depth, PageTable* pageTable) {
    this->depth = depth;
    this->pageTablePtr = pageTable;
    this->numOfAccesses = 0;
    this->nextLevelPtr = new Level*[pageTable->entryCount[depth]];
    this->map = nullptr;
    for (unsigned int i = 0; i < pageTable->entryCount[depth]; i++) {
        nextLevelPtr[i] = nullptr;
    }
}

unsigned int Level::recordPageAccess(unsigned int address) {
    unsigned int pageIndex = extractPageNumberFromAddress(address, pageTablePtr->bitMaskAry[depth], pageTablePtr->shiftAry[depth]);
    if (depth == pageTablePtr->levelCount) {
        numOfAccesses++;
        return numOfAccesses;
    }
    if (nextLevelPtr[pageIndex] == nullptr) {
        nextLevelPtr[pageIndex] = new Level(depth + 1, pageTablePtr);
    }
    return nextLevelPtr[pageIndex]->recordPageAccess(address);
}

unsigned int extractPageNumberFromAddress(unsigned int address, unsigned int mask, unsigned int shift) {
    address = address & mask; // Applies the mask
    return address >> shift; // Shifts and returns the address
}

void insert_vpn2pfn(PageTable* pageTable, unsigned int virtualAddress, unsigned int frame) {
    // Implementation here
}

int lookup_TLB(unsigned int vpn, unsigned int& pfn) {
    for (int i = 0; i < tlb.size(); i++) {
        if (tlb[i].vpn == vpn) {
            pfn = tlb[i].pfn;
            tlb[i].lruCounter = 0; // Reset LRU counter for accessed entry
            return 1; // TLB hit
        }
    }
    return 0; // TLB miss
}

void insert_TLB(unsigned int vpn, unsigned int pfn) {
    int lruIndex = 0;
    for (int i = 1; i < tlb.size(); i++) {
        if (tlb[i].lruCounter > tlb[lruIndex].lruCounter) {
            lruIndex = i;
        }
    }
    tlb[lruIndex].vpn = vpn;
    tlb[lruIndex].pfn = pfn;
    tlb[lruIndex].lruCounter = 0;
}

void update_LRU() {
    for (int i = 0; i < tlb.size(); i++) {
        tlb[i].lruCounter++;
    }
}