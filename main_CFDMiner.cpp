#include <fstream>
#include <iostream>
#include "data/tabulardatabase.h"
#include "algorithms/clogenminer.h"
#include "algorithms/clogenmerger.h"
#include <dirent.h>
#include <chrono>
int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cout << "Usage: ./CFDMiner csv_file_path minsupp maxsize" << std::endl;
		std::cout << "\t where csv_file_path is the path to the csv file, minsupp a positive float number specifying the minimum support of the discovered rules (range from 0 to 1), and maxsize a positive integer specifying the maximum size of the rules, i.e., the maximum number of attributes occurring in the rule" << std::endl;
    }
    else{
        std::string csv_file_path = argv[1];
        float supp = std::stof(argv[2]);
        int size = atoi(argv[3]);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        std::ifstream ifile(csv_file_path);
        std::cout << "Reading the database from " << csv_file_path << std::endl;
        Database db = TabularDatabase::fromFile(ifile, ',');

        int db_supp = std::max(int(db.size() * supp), 2);
        std::cout << "Run CloGenMiner on the database " << csv_file_path << std::endl;
        std::cout << "Minimal Support: " << db_supp << std::endl;
        CloGenMiner full_m(db, db_supp, size);
        full_m.run();

        CloGenMerger full_merger(full_m);
        std::cout << "CCFD Mining..." << std::endl;
        full_merger.print_ccfd("CFDMiner_ccfd.txt", db_supp);
        
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Runtime = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
    }
}