#ifndef SRC_ALGORITHMS_CCFD_GRAPH_H_
#define SRC_ALGORITHMS_CCFD_GRAPH_H_

#include "ccfd_node.h"
#include <vector>
#include <unordered_map>
#include <set>

struct ccfdGraph{
    std::vector<ccfdNode*> nodes;

    void filter_infrequent_nodes(int support_threshold) {
        // filter out the nodes with support less than the support threshold
        for (ccfdNode* node : nodes) {
            if (node->supp < support_threshold) {
                nodes.erase(std::remove(nodes.begin(), nodes.end(), node), nodes.end());
            }
        }
    }

    void add_node(ccfdNode* node) {
        // the list of nodes that is conflicting with the new node
        std::vector<ccfdNode*> conflicts;

        // check if the node is conflicting with or equal to any existing nodes
        for (ccfdNode* existing_node : nodes) {
            // if the node is equal to the existing node
            if (*node == *existing_node) {
                // add the support of the new node to the existing node
                existing_node->supp += node->supp;
                return;
            }
            // if the node is conflicting with the existing node
            else if (node->is_conflict(*existing_node)) {
                // add the existing node to the list of conflicts
                // potential update of the conflicts of the existing node
                conflicts.push_back(existing_node);
            }
        }
        // add the new node to the graph
        node->conflicts = conflicts;
        nodes.push_back(node);
        // update the conflicts of the existing nodes
        for (ccfdNode* conflict : conflicts) {
            conflict->conflicts.push_back(node);
        }
    }

    std::vector<ccfdNode*> approximate_mwis() {
        // use the greedy algorithm to find an approximate maximum weight independent set
        // rank the nodes by the support/(degree + 1)
        // add the node with the highest rank to the independent set
        // remove the node and its conflicts from the graph
        // repeat until the graph is empty
        // output the independent set as a list of ccfdNodes
        std::vector<ccfdNode*> independent_set;
        
        // Create a working copy of the nodes to avoid modifying the original graph
        std::vector<ccfdNode*> remaining_nodes = nodes;
        
        while (!remaining_nodes.empty()) {
            // Find the node with the highest rank (support / (degree + 1))
            ccfdNode* best_node = nullptr;
            double best_rank = -1.0;
            size_t best_index = 0;
            
            for (size_t i = 0; i < remaining_nodes.size(); ++i) {
                ccfdNode* node = remaining_nodes[i];
                int degree = node->conflicts.size();
                double rank = static_cast<double>(node->supp) / (degree + 1);
                
                if (rank > best_rank) {
                    best_rank = rank;
                    best_node = node;
                    best_index = i;
                }
            }
            
            // Add the best node to the independent set
            independent_set.push_back(best_node);
            
            // Create a set of nodes to remove (the best node and its conflicts)
            std::set<ccfdNode*> to_remove;
            to_remove.insert(best_node);
            
            for (ccfdNode* conflict : best_node->conflicts) {
                to_remove.insert(conflict);
            }
            
            // Filter out the nodes that need to be removed
            std::vector<ccfdNode*> filtered_nodes;
            for (ccfdNode* node : remaining_nodes) {
                if (to_remove.find(node) == to_remove.end()) {
                    filtered_nodes.push_back(node);
                }
            }
            
            // Update the remaining nodes
            remaining_nodes = filtered_nodes;
        }
        
        return independent_set;
    }
};


#endif  // SRC_ALGORITHMS_CCFD_GRAPH_H_
