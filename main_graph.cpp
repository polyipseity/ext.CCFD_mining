#include <fstream>
#include <iostream>
#include <sstream>
#include "data/tabulardatabase.h"
#include "algorithms/clogenminer.h"
#include "algorithms/clogenmerger.h"
#include "algorithms/ccfd_graph.h"
// #include <dirent.h>
#include <chrono>
#include <map>
#include <set>

int main(int argc, char *argv[])
{
    // if (argc != 4) {
    if (argc < 4) {
        std::cout << "Usage: ./CFDMiner_Graph minsupp maxsize csv_files_folder..." << std::endl;
		std::cout << "\t where csv_files_folder is the folder containing csv files, minsupp a positive float number specifying the minimum support of the discovered rules (range from 0 to 1), and maxsize a positive integer specifying the maximum size of the rules, i.e., the maximum number of attributes occurring in the rule" << std::endl;
    }
    else{
        /*
        std::string csv_files_folder = argv[1];
        float supp = std::stof(argv[2]);
        int size = atoi(argv[3]);
        */
        float supp = std::stof(argv[1]);
        int size = atoi(argv[2]);

        // read all filenames in the folder
        std::vector<std::string> partition_filenames;
        /*
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (csv_files_folder.c_str())) != NULL) {
            while ((ent = readdir (dir)) != NULL) {
                // Skip hidden files and directories
                if (ent->d_name[0] != '.') {
                    partition_filenames.push_back(csv_files_folder + "/" + ent->d_name);
                }
            }
            closedir (dir);
        }
        else {
            std::cerr << "Error opening directory: " << csv_files_folder << std::endl;
            return 1;
        }
        */
        for (int idx{3}; idx < argc; ++idx) {
            partition_filenames.emplace_back(argv[idx]);
        }

        if (partition_filenames.empty()) {
            // std::cerr << "No CSV files found in directory: " << csv_files_folder << std::endl;
            std::cerr << "No CSV files found in directory" << std::endl;
            return 1;
        }
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        // Create a unified token mapping for all tokens
        std::set<std::pair<std::string, std::string>> all_tokens;
        std::map<std::pair<std::string, std::string>, int> unified_token_to_int_map;
        std::map<int, std::pair<std::string, std::string>> unified_int_to_token_map;
        std::vector<ccfd> unified_ccfd_list;
        int total_transactions = 0;

        // for each partition, read the csv file and mine the CCFDs
        for (const std::string& filename : partition_filenames) {
            std::ifstream ifile(filename);
            if (!ifile.is_open()) {
                std::cerr << "Error opening file: " << filename << std::endl;
                continue;
            }
            
            std::cout << "Reading the database from " << filename << std::endl;
            TabularDatabase temp_db = TabularDatabase::fromFile(ifile, ',');
            Database* db = new Database();
            *db = temp_db;
            
            total_transactions += db->size();
            int db_supp = std::max(int(db->size() * supp), 2);
            std::cout << "Run CloGenMiner on the database " << filename << std::endl;
            std::cout << "Minimal Support: " << db_supp << std::endl;
            CloGenMiner miner(*db, db_supp, size);
            miner.run();

            CloGenMerger merger(miner);
            std::cout << "CCFD Mining..." << std::endl;
            std::vector<ccfd> new_ccfd_list = merger.ccfd_mine(db_supp, 0);
            
            // Process CCFDs from this partition: collect tokens and remap immediately
            for (const ccfd& rule : new_ccfd_list) {
                // Collect tokens from the rule
                for (int item : rule.lhs) {
                    all_tokens.insert(db->getToken(item));
                }
                for (int item : rule.rhs) {
                    all_tokens.insert(db->getToken(item));
                }
            }
            
            // Update the unified token mapping
            int next_id = unified_token_to_int_map.size() + 1;
            for (const auto& token : all_tokens) {
                if (unified_token_to_int_map.find(token) == unified_token_to_int_map.end()) {
                    // if the token is not in the map, add it to the map
                    unified_token_to_int_map[token] = next_id;
                    unified_int_to_token_map[next_id] = token;
                    next_id++;
                }
            }
            
            // Remap and add the CCFDs from this partition
            for (const ccfd& rule : new_ccfd_list) {
                std::set<int> new_lhs;
                for (int item : rule.lhs) {
                    new_lhs.insert(unified_token_to_int_map[db->getToken(item)]);
                }
                
                std::set<int> new_rhs;
                for (int item : rule.rhs) {
                    new_rhs.insert(unified_token_to_int_map[db->getToken(item)]);
                }
                
                unified_ccfd_list.push_back(ccfd(new_lhs, new_rhs, rule.supp, 0));
            }
            
            // Free memory by deleting the database
            delete db;
        }
        
        std::cout << "Total number of CCFDs: " << unified_ccfd_list.size() << std::endl;
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Time taken to mine CCFDs: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;

        std::chrono::steady_clock::time_point begin_graph = std::chrono::steady_clock::now();
        // use the graph-based method to resolve conflicts
        ccfdGraph graph;
        for (const ccfd& rule : unified_ccfd_list) {
            // convert the ccfd to a ccfdNode
            ccfdNode* node = new ccfdNode(rule.lhs, rule.rhs, rule.supp);
            graph.add_node(node);
        }
        int support_threshold = std::max(int(total_transactions * supp), 2);
        graph.filter_infrequent_nodes(support_threshold);
        std::vector<ccfdNode*> independent_set = graph.approximate_mwis();
        std::cout << "Number of CCFDs in the independent set: " << independent_set.size() << std::endl;
        std::chrono::steady_clock::time_point end_graph = std::chrono::steady_clock::now();
        std::cout << "Time taken to find the independent set: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_graph - begin_graph).count() << "[ms]" << std::endl;

        // output the independent set
        std::ofstream output_file("CFDMiner_Graph_ccfd.txt");
        for (ccfdNode* node : independent_set) {
            node->print_ccfd(unified_int_to_token_map, output_file);
        }
        output_file.close();
    }
}