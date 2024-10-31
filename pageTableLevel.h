#ifndef PAGETABLELEVEL_H
#define PAGETABLELEVEL_H
#include <vector>

#define TLB_CACHE_SIZE 0 // Default TLB Cache Size

struct Map {
    unsigned int vpn;
    unsigned int pfn;
    Map(unsigned int vpn, unsigned int pfn) : vpn(vpn), pfn(pfn) {}
};

struct TLB {
    unsigned int vpn;
    unsigned int pfn;
    unsigned int lruCounter;
};

extern std::vector<TLB> tlb;

class PageTable {
public:
    unsigned int levelCount;
    unsigned int* bitMaskAry;
    unsigned int* shiftAry;
    unsigned int* entryCount;
    class Level* rootNodePtr;
    PageTable(unsigned int levelCount, unsigned int* bitMaskAry, unsigned int* shiftAry, unsigned int* entryCount);
    unsigned int recordPageAccess(unsigned int address);
    
};

class Level {
public:
    unsigned int depth;
    PageTable* pageTablePtr;
    unsigned int numOfAccesses;
    Level** nextLevelPtr;
    Map* map;
    Level(unsigned int depth, PageTable* pageTable);
    unsigned int recordPageAccess(unsigned int address);
};

unsigned int extractPageNumberFromAddress(unsigned int address, unsigned int mask, unsigned int shift);
void insert_vpn2pfn(PageTable* pageTable, unsigned int virtualAddress, unsigned int frame);
int lookup_TLB(unsigned int vpn, unsigned int& pfn);
void insert_TLB(unsigned int vpn, unsigned int pfn);
void update_LRU();

Map* lookup_vpn2pfn(PageTable* pageTable, unsigned int virtualAddress);
#endif
