#ifndef SRC_ALGORITHMS_CCFD_H_
#define SRC_ALGORITHMS_CCFD_H_

#include "../data/database.h"

struct ccfd{
    ccfd(std::set<int> lhs, std::set<int> rhs, int supp) : lhs(lhs), rhs(rhs), supp(supp) {}

    // left hand side of the CCFD
    std::set<int> lhs;
    // right hand side of the CCFD
    std::set<int> rhs;
    // support of the CCFD
    int supp;

    // compare two CCFDs
    bool operator==(const ccfd& other) const {
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
};

#endif //SRC_ALGORITHMS_CCFD_H_