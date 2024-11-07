#ifndef SRC_ALGORITHMS_CLOMAPENTRY_H_
#define SRC_ALGORITHMS_CLOMAPENTRY_H_

#include "../data/database.h"

struct CloMapEntry {
    CloMapEntry(const Itemset& items, int supp, int hash = 0)
        : fItems(items), fSupp(supp), fHash(hash) {
        same_closure = nullptr;
        minimal_superset = nullptr;
        subsets.clear();
    }

    CloMapEntry(const CloMapEntry& other){
        fItems = other.fItems;
        fGenerators = other.fGenerators;
        fSupp = other.fSupp;
        fHash = other.fHash;
        same_closure = nullptr;
        minimal_superset = nullptr;
        subsets.clear();
    }


    std::vector<int> fItems;
    std::vector<std::vector<int>> fGenerators;
    // std::vector<int> fGenerator;
	int fSupp;
    int fHash;

    CloMapEntry* same_closure;
    CloMapEntry* minimal_superset;
    std::vector<CloMapEntry*> subsets;
};

#endif //SRC_ALGORITHMS_CLOMAPENTRY_H_