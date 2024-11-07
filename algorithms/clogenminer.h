#ifndef SRC_ALGORITHMS_CLOGENMINER_H_
#define SRC_ALGORITHMS_CLOGENMINER_H_

#include "minernode.h"
#include "genmapentry.h"
#include "clomapentry.h"
#include "../util/hashstorer.h"
#include "../data/database.h"
#include <list>

class CloGenMiner {
public:
    CloGenMiner(Database&, int, int);
	void run();
    int getMinSupp();
    int getMaxSize();
    std::unordered_map<int, DbToken> get_used_int_to_token_map();
    std::unordered_map<HashStorer<Itemset>, CloMapEntry*> get_closures();

protected:
    void mine(const Itemset&, const std::vector<MinerNode>&, const Itemset&);
    void print_after_mine();
    void gen_to_clo();
    void print();

    std::vector<MinerNode> getSingletons(int) const;
    void sortNodes(std::vector<MinerNode>&) const;
    std::unordered_map<int,TidList> bucketTids(const std::vector<MinerNode>&, uint, const TidList&) const;

    GenMapEntry* addMinGen(const GenMapEntry&);
    std::list<GenMapEntry*> getMinGens(const Itemset&, int, int);

private:
    Database& fDb;
    std::unordered_map<int, std::list<GenMapEntry>> fGenMap;
    std::unordered_map<HashStorer<Itemset>, GenMapEntry*> fGenerators;
    std::unordered_map<HashStorer<Itemset>, CloMapEntry*> fClosures;
	int fMinSupp;
    int fMaxSize;
};
#endif //SRC_ALGORITHMS_CLOGENMINER_H_
