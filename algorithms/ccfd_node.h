#ifndef SRC_ALGORITHMS_CCFD_NODE_H_
#define SRC_ALGORITHMS_CCFD_NODE_H_

#include "../data/database.h"
#include <iostream>
#include <algorithm>  // for std::includes
#include <map>
#include <string>
#include <sstream>

struct ccfdNode{
    ccfdNode(std::set<int> lhs, std::set<int> rhs, int supp) : lhs(lhs), rhs(rhs), supp(supp) {}

    // left hand side of the CCFD
    std::set<int> lhs;
    // right hand side of the CCFD
    std::set<int> rhs;
    // support of the CCFD
    int supp;
    // a list of pointers to other CCFDs that are conflicting with this CCFD
    std::vector<ccfdNode*> conflicts;

    // Two CCFDs are equal if their lhs and rhs are the same
    bool operator==(ccfdNode& other) {
        return lhs == other.lhs && rhs == other.rhs;
    }

    // Two CCFDs, C1, C2 are conflicting if 
    // (lhs(C1) \subset lhs(C2)) and (rhs(C1) != rhs(C2))
    // or (lhs(C2) \subset lhs(C1)) and (rhs(C1) != rhs(C2))
    bool is_conflict(ccfdNode& other) {
        // check if lhs is a proper subset of other.lhs
        bool lhs_subset_other = std::includes(
            other.lhs.begin(), other.lhs.end(),
            lhs.begin(),       lhs.end()
        ) && lhs != other.lhs;

        // check if other.lhs is a proper subset of lhs
        bool other_subset_lhs = std::includes(
            lhs.begin(),       lhs.end(),
            other.lhs.begin(), other.lhs.end()
        ) && lhs != other.lhs;

        // conflict if one lhs is a proper subset of the other AND rhs differ
        return (lhs_subset_other || other_subset_lhs)
            && (rhs != other.rhs);
    }

    // print the CCFD
    void print_ccfd(std::map<int, std::pair<std::string, std::string>> fIntToTokenMap, std::ofstream& output_file) {
        std::stringstream ssH;
        std::stringstream ssV;
        ssH << "[";
        ssV << "(";
        bool first = true;
        for (int i : lhs) {
            const auto tok = fIntToTokenMap[i];
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
            const auto tok = fIntToTokenMap[i];
            output_file << headH << tok.first << ", " << headV << tok.second << ")" << ", support : " << supp << std::endl;
        }
    }
};

#endif  // SRC_ALGORITHMS_CCFD_NODE_H_