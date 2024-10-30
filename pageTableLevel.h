// Name: Juan Cota
// Date: 10/22/2024
// RedID: 827272362

#ifndef PAGETABLELEVEL_H
#define PAGETABLELEVEL_H

#include <vector>

#define TLB_CACHE_SIZE 0 // Default TLB Cache Size

struct Map {
    unsigned int vpn;
    unsigned int pfn;
    // Constructor for easy initialization
    Map(unsigned int vpn, unsigned int pfn) : vpn(vpn), pfn(pfn) {}
};

struct TLB {
    unsigned int vpn;
    unsigned int pfn;
    unsigned int lruCounter;
};

// Declare the TLB vector as extern
extern std::vector<TLB> tlb;

// PageTable: Contains information about the tree
class PageTable {
public:
    unsigned int levelCount; // Number of levels
    unsigned int* bitMaskAry; // Array of bit masks for each level
    unsigned int* shiftAry; // Array of shifts for each level
    unsigned int* entryCount; // Array for the number of entries per level
    class Level* rootNodePtr; // Pointer to the root level

    PageTable(unsigned int levelCount, unsigned int* bitMaskAry, unsigned int* shiftAry, unsigned int* entryCount);
    unsigned int recordPageAccess(unsigned int address);
    Map* lookup_vpn2pfn(PageTable* pageTable, unsigned int virtualAddress);
};

// Level: The page tree node representation. A structure describing a specific level of the page table.
class Level {
public:
    unsigned int depth; // Current depth
    PageTable* pageTablePtr; // Pointer back to the PageTable structure
    unsigned int numOfAccesses; // Number of accesses to this level
    Level** nextLevelPtr; // Array of pointers to the next level
    Map* map;

    Level(unsigned int depth, PageTable* pageTable);
    unsigned int recordPageAccess(unsigned int address);
};

unsigned int extractPageNumberFromAddress(unsigned int address, unsigned int mask, unsigned int shift);
void insert_vpn2pfn(PageTable* pageTable, unsigned int virtualAddress, unsigned int frame);
int lookup_TLB(unsigned int vpn, unsigned int& pfn);
void insert_TLB(unsigned int vpn, unsigned int pfn);
void update_LRU();

#endif