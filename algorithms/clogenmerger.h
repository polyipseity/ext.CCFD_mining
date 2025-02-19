#ifndef SRC_ALGORITHMS_CLOGENMERGER_H_
#define SRC_ALGORITHMS_CLOGENMERGER_H_

#include "minernode.h"
#include "clogenminer.h"
#include "genmapentry.h"
#include "clomapentry.h"
#include "ccfd.h"
#include "../util/hashstorer.h"
#include "../data/database.h"
#include <list>

class CloGenMerger {
public:
    CloGenMerger();
    CloGenMerger(CloGenMiner&);
    CloGenMerger(CloGenMerger&);
    CloGenMerger(std::unordered_map<HashStorer<Itemset>, CloMapEntry*>, std::unordered_map<int, DbToken>, std::unordered_map<DbToken, int>);
    void run_merge(CloGenMerger&);
    void print_closures(int);
    void compare(CloGenMerger&, int);
    void print_clogen_info(int);
    void print_ccfd(std::string filename, int);
    std::vector<ccfd> ccfd_mine(int, int);
    
protected:
    void merge(CloGenMerger&);
    void update_int_to_token_map(CloGenMerger&);
    void sync_token_to_int_map();
    void clo_to_gen();

private:
    std::unordered_map<int, DbToken> fIntToTokenMap;
    std::unordered_map<DbToken, int> fTokenToIntMap;
    std::unordered_map<HashStorer<Itemset>, GenMapEntry*> fGenerators;
    std::unordered_map<HashStorer<Itemset>, CloMapEntry*> fClosures;
    std::unordered_map<HashStorer<Itemset>, CloMapEntry*> new_fClosures;
    int fMinSupp;
    int fMaxSize;
};

#endif //SRC_ALGORITHMS_CLOGENMERGER_H_