#ifndef SRC_ALGORITHMS_CCFD_H_
#define SRC_ALGORITHMS_CCFD_H_

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
    bool is_conflicting(const ccfd& other) const {
        return lhs == other.lhs && rhs != other.rhs;
    }
};

#endif //SRC_ALGORITHMS_CCFD_H_