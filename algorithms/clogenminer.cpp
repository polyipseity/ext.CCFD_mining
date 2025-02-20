#include "clogenminer.h"
#include "../util/setutil.h"
#include <iostream>
#include <sstream>

CloGenMiner::CloGenMiner(Database& db, int minsupp, int maxsize)
    :fDb(db), fMinSupp(minsupp), fMaxSize(maxsize) {
}

void CloGenMiner::run() {
    mine(Itemset(), getSingletons(fMinSupp), Itemset());
    // print_after_mine();
    gen_to_clo();
    // print();
}

int CloGenMiner::getMinSupp() {
    int minsupp = fMinSupp;
    return minsupp;
}

int CloGenMiner::getMaxSize() {
    int maxsize = fMaxSize;
    return maxsize;
}

std::unordered_map<int, DbToken> CloGenMiner::get_used_int_to_token_map(){
    std::unordered_map<int, DbToken> int_to_token_map;
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        for (int i : it->second->fItems) {
            const auto tok = fDb.getToken(i);
            int_to_token_map[i] = DbToken(tok.first, tok.second);
        }
    }
    return int_to_token_map;
}

std::unordered_map<HashStorer<Itemset>, CloMapEntry*> CloGenMiner::get_closures(){
    // copy the closures and return them
    std::unordered_map<HashStorer<Itemset>, CloMapEntry*> closures;
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        CloMapEntry* newclosure = new CloMapEntry(*(it->second));
        closures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
    }
    return closures;
}

void CloGenMiner::mine(const Itemset& prefix, const std::vector<MinerNode>& items, const Itemset& parentClosure) {
    // Early Termination Check
    // If the prefix size reaches the maximum allowed size (fMaxSize), stop recursion to limit exploration depth
    if (prefix.size() == fMaxSize) return;

    // Iterate over items in reverse order
    for (int ix = items.size()-1; ix >= 0; ix--) {
        const MinerNode& node = items[ix]; // Current item to extend the prefix

        // Building the Current Itemset
        // Create a new itemset (iset) by combining the prefix with the current item
        const Itemset iset = join(prefix, node.fItem);

        // Adding Minimal Generators
        // Attempt to register the new itemset as a minimal generator; returns pointer if added, 0 if not
        GenMapEntry* newset = addMinGen(GenMapEntry(iset, node.fSupp, node.fHash));
        
        // Building Suffix and Joins
        // Initialize containers: 'joins' for items always appearing with the current item, 'suffix' for further exploration
        Itemset joins; // Items with the same support as the current item
        std::vector<MinerNode> suffix; // Frequent items to explore in the next recursion

        // Optimization: Choose between bucketing or direct TID intersection based on remaining items
        if (items.size() - ix - 1 > 2 * fDb.nrAttrs()) {
            // Use bucketing for efficiency when many items remain; precompute TID lists for each item
            std::unordered_map<int,TidList> ijtidMap = bucketTids(items, ix+1, node.fTids);
            // for each item in the items vector, if the item always appears in the same transaction as the current item, add the item to the joins vector
            for (uint jx = ix + 1; jx < items.size(); jx++) {
                int jtem = items[jx].fItem;
                TidList& ijtids = ijtidMap[jtem];
                int ijsupp = ijtids.size();
                if (ijsupp == node.fSupp) {
                    joins.push_back(jtem); // Add to joins if support matches (always appears with current item)
                }
                else if (ijsupp >= fMinSupp) {
                    suffix.push_back(MinerNode(jtem, std::move(ijtids))); // Add to suffix if frequent enough
                }
            }
        }
        else {
            // for each item in the items vector, if the item always appears in the same transaction as the current item, add the item to the joins vector
            for (uint jx = ix + 1; jx < items.size(); jx++) {
                const MinerNode& j = items[jx];
                TidList ijtids = intersection(node.fTids, j.fTids);
                int ijsupp = ijtids.size();
                if (ijsupp == node.fSupp) {
                    joins.push_back(j.fItem);
                }
                else if (ijsupp >= fMinSupp) {
                    suffix.push_back(MinerNode(j.fItem, std::move(ijtids)));
                }
            }
        }

        // Sort joins for consistency if there are multiple items
        if (joins.size() > 1) {
            std::sort(joins.begin(), joins.end());
        }

        // Computing the Closure
        // Compute the closure by combining joins (items always present) with the parent's closure
        Itemset newClosure = join(joins, parentClosure);
        // Form the closed itemset by combining the current itemset with its closure
        Itemset cset = join(iset, newClosure);

        // Updating Minimal Generators
        // Find all minimal generators that are subsets of the closed itemset with matching support
        std::list<GenMapEntry*> postMinGens = getMinGens(cset, node.fSupp, node.fHash);
        for (GenMapEntry* ge : postMinGens) {
            if (ge != newset) { // Skip the newly added generator to avoid self-update
                Itemset add; // Items to add to the closure
                std::set_difference(cset.begin(), cset.end(), ge->fItems.begin(), ge->fItems.end(), std::inserter(add, add.begin()));
                if (add.size()) {
                    Itemset uni(ge->fClosure.size() + add.size());
                    auto it = std::set_union(add.begin(), add.end(), ge->fClosure.begin(), ge->fClosure.end(), uni.begin());
                    uni.resize((int)(it - uni.begin()));
                    ge->fClosure.swap(uni);
                }
            }
        }

        // Storing the New Generator
        // If a new minimal generator was added, assign its closure and store it in the generator map
        if (newset) {
            newset->fClosure = newClosure;
            HashStorer<Itemset> h(&newset->fItems);
            fGenerators[h] = newset;
        }
        
        // Recursive Call
        // If there are items in suffix, sort them and recurse to explore deeper itemsets
        if (suffix.size()) {
            sortNodes(suffix);
            mine(iset, suffix, newClosure);
        }
    }
}

void CloGenMiner::gen_to_clo(){
    // update the fClosures with the generators (fGenerators)

    for (auto it = fGenerators.begin(); it != fGenerators.end(); ++it ) {
        // get closure of the generator
        Itemset& closure = it->second->fClosure;
        Itemset& items = it->second->fItems;
        Itemset cset = join(closure, items);

        int supp = it->second->fSupp;
        int hash = it->second->fHash;

        // if the closure is already in the closure map, add the generator to the closure
        if (fClosures.find(HashStorer<Itemset>(cset)) != fClosures.end()) {
            fClosures[HashStorer<Itemset>(cset)]->fGenerators.push_back(items);
        }

        // if the closure is not in the closure map, create a new closure
        else {
            CloMapEntry* newclosure = new CloMapEntry(cset, supp, hash);
            newclosure->fGenerators.push_back(items);

            fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
        }
    }
    // print the closures and their generators
    // for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
    //     std::cout << "Closure:" << std::endl;
    //     for (int i : it->second->fItems) {
    //         // std::cout << i << std::endl;
    //         const auto tok = fDb.getToken(i);
    //         std::cout << tok.first << ", " << tok.second << std::endl;
    //     }
    //     std::cout << "Generators:" << std::endl;
    //     int gen_id = 0;
    //     for (const auto& gen : it->second->fGenerators) {
    //         std::cout << "Generator " << ++gen_id << ":" << std::endl;
    //         for (int i : gen) {
    //             const auto tok = fDb.getToken(i);
    //             std::cout << tok.first << ", " << tok.second << std::endl;
    //         }
    //     }
    //     std::cout << "support: " << it->second->fSupp << std::endl;
    //     std::cout << std::endl;
    // }
}

void CloGenMiner::print_after_mine() {
    // print the generators and their closures
    for (auto it = fGenerators.begin(); it != fGenerators.end(); ++it ) {
        std::cout << "Generators:" << std::endl;
        Itemset& items = it->second->fItems;
        for (int i : items) {
            const auto tok = fDb.getToken(i);
            std::cout << tok.first << ", " << tok.second << std::endl;
        }
        std::cout << "Closure:" << std::endl;
        for (int i : it->second->fClosure) {
            const auto tok = fDb.getToken(i);
            std::cout << tok.first << ", " << tok.second << std::endl;
        }
        std::cout << std::endl;
    }
}

void CloGenMiner::print() {
    for (auto it = fGenerators.begin(); it != fGenerators.end(); ++it ) {
            // CCFD Mining
            // get the RHS of the CCFD
            Itemset& items = it->second->fItems;
            Itemset rhs = it->second->fClosure; // rhs is initially the closure of the generator
            // if rhs is empty, skip
            if (rhs.empty()) continue;
            // if rhs is not empty
            // make the output CCFD left-reduced
            for (int leaveOut : items) {
                // Generate subset of items without leaveOut
                Itemset sub = subset(items, leaveOut);
                if (contains(fGenerators, sub)) {
                    const auto& subClosure = fGenerators[sub]->fClosure;
                    std::vector<int> diff(std::max(rhs.size(), subClosure.size()));
                    auto it = std::set_difference(rhs.begin(), rhs.end(), subClosure.begin(), subClosure.end(), diff.begin());
                    if (diff.empty()) {
                        rhs.clear();
                        break;
                    }
                    diff.resize((int)(it - diff.begin()));
                    rhs.swap(diff);
                }
            }

            // Print the CCFD
            if (rhs.size()) {
                std::stringstream ssH;
                std::stringstream ssV;
                ssH << "[";
                ssV << "(";
                bool first = true;
                for (int i : *it->first.getData()) {
                    const auto tok = fDb.getToken(i);
                    if (first) {
                        first = false;
                        ssH << tok.first;
                        ssV << tok.second;
                    }
                    else {
                        ssH << ", " << tok.first;
                        ssV << ", " << tok.second;
                    }
                }

                ssH << "] => ";
                ssV << " || ";

                std::string headH = ssH.str();
                std::string headV = ssV.str();
                for (int i : rhs) {
                    const auto tok = fDb.getToken(i);
                    std::cout << headH << tok.first << ", " << headV << tok.second << ")" << std::endl;
                    // e.g. [account_type] => acount_branch, (a55 || 10023)
                    // headH = [account_type] =>
                    // tok.first = acount_branch
                    // headV = (a55 || 
                    // tok.second = 10023
                }
            }
        }
}

void CloGenMiner::sortNodes(std::vector<MinerNode>& nodes) const {
    // Sort itemsets by increasing support
    std::sort(nodes.begin(), nodes.end(),
        [](const MinerNode& a, const MinerNode& b)
        {
            return a.fTids.size() < b.fTids.size();
        });
}

std::vector<MinerNode> CloGenMiner::getSingletons(int minsup) const {
    // get the singletons in the database with support greater than or equal to minsup
    // a singleton is a set of one item
    // use this function to init the mine function
    
    std::unordered_map<int, int> nodeIndices; // Build a list of (item, tidlist) pairs
    std::vector<MinerNode> singletons;

    // collect the singletons with enough support
    for (unsigned item = 1; item <= fDb.nrItems(); item++) {
        if (fDb.frequency(item) >= minsup) {
            singletons.push_back(MinerNode(item, fDb.frequency(item)));
            nodeIndices[item] = singletons.size()-1;
        }
    }

    // for each row in the database, if the item in the row is in the nodeIndices, add the row to the tidlist of the item
    for (unsigned row = 0; row < fDb.size(); row++) {
        for (int item : fDb.getRow(row)) {
            if (contains(nodeIndices, item)) {
                singletons[nodeIndices.at(item)].fTids.push_back(row);
            }
        }
    }
    
    // calculate the Hash Tid of each singleton
    for (MinerNode& node : singletons) {
        node.hashTids();
    }
    
    // sort the singletons by increasing support
    sortNodes(singletons);

    return singletons;
}

GenMapEntry* CloGenMiner::addMinGen(const GenMapEntry& newset) {
    if (fGenMap.find(newset.fHash) != fGenMap.end()) {
        for (const GenMapEntry& g : fGenMap.at(newset.fHash)) {
            // if the support of the generator is equal to the support of the newset 
            // and the items of the generator is a subset of the items of the newset
            // then the newset is not a new generator
            // so return 0
            if (g.fSupp == newset.fSupp && isSubsetOf(g.fItems, newset.fItems)) {
                return 0;
            }
        }
    }
    // if the newset is not in the generator map, add it to the generator map
    fGenMap[newset.fHash].push_back(newset);
    return &fGenMap[newset.fHash].back();
}

std::list<GenMapEntry*> CloGenMiner::getMinGens(const Itemset& items, int supp, int hash) {
    // find the generators in the generator map with the same support and a subset of the items

    std::list<GenMapEntry*> minGens;
    if (fGenMap.find(hash) != fGenMap.end()) {
        for (GenMapEntry& g : fGenMap.at(hash)) {
            if (g.fSupp == supp && isSubsetOf(g.fItems, items)) {
                minGens.push_back(&g);
            }
        }
    }
    return minGens;
}

std::unordered_map<int,TidList> CloGenMiner::bucketTids(const std::vector<MinerNode>& items,
                                                      uint jx, const TidList& nodeTids) const {
    // bucket the tids of the items in the items vector into the ijtidMap
    // ijtidMap is a map of item to its tids
    std::unordered_map<int,TidList> ijtidMap;
    for (; jx < items.size(); jx++) {
        ijtidMap[items[jx].fItem].reserve(nodeTids.size());
    }
    for (int t : nodeTids) {
        const Transaction& tup = fDb.getRow(t);
        for (uint vi = 0; vi < tup.size(); vi++) {
            if (ijtidMap.find(tup[vi]) != ijtidMap.end()) {
                ijtidMap.at(tup[vi]).push_back(t); 
            }
        }
    }
    return ijtidMap;
}
