#ifndef SRC_ALGORITHMS_CCFD_H_
#define SRC_ALGORITHMS_CCFD_H_

#include "../data/database.h"
#include <iostream>

struct ccfd{
    ccfd(std::set<int> lhs, std::set<int> rhs, int supp, int window_id) : lhs(lhs), rhs(rhs), supp(supp), window_id(window_id) {}

    int window_partition = 4;

    // left hand side of the CCFD
    std::set<int> lhs;
    // right hand side of the CCFD
    std::set<int> rhs;
    // support of the CCFD
    int supp;
    // window_id where the CCFD is last updated
    int window_id;

    // compare two CCFDs
    bool operator==(ccfd& other) {
        return lhs == other.lhs && rhs == other.rhs;
    }

    // check whether two CCFDs are conflicting
    // if the lhs of two CCFDs are the same, and the rhs are different, then they are conflicting
    bool resolve_conflict(ccfd& other, std::unordered_map<int, DbToken> fIntToTokenMap) {
        if (lhs != other.lhs) {
            return false;
        }
        else {
            bool is_conflict = false;
            std::set<int> to_erase_i;
            std::set<int> to_erase_j;
            for (int i : rhs) {
                DbToken token_i = fIntToTokenMap[i];
                for (int j : other.rhs) {
                    DbToken token_j = fIntToTokenMap[j];
                    if (token_i.getAttr() == token_j.getAttr() && token_i.getValue() != token_j.getValue()) {
                        is_conflict = true;
                        to_erase_i.insert(i);
                        to_erase_j.insert(j);
                    }
                }
            }
            for (int i : to_erase_i) {
                rhs.erase(i);
            }
            for (int j : to_erase_j) {
                other.rhs.erase(j);
            }
            
            return is_conflict;
        }
    }

    // update the support of the CCFD
    void update_support(ccfd& other) {
        window_id = other.window_id;
        if (other.window_id - window_id >= window_partition) {
            // no window overlap, sum the support
            supp = supp + other.supp;
        }
        else {
            // window overlap, add partial support
            supp = supp + other.supp*(window_partition - (other.window_id - window_id))/window_partition;
        }
    }

    // print the CCFD
    void print_ccfd(std::unordered_map<int, DbToken> fIntToTokenMap) {
        std::cout << "CCFD : ";
        for (int i : lhs) {
            const auto tok = fIntToTokenMap[i];
            std::cout << tok.getValue() << " ";
        }
        std::cout << "=> ";
        for (int i : rhs) {
            const auto tok = fIntToTokenMap[i];
            std::cout << tok.getValue() << " ";
        }
        std::cout << ", support : " << supp << std::endl;
    }
};

#endif //SRC_ALGORITHMS_CCFD_H_