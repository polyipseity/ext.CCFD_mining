#include "clogenmerger.h"
#include "../util/setutil.h"
#include <iostream>
#include <sstream>

CloGenMerger::CloGenMerger(){
    fMinSupp = 0;
    fMaxSize = 0;
}

CloGenMerger::CloGenMerger(CloGenMiner& given_CloGenMiner){
    fMinSupp = given_CloGenMiner.getMinSupp();
    fMaxSize = given_CloGenMiner.getMaxSize();
    fIntToTokenMap = given_CloGenMiner.get_used_int_to_token_map();
    sync_token_to_int_map();
    fClosures = given_CloGenMiner.get_closures();
    clo_to_gen();
}

CloGenMerger::CloGenMerger(CloGenMerger& given_CloGenMerger){
    fMinSupp = given_CloGenMerger.fMinSupp;
    fMaxSize = given_CloGenMerger.fMaxSize;
    fIntToTokenMap = given_CloGenMerger.fIntToTokenMap;
    sync_token_to_int_map();
    for (auto it = given_CloGenMerger.fClosures.begin(); it != given_CloGenMerger.fClosures.end(); ++it ) {
        CloMapEntry* newclosure = new CloMapEntry(*(it->second));
        fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
    }
    clo_to_gen();
}

CloGenMerger::CloGenMerger(std::unordered_map<HashStorer<Itemset>, CloMapEntry*> closures, std::unordered_map<int, DbToken> int_to_token_map, std::unordered_map<DbToken, int> token_to_int_map){
    fClosures = closures;
    fIntToTokenMap = int_to_token_map;
    fTokenToIntMap = token_to_int_map;
    // sync_token_to_int_map();
    clo_to_gen();
    fMinSupp = 0;
    fMaxSize = 0;
}

void CloGenMerger::run_merge(CloGenMerger& new_CloGenMerger){
    merge(new_CloGenMerger);
    // print_closures();
}

void CloGenMerger::sync_token_to_int_map(){
    // update the token-to-int map
    for (auto it = fIntToTokenMap.begin(); it != fIntToTokenMap.end(); ++it) {
        fTokenToIntMap[it->second] = it->first;
    }
}

void CloGenMerger::clo_to_gen(){
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        Itemset& closure = it->second->fItems;
        int supp = it->second->fSupp;
        int hash = it->second->fHash;
        std::vector<std::vector<int>> generators = it->second->fGenerators; 
        for (std::vector<int> generator : generators) {
            GenMapEntry* newgen = new GenMapEntry(generator, supp, hash);
            // save only the  difference between the closure and the generator
            std::vector<int> diff;
            std::set_difference(closure.begin(), closure.end(), generator.begin(), generator.end(), std::inserter(diff, diff.begin()));
            newgen->fClosure = diff;
            fGenerators[HashStorer<Itemset>(newgen->fItems)] = newgen;
        }
    }
}

void CloGenMerger::print_closures(int MinSupp){
    std::cout << "--- Print closures ---" << std::endl;
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        if (it->second->fSupp < MinSupp) {
            continue;
        }
        std::cout << "-- Closure: --" << std::endl;
        for (int i : it->second->fItems) {
            // std::cout << i << std::endl;
            const auto tok = fIntToTokenMap[i];
            std::cout << tok.getAttr() << ", " << tok.getValue() << std::endl;
        }
        std::cout << "-- Generators: --" << std::endl;
        int gen_id = 0;
        for (const auto& gen : it->second->fGenerators) {
            std::cout << "- Generator " << ++gen_id << ":" << std::endl;
            for (int i : gen) {
                const auto tok = fIntToTokenMap[i];
                std::cout << tok.getAttr() << ", " << tok.getValue() << std::endl;
            }
        }
        std::cout << "-- Support: " << it->second->fSupp << " --" << std::endl;
        std::cout << std::endl;
    }
}

void CloGenMerger::compare(CloGenMerger& new_CloGenMerger, int MinSupp){
    // compare two CloGenMergers
    int num_closures = 0;
    int num_identical_closures = 0;
    int num_same_closures_diff_gen = 0;

    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        if (it->second->fSupp < MinSupp) {
            continue;
        }
        num_closures++;
        for (auto new_it = new_CloGenMerger.fClosures.begin(); new_it != new_CloGenMerger.fClosures.end(); ++new_it ) {
            if (new_it->second->fSupp < MinSupp) {
                continue;
            }
            if (it->second->fItems.size() == new_it->second->fItems.size() && std::includes(new_it->second->fItems.begin(), new_it->second->fItems.end(), it->second->fItems.begin(), it->second->fItems.end())) {
                // same closure is found
                // compare the generators
                int diff_gens = 0;
                for (int i = 0; i < it->second->fGenerators.size(); i++) {
                    int same_gen = 0;
                    for (int j = 0; j < new_it->second->fGenerators.size(); j++) {
                        if (it->second->fGenerators[i].size() == new_it->second->fGenerators[j].size() && std::includes(new_it->second->fGenerators[j].begin(), new_it->second->fGenerators[j].end(), it->second->fGenerators[i].begin(), it->second->fGenerators[i].end())) {
                            same_gen = 1;
                            break;
                        }
                    }
                    if (same_gen == 0) {
                        diff_gens = 1;
                        break;
                    }
                }
                if (diff_gens == 0) {
                    num_identical_closures++;
                }
                else{
                    num_same_closures_diff_gen++;
                }
            }
        }
    }
    std::cout << "Total number of closures in full result: " << num_closures << std::endl;
    std::cout << "Number of identical closures: " << num_identical_closures << std::endl;
    std::cout << "identical percentage: " << (double)(num_identical_closures * 100) / num_closures << "%" << std::endl;
    std::cout << "Number of same closures with different generators: " << num_same_closures_diff_gen << std::endl;
}

void CloGenMerger::print_clogen_info(int MinSupp){
    int num_closures = 0;
    int num_generators = 0;
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        if (it->second->fSupp < MinSupp) {
            continue;
        }
        num_closures++;
    }
    for (auto it = fGenerators.begin(); it != fGenerators.end(); ++it ) {
        if (it->second->fSupp < MinSupp) {
            continue;
        }
        num_generators++;
    }
    std::cout << "Number of closures: " << num_closures << std::endl;
    std::cout << "Number of generators: " << num_generators << std::endl;
}

void CloGenMerger::merge(CloGenMerger& new_CloGenMerger){
    // update the minimum support and maximum size
    fMinSupp += new_CloGenMerger.fMinSupp;
    fMaxSize = std::min(fMaxSize, new_CloGenMerger.fMaxSize);

    // copy the fClosures to new_fClosures
    new_fClosures.clear();
    for (auto it = new_CloGenMerger.fClosures.begin(); it != new_CloGenMerger.fClosures.end(); ++it ) {
        CloMapEntry* newclosure = new CloMapEntry(*(it->second));
        new_fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
    }

    // update the int-to-token map
    update_int_to_token_map(new_CloGenMerger);

    // search for the superset and subsets of the closures
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        for (auto new_it = new_fClosures.begin(); new_it != new_fClosures.end(); ++new_it ) {
            // search for the superset with the largest support
            if (it->second->fItems.size() == new_it->second->fItems.size() && std::includes(new_it->second->fItems.begin(), new_it->second->fItems.end(), it->second->fItems.begin(), it->second->fItems.end())) {
                // reset the minimal superset and subsets
                it->second->minimal_superset = nullptr;
                it->second->subsets.clear();
                it->second->same_closure = new_it->second;
                break;
            }
            else{
                if (it->second->fItems.size() < new_it->second->fItems.size() && std::includes(new_it->second->fItems.begin(), new_it->second->fItems.end(), it->second->fItems.begin(), it->second->fItems.end())) {
                    if (it->second->fSupp < new_it->second->fSupp) {
                        it->second->minimal_superset = new_it->second;
                    }
                }
                // to-do: search for subsets
                if (it->second->fItems.size() > new_it->second->fItems.size() && std::includes(it->second->fItems.begin(), it->second->fItems.end(), new_it->second->fItems.begin(), new_it->second->fItems.end())) {
                    it->second->subsets.push_back(new_it->second);
                }
            }
        }
    }
    for (auto new_it = new_fClosures.begin(); new_it != new_fClosures.end(); ++new_it ) {
        for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
            if (it->second->fItems.size() == new_it->second->fItems.size() && std::includes(new_it->second->fItems.begin(), new_it->second->fItems.end(), it->second->fItems.begin(), it->second->fItems.end())) {
                new_it->second->minimal_superset = nullptr;
                new_it->second->subsets.clear();
                new_it->second->same_closure = it->second;
                break;
            }
            // search for the superset with the largest support
            if (new_it->second->fItems.size() < it->second->fItems.size() && std::includes(it->second->fItems.begin(), it->second->fItems.end(), new_it->second->fItems.begin(), new_it->second->fItems.end())) {
                if (new_it->second->fSupp < it->second->fSupp) {
                    new_it->second->minimal_superset = it->second;
                }
            }
            // search for subsets
            if (new_it->second->fItems.size() > it->second->fItems.size() && std::includes(new_it->second->fItems.begin(), new_it->second->fItems.end(), it->second->fItems.begin(), it->second->fItems.end())) {
                new_it->second->subsets.push_back(it->second);
            }
        }
    }

    // substitute the fClosures with the merged closures
    std::unordered_map<HashStorer<Itemset>, CloMapEntry*> output_fClosures;
    for (auto it = fClosures.begin(); it != fClosures.end(); ++it ) {
        CloMapEntry* newclosure = new CloMapEntry(*(it->second));
        // if same closure is found in the new_fClosures
        if (it->second->same_closure != nullptr) {
            newclosure->fSupp += it->second->same_closure->fSupp;
            for (int i = 0; i < newclosure->fGenerators.size(); i++) {
                for (int j = 0; j < it->second->same_closure->fGenerators.size(); j++) {
                    // if the generator is not in the same_closure, delete it from the newclosure
                    if (newclosure->fGenerators[i].size() != it->second->same_closure->fGenerators[j].size() || !(std::includes(it->second->same_closure->fGenerators[j].begin(), it->second->same_closure->fGenerators[j].end(), newclosure->fGenerators[i].begin(), newclosure->fGenerators[i].end()))) {
                        // delete the generator from the newclosure
                        newclosure->fGenerators.erase(newclosure->fGenerators.begin() + i);
                    }
                }
            }
            output_fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
        }
        // if the closure has no superset and no subset, directly add it to the output_fClosures
        else if (it->second->minimal_superset == nullptr && it->second->subsets.size() == 0)
        {
            output_fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
        }
        // else, update the closure with the superset and subsets
        else{
            if (it->second->minimal_superset != nullptr) {
                newclosure->fSupp += it->second->minimal_superset->fSupp;
            }
            if (it->second->subsets.size() > 0) {
                for (CloMapEntry* subset : it->second->subsets) {
                    for (int i = 0; i < newclosure->fGenerators.size(); i++) {
                        for (int j = 0; j < subset->fGenerators.size(); j++) {
                            // if identical generator is found in the subset, delete it from the newclosure
                            if (newclosure->fGenerators[i].size() == subset->fGenerators[j].size() && std::includes(subset->fGenerators[j].begin(), subset->fGenerators[j].end(), newclosure->fGenerators[i].begin(), newclosure->fGenerators[i].end())) {
                                // delete the generator from the newclosure
                                newclosure->fGenerators.erase(newclosure->fGenerators.begin() + i);
                            }
                        }
                    }
                }
            }
            output_fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
        }
    }
    for (auto it = new_fClosures.begin(); it != new_fClosures.end(); ++it ) {
        CloMapEntry* newclosure = new CloMapEntry(*(it->second));
        // if identical closure is found in the new_fClosures
        // directly add it to the output_fClosures
        if (it->second->same_closure != nullptr) {
            // already handled in the previous loop (fClosures)
            continue;
        }
        // if the closure has no superset and no subset, directly add it to the output_fClosures
        else if (it->second->minimal_superset == nullptr && it->second->subsets.size() == 0)
        {
            output_fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
        }
        // else, update the closure with the superset and subsets
        else{
            if (it->second->minimal_superset != nullptr) {
                newclosure->fSupp += it->second->minimal_superset->fSupp;
            }
            if (it->second->subsets.size() > 0) {
                for (CloMapEntry* subset : it->second->subsets) {
                    for (int i = 0; i < newclosure->fGenerators.size(); i++) {
                        for (int j = 0; j < subset->fGenerators.size(); j++) {
                            // if identical generator is found in the subset, delete it from the newclosure
                            if (newclosure->fGenerators[i].size() == subset->fGenerators[j].size() && std::includes(subset->fGenerators[j].begin(), subset->fGenerators[j].end(), newclosure->fGenerators[i].begin(), newclosure->fGenerators[i].end())) {
                                // delete the generator from the newclosure
                                newclosure->fGenerators.erase(newclosure->fGenerators.begin() + i);
                            }
                        }
                    }
                }
            }
            output_fClosures[HashStorer<Itemset>(newclosure->fItems)] = newclosure;
        }
    }
    fClosures = output_fClosures;
    // update the fGenerators based on the updated fClosures
    clo_to_gen();
}

void CloGenMerger::update_int_to_token_map(CloGenMerger& new_CloGenMerger){
    // get all the tokens in the closures and generators from the new CloGenMiner
    std::unordered_map<int, DbToken> new_int_to_token_map = new_CloGenMerger.fIntToTokenMap;
    // add the int-to-token maps to the int_to_token_map
    if (fIntToTokenMap.empty()) {
        fIntToTokenMap = new_int_to_token_map;
    } 
    else {
        std::unordered_map<int, int> int_update_map;
        for (auto it = new_int_to_token_map.begin(); it != new_int_to_token_map.end(); ++it) {
            // if the token is not in the int_to_token_map, add it
            if (fTokenToIntMap.find(it->second) == fTokenToIntMap.end()) {
                fIntToTokenMap[it->first] = it->second;
                int_update_map[it->first] = it->first;
            }
            // if the token is already in the int_to_token_map, store it in the int_update_map
            else {
                int_update_map[it->first] = fTokenToIntMap[it->second];
            }
        }
        sync_token_to_int_map();
        // update the new_fClosures based on the new int-to-token map
        for (auto it = new_fClosures.begin(); it != new_fClosures.end(); ++it ) {
            for (int i = 0; i < it->second->fItems.size(); i++) {
                it->second->fItems[i] = int_update_map[it->second->fItems[i]];
            }
            for (int i = 0; i < it->second->fGenerators.size(); i++) {
                for (int j = 0; j < it->second->fGenerators[i].size(); j++) {
                    it->second->fGenerators[i][j] = int_update_map[it->second->fGenerators[i][j]];
                }
            }
        }
    }
}

std::vector<ccfd> CloGenMerger::ccfd_mine(int MinSupp, int window_id){
    std::vector<ccfd> ccfd_list;
    for (auto it = fGenerators.begin(); it != fGenerators.end(); ++it ) {
        if (it->second->fSupp < MinSupp) {
            continue;
        }
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
                if (fGenerators[sub]->fSupp < MinSupp) {
                    continue;
                }
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
        // add the CCFD to the list
        if (rhs.size()) {
            std::set<int> lhs;
            for (int i : *it->first.getData()) {
                lhs.insert(i);
            }
            for (int i : rhs) {
                std::set<int> rhs_set;
                rhs_set.insert(i);
                ccfd_list.push_back(ccfd(lhs, rhs_set, it->second->fSupp, window_id));
            }
        }
    }
    return ccfd_list;
}

void CloGenMerger::print_ccfd(std::string filename, int MinSupp){

    // print the CCFD to the file
    std::ofstream ofile(filename);
    for (auto it = fGenerators.begin(); it != fGenerators.end(); ++it ) {
        if (it->second->fSupp < MinSupp) {
            continue;
        }
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
                if (fGenerators[sub]->fSupp < MinSupp) {
                    continue;
                }
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
                const auto tok = fIntToTokenMap[i];
                if (first) {
                    first = false;
                    ssH << tok.getAttr();
                    ssV << tok.getValue();
                }
                else {
                    ssH << ", " << tok.getAttr();
                    ssV << ", " << tok.getValue();
                }
            }

            ssH << "] => ";
            ssV << " || ";

            std::string headH = ssH.str();
            std::string headV = ssV.str();
            for (int i : rhs) {
                const auto tok = fIntToTokenMap[i];
                ofile << headH << tok.getAttr() << ", " << headV << tok.getValue() << ")" << ", support: " << it->second->fSupp << std::endl;
                // std::cout << headH << tok.getAttr() << ", " << headV << tok.getValue() << ")" << std::endl;
                // e.g. [account_type] => acount_branch, (a55 || 10023)
                // headH = [account_type] =>
                // tok.getAttr() = acount_branch
                // headV = (a55 || 
                // tok.getValue() = 10023
            }
        }
    }
    ofile.close();
}