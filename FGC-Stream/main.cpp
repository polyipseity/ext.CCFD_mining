#include "FGC-Stream.h"
#include "Transaction.h"
#include "../data/dbtoken.h"
#include "../algorithms/clogenmerger.h"
#include "../util/hashstorer.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <string>
#include <sstream>

uint32_t NODE_ID = 0;
uint32_t minSupp = 3;
uint32_t totalGens = 0;

//const uint32_t windowSize = 1000;

std::set<uint32_t>** TListByID;

int testedJp = 0;
float sumJp = 0;
float actgen = 0;
bool extratext = false;


void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

    TList->add(t_n, n);
    std::set<uint32_t> emptySet;
    std::multimap<uint32_t, ClosedIS*> fGenitors;
    std::vector<ClosedIS*>* newClosures = new std::vector<ClosedIS*>;


    descend(root, emptySet, t_n, &fGenitors, ClosureList, newClosures, TList, root);
    if (extratext) {
        std::cout << "filterCandidates" << std::endl;
    }
    filterCandidates(&fGenitors, root, ClosureList);
    if (extratext) {
        std::cout << "computeJumpers" << std::endl;
    }
    //computeJumpers(root, t_n, newClosures, TList, root, ClosureList);
    if (extratext) {
        std::cout << "endloop " << newClosures->size() << std::endl;
    }
    for (std::vector<ClosedIS*>::iterator jClos = newClosures->begin(); jClos != newClosures->end(); ++jClos) {
        if (extratext) {
            std::cout << " -------- " << (*jClos)->itemset.size() << " ------ " << t_n.size() << std::endl;
        }

        std::set<std::set<uint32_t>*> preds = compute_preds_exp(*jClos);
        //std::set<std::set<uint32_t>*> preds = std::set<std::set<uint32_t>*>();
        //compute_generators_v2(&preds, *jClos);

        uint32_t key = CISSum((*jClos)->itemset);
        if (extratext) {
            std::cout << "|preds|=" << preds.size() << std::endl;
        }
        //for (std::vector<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {
        for (std::set<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {
            // pour chaque predecesseur, on le retrouve via son intent
            ClosedIS* predNode = findCI(**pred, ClosureList);
            if (predNode) {

              /*if (predNode->deleted) {
                std::cout << "...." << std::endl;
              }*/

              // puis on link dans les deux sens
              predNode->succ->insert(std::make_pair(key, *jClos));
              (*jClos)->preds->insert(std::make_pair(CISSum(**pred), predNode));
            }
            else {
              std::cout << "oh pred is null..." << std::endl;
            }
            delete *pred;
        }
    }
    if (extratext) {
        std::cout << "reseting" << std::endl;
    }
    //std::cout << testedJp << " jumpers tested.\n";
    closureReset(ClosureList); // This is needed to set all visited flags back to false and clear the candidate list

    delete newClosures;
}


void Deletion(std::set<uint32_t> t_0, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
    TList->remove(t_0, n);
    std::vector<ClosedIS*>* iJumpers = new std::vector<ClosedIS*>;
    std::multimap<uint32_t, ClosedIS*>* fObsoletes = new std::multimap<uint32_t, ClosedIS*>;

    descendM(root, t_0, ClosureList, iJumpers, fObsoletes);

    for (std::vector<ClosedIS*>::iterator jprIt = iJumpers->begin(); jprIt != iJumpers->end(); ++jprIt) {
        ClosedIS* jumper = *jprIt;
        dropJumper(jumper, ClosureList);
    }

    for (std::multimap<uint32_t, ClosedIS*>::reverse_iterator obsIt = fObsoletes->rbegin(); obsIt != fObsoletes->rend(); ++obsIt) {
        ClosedIS* obsolete = obsIt->second;
        dropObsolete(obsolete, ClosureList, root); // bug here!!!
    }

    closureReset(ClosureList);

    delete iJumpers;
    delete fObsoletes;
}



// Helper functions
void printAllGens(GenNode* node, std::ostream& _output) {
  //totalGens = 0;
  if (node->succ) {
    for (auto child : *node->succ) {
      printAllGens(child.second, _output);
    }
  }
  _output << node->clos->support << " ";
  std::set<uint32_t> itemset = node->items();
  for (auto item : itemset) {
    _output << item << " ";
  }
  //totalGens++;
  _output << "\n";
}

void releaseAllGens(GenNode* node) {
  //totalGens = 0;
  if (node->succ) {
    for (auto child : *node->succ) {
      releaseAllGens(child.second);
    }
  }
  node->succ->clear();
  delete node->succ;
  delete node;
}


void printAllClosuresWithGens(std::multimap<uint32_t, ClosedIS*> ClosureList) {
    totalGens = 0;
    for (auto x : ClosureList) {
        ClosedIS currCI = *x.second;
        std::cout << "Closed itemset { ";
        for (auto item : currCI.itemset) {
          std::cout << item << " ";
        }
        std::cout << "} (" << currCI.support << ") has generators : ";
        for (auto gen : currCI.gens) {
            totalGens++;
            std::cout << "{ ";
            for (auto item : gen->items()) {
              std::cout << item << " ";
            }
            std::cout << "} ";
        }
        std::cout << "\n";
        
    }
}


void printAllClosuresWithGensTM(std::multimap<uint32_t, ClosedIS*> ClosureList, std::ostream& f) {
  //totalGens = 0;
  for (auto x : ClosureList) {
    ClosedIS currCI = *x.second;
    f << "s=" << currCI.support << " closure : ";
    for (auto item : currCI.itemset) {
      f << item << " ";
    }
    f << "generators : ";
    uint32_t cursor = 0;
    for (auto gen : currCI.gens) {
      if (cursor != 0) {
        f << "  ";
      }
      for (auto item : gen->items()) {
        f << item << " ";
      }
      cursor += 1;
    }
    f << "\n";
  }
}

void printClosureOrderTM(std::multimap<uint32_t, ClosedIS*> ClosureList, std::ostream& f) {
  for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList.begin(); clos != ClosureList.end(); ++clos) {
    ClosedIS currCI = *clos->second;
    uint32_t cursor = 0;
    f << "#" << currCI.id << "[";
    for (auto item : currCI.itemset) {
      if (cursor != 0) {
        f << ", ";
      }
      f << item;
      cursor += 1;
    }
    f << "]|s=" << currCI.support << " => {";
    cursor = 0;
    for (std::multimap<uint32_t, ClosedIS*>::iterator child = currCI.preds->begin(); child != currCI.preds->end(); ++child) {
      ClosedIS currChild = *child->second;
      if (cursor != 0) {
        f << ", ";
      }
      f << "#" << currChild.id;
      cursor += 1;
    }
    f << "}\n";
  }
}

void printClosureOrder(std::multimap<uint32_t, ClosedIS*> ClosureList) {
    for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList.begin(); clos != ClosureList.end(); ++clos) {
        ClosedIS currCI = *clos->second;
        std::cout << "Closed itemset { ";
        for (auto item : currCI.itemset) {
            std::cout << item << " ";
        }
        std::cout << "} (" << currCI.support << ") has children : ";
        for (std::multimap<uint32_t, ClosedIS*>::iterator child = currCI.succ->begin(); child != currCI.succ->end(); ++child) {
            ClosedIS currChild = *child->second;
            std::cout << "{";
            for (auto item : currChild.itemset) {
                std::cout << item << " ";
            }
            std::cout << "}, ";
        }
        std::cout << "\n";
        std::cout << " has predecessors : ";
        for (std::multimap<uint32_t, ClosedIS*>::iterator child = currCI.preds->begin(); child != currCI.preds->end(); ++child) {
          ClosedIS currChild = *child->second;
          std::cout << "{";
          for (auto item : currChild.itemset) {
            std::cout << item << " ";
          }
          std::cout << "}, ";
        }
        std::cout << "\n";
    }
}

void releaseClosures(std::multimap<uint32_t, ClosedIS*> ClosureList) {
  for (multimap<uint32_t, ClosedIS*>::iterator itr = ClosureList.begin(); itr != ClosureList.end(); ++itr) {
    ClosedIS* currCI = itr->second;
    currCI->candidates.clear();
    currCI->gens.clear();
    currCI->itemset.clear();
    currCI->preds->clear();
    delete currCI->preds;
    currCI->succ->clear();
    delete currCI->succ;
    //std::cout << "deleted #" << currCI->id << std::endl;
    if (currCI->id != 1) {
      delete currCI;
    }
  }
}

void sanityCheck(GenNode* n) {
    if (n->clos == nullptr) {
        std::cout << "Sanity check failed for \"generator\" ";
        for (auto item : n->items()) {
            std::cout << item << " ";
        }
        std::cout << "\n";
    }
    for (std::map<uint32_t, GenNode*>::iterator child = n->succ->begin(); child != n->succ->end(); ++child) {
        sanityCheck(child->second);
    }
}

//only use this for (very) small datasets
void sanityCheck_full(ClosedIS* clos, TIDList* TList) {
    std::set<uint32_t> cIS = clos->itemset;
    std::set<uint32_t> closTL;

    if(!cIS.empty())closTL = TList->getISTL(clos->itemset);

    for (std::set<GenNode*>::iterator genIt = clos->gens.begin(); genIt != clos->gens.end(); ++genIt) {
        std::set<uint32_t> items = (*genIt)->items();
        if (!items.empty()){
            std::set<uint32_t> genTL = TList->getISTL(items);
            if (closTL != genTL) {
                std::cout << "pseudo-full SC failed !\n";
            }
        }
    }
}

// Function to convert dataset to integer format and store mappings
int convert_dataset(const std::string& input_file, const std::string& output_file, std::unordered_map<DbToken, int>& fTokenToIntMap, std::unordered_map<int, DbToken>& fIntToTokenMap) {
  std::cout << "Converting dataset to integer format..." << std::endl;
  std::ifstream infile(input_file);
  std::ofstream outfile(output_file);
  std::string line;
  int transaction_count = 0;
  int current_id = 1;
  int header = 0;
  std::vector<std::string> header_items;
  while (std::getline(infile, line)) {
    std::stringstream ss(line);
    std::string item;
    bool first_item = true;

    if (header == 0) {
      while (std::getline(ss, item, ',')) {
        header_items.push_back(item);
      }
      header++;
      // std::cout << "Header items : ";
      // for (auto header_item : header_items) {
      //   std::cout << header_item << " ";
      // }
      // std::cout << "\n" << std::endl;
    }
    else {
      transaction_count++;
      if (transaction_count % 10000 == 0) {
          std::cout << "Processed " << transaction_count << " transactions" << std::endl;
      }
      int pos = 0;
      while (std::getline(ss, item, ',')) {
        // Remove any leading or trailing whitespace
        item.erase(0, item.find_first_not_of(" \t\r\n"));
        item.erase(item.find_last_not_of(" \t\r\n") + 1);
        DbToken token = DbToken(header_items[pos], item);
        pos++;
        if (fTokenToIntMap.find(token) == fTokenToIntMap.end()) {
          fTokenToIntMap[token] = current_id;
          fIntToTokenMap[current_id] = token;
          current_id++;
        }
        outfile << fTokenToIntMap[token] << " ";
      }
      outfile << '\n';
    }
  }
  infile.close();
  outfile.close();
  return transaction_count;
}

// Convert a ClosedIS to a CloMapEntry
CloMapEntry* convertToCloMapEntry(ClosedIS* closedis) {
  std::vector<int> itemset;
  for (auto item : closedis->itemset) {
    itemset.push_back(item);
  }
  CloMapEntry* clomapentry = new CloMapEntry(itemset, closedis->support);
  for (auto gen : closedis->gens) {
    std::vector<int> genvec;
    for (auto item : gen->items()) {
      genvec.push_back(item);
    }
    clomapentry->fGenerators.push_back(genvec);
  }
  return clomapentry;
}

std::vector<ccfd> cfdMiner(std::multimap<uint32_t, ClosedIS*> ClosureList, std::unordered_map<int, DbToken> fIntToTokenMap, std::unordered_map<DbToken, int> fTokenToIntMap, int minSupp, int window_id) {
  // create CloGenMerger by ClosureList, int_to_DbToken_map
  std::unordered_map<HashStorer<Itemset>, CloMapEntry*> fClosures;

  // convert ClosureList to fClosures
  for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList.begin(); clos != ClosureList.end(); ++clos) {
    ClosedIS currCI = *clos->second;
    CloMapEntry* clomapentry = convertToCloMapEntry(&currCI);
    fClosures[HashStorer<Itemset>(clomapentry->fItems)] = clomapentry;
  }
  
  // create CloGenMerger
  CloGenMerger* clogenmerger = new CloGenMerger(fClosures, fIntToTokenMap, fTokenToIntMap);

  // Do CCFD Mining
  std::cout << "CCFD Mining..." << std::endl;
  std::vector<ccfd> ccfd_list = clogenmerger->ccfd_mine(minSupp, window_id);

  return ccfd_list;
}

void collect_ccfds(std::vector<ccfd>& all_ccfds, std::vector<ccfd>& new_ccfds, std::unordered_map<int, DbToken> fIntToTokenMap) {
  for (auto new_ccfd_it = new_ccfds.begin(); new_ccfd_it != new_ccfds.end(); ++new_ccfd_it) {
    bool new_ccfd_without_conflict = true;
    for (auto all_ccfd_it = all_ccfds.begin(); all_ccfd_it != all_ccfds.end(); ++all_ccfd_it) {
      if (*new_ccfd_it == *all_ccfd_it) {
        all_ccfd_it->update_support(*new_ccfd_it);
        new_ccfd_without_conflict = false;
        break;
      }
      else {
        if (all_ccfd_it->resolve_conflict(*new_ccfd_it, fIntToTokenMap)) {
          // if there is a conflict
          if (new_ccfd_it->rhs.empty()) {
            new_ccfd_without_conflict = false;
            break;
          }
        }
      }
    }
    if (new_ccfd_without_conflict) {
      all_ccfds.push_back(*new_ccfd_it);
    }
  }
  for (auto all_ccfd_it = all_ccfds.begin(); all_ccfd_it != all_ccfds.end(); ++all_ccfd_it) {
    if (all_ccfd_it->rhs.empty()) {
      all_ccfds.erase(all_ccfd_it);
    }
  }
  // for (ccfd ccfd : all_ccfds) {
  //   ccfd.print_ccfd(fIntToTokenMap);
  // }
}


void output_ccfd(std::vector<ccfd> ccfd_list, std::unordered_map<int, DbToken> fIntToTokenMap, std::string output_file) {
  // Output the CCFDs to a file
  std::ofstream ofile(output_file);
  for (ccfd ccfd : ccfd_list) {
    std::stringstream ssH;
    std::stringstream ssV;
    ssH << "[";
    ssV << "(";
    bool first = true;
    for (int i : ccfd.lhs) {
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
    for (int i : ccfd.rhs) {
        const auto tok = fIntToTokenMap[i];
        ofile << headH << tok.getAttr() << ", " << headV << tok.getValue() << ")" << ", support : " << ccfd.supp << std::endl;
        // std::cout << headH << tok.getAttr() << ", " << headV << tok.getValue() << ")" << std::endl;
        // e.g. [account_type] => acount_branch, (a55 || 10023)
        // headH = [account_type] =>
        // tok.getAttr() = acount_branch
        // headV = (a55 || 
        // tok.getValue() = 10023
    }
  }
  ofile.close();
}


// template override due to what seems to be a VS bug ?
// comment this function, and compare readings of trx with debug/release configs
// if differences between D and R, then nasty bug still present...
// void Transaction<uint32_t>::load(char* _s, const char* _delims, const short _withcrc) {
//   uint32_t v;
//   uint32_t oldV = 1 << 31;
//   clean();
//   char* pch = _s;
//   while (pch != 0) {
//     std::from_chars(pch, pch + strlen(pch), v);
//     //std::cout << "item is " << v << std::endl;
//     //Stupid hack to avoid blank spaces at end of lines
//     if (v != oldV) {
//       __data.push_back(v);
//     }
//     pch = strtok(0, _delims);
//     oldV = v;
//   }
// };

int main(int argc, char** argv)
{
  /*for (int k = 0; k < windowSize; k++) {
    TListByID[k] = new std::set<uint32_t>;
  }*/

  // argv[1] = input file
  // dataset file in txt format, each line is a transaction, items are separated by spaces
  // argv[2] = minsupp
  // the minimum support threshold for a frequent itemset to be considered, a float number between 0 and 1
  // argv[3] = window_size
  // the size of the sliding window
  // argv[4] = exitAt
  // the number of transactions to process before stopping
  // argv[5] = output_cis_gen // optional
  // output file for the closed itemsets and their generators
  // argv[6] = output_order // optional
  // output file for the order of the closed itemsets

  // preprocess the dataset

  std::unordered_map<int, DbToken> fIntToTokenMap;
  std::unordered_map<DbToken, int> fTokenToIntMap;
  std::vector<ccfd> all_ccfds;
  int transaction_number = convert_dataset(argv[1], "converted_dataset.txt", fTokenToIntMap, fIntToTokenMap);
  std::ifstream input("converted_dataset.txt");

  auto start = std::chrono::high_resolution_clock::now();
  uint32_t window_size = 0;

  uint32_t exitAt = 0;
  if (argc < 3) return 1;

  float supp = std::stof(argv[2]);
  // minSupp = uint32_t(std::max(int(transaction_number * supp), 2));
  std::cout << "Run FGC-Stream + CFDMiner on the database " << argv[1] << std::endl;

  
  char s[10000];
  uint32_t i = 0;

  char* output_cis_gen = 0;
  char* output_order = 0;

  if (argc >= 4) {
    window_size = strtoul(argv[3], 0, 10);//1;
    std::cout << "Window size: " << window_size << std::endl;
    minSupp = uint32_t(std::max(int(window_size * supp), 2));
    std::cout << "Minimal Support: " << minSupp << std::endl;

    TListByID = new std::set<uint32_t> * [window_size];
    for (int k = 0; k < window_size; k++) {
      TListByID[k] = new std::set<uint32_t>();
    }
  }

  if (argc >= 5) {
    exitAt = strtoul(argv[4], 0, 10);//1;
  }

  if (argc >= 6) {
    output_cis_gen = argv[5];
  }
  if (argc >= 7) {
    output_order = argv[6];
  }

  TIDList* TList = new TIDList();

  std::set<uint32_t> closSet;
  std::multimap<uint32_t, ClosedIS*> ClosureList;

  input.getline(s, 10000);
  char* pch = strtok(s, " ");
  Transaction_fgcstream<uint32_t> new_transaction = Transaction_fgcstream<uint32_t>(pch, " ", 0);
  i++; // i == 1
  std::vector<uint32_t> closSetvec = *new_transaction.data();
  TListByID[i % window_size]->insert(closSetvec.begin(), closSetvec.end());

  closSet.insert(closSetvec.begin(), closSetvec.end());
  TList->add(closSet, i);

  while (i < minSupp) {

    input.getline(s, 10000);
    char* pch = strtok(s, " ");
    Transaction_fgcstream<uint32_t> new_transaction = Transaction_fgcstream<uint32_t>(pch, " ", 0);
    i++;
    std::vector<uint32_t> closSetvec = *new_transaction.data();
    TListByID[i % window_size]->insert(closSetvec.begin(), closSetvec.end());

    std::set<uint32_t> closSetPart(closSetvec.begin(), closSetvec.end());
    TList->add(closSetPart, i);

    std::set<uint32_t>::iterator it1 = closSet.begin();
    std::set<uint32_t>::iterator it2 = closSetPart.begin();

    while ((it1 != closSet.end()) && (it2 != closSetPart.end())) {
      if (*it1 < *it2) {
        closSet.erase(it1++);
      }
      else if (*it2 < *it1) {
        ++it2;
      }
      else {
        ++it1;
        ++it2;
      }
    }
    closSet.erase(it1, closSet.end());

  }

  ClosedIS EmptyClos(closSet, minSupp, &ClosureList);
  GenNode* root = new GenNode(1 << 31, nullptr, &EmptyClos);
  int window_id = 0;
  int cfd_miner_interval = window_size / 4;
  while (input.getline(s, 10000)) {
    i++;
    // std::cout << "Processing transaction " << i << std::endl;


    // if (i == 121) {
    //     i++; i--;
    // }

    char* pch = strtok(s, " ");
    /*
    sanityCheck(root);
    for (std::multimap<uint32_t, ClosedIS*>::iterator closIT = ClosureList.begin(); closIT != ClosureList.end(); ++closIT) {
        ClosedIS* clos = closIT->second;
        sanityCheck_full(clos, TList);
    }
    */
    Transaction_fgcstream<uint32_t> new_transaction = Transaction_fgcstream<uint32_t>(pch, " ", 0);
    std::vector<uint32_t> t_nVec = *new_transaction.data();

    std::set<uint32_t> t_n(t_nVec.begin(), t_nVec.end());

    Addition(t_n, i, root, TList, &ClosureList);


    if (i > window_size) {
        Deletion(*TListByID[i% window_size], i- window_size, root, TList, &ClosureList);
    }

    TListByID[i % window_size]->clear();
    TListByID[i % window_size]->insert(t_n.begin(), t_n.end());

    if (i % 1000 == 0) {
      std::cout << i << " transactions processed" << std::endl;
    }
    if (i % 5000 == 0) {
      auto stop = std::chrono::high_resolution_clock::now();
      std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms elapsed between start and current transaction" << std::endl;
    }

    if (i >= window_size && i % cfd_miner_interval == 0) {
      window_id++;
      std::vector<ccfd> ccfd_list = cfdMiner(ClosureList, fIntToTokenMap, fTokenToIntMap, minSupp, window_id);
      // std::cout << "CCFDs found in this CFDMiner interval: " << ccfd_list.size() << std::endl;
      collect_ccfds(all_ccfds, ccfd_list, fIntToTokenMap);
      // std::cout << "Total CCFDs found so far: " << all_ccfds.size() << std::endl;
    }
    if (i == exitAt) {
      break;
    }
  }

  std::cout << "Total number of transactions processed: " << i << std::endl;
  std::cout << "Totoal time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << " ms" << std::endl;

  output_ccfd(all_ccfds, fIntToTokenMap, "FGC_Stream_CFDMiner_ccfd.txt");
  

  /*
  bool removeEnd = true;
  if (removeEnd) {
      std::cout << "removing...\n";
      uint32_t imax = i + windowSize - minSupp;
      while (i++ < imax) {
          Deletion(*TListByID[i % windowSize], i - windowSize, root, TList, &ClosureList);
          if ((imax - i) % 100 == 0) {
              std::cout << imax - i << " transactions left\n";
          }
      }
  }*/

  
  // std::cout << "Displaying all found generators as of transaction " << i << " :\n";



  for (std::map<uint32_t, std::set<uint32_t>*>::iterator oo = TList->TransactionList.begin(); oo != TList->TransactionList.end(); ++oo) {
    delete oo->second;
  }
  TList->TransactionList.clear();
  delete TList;

  for (int k = 0; k < window_size; k++) {
    delete TListByID[k];
  }


  //printAllClosuresWithGensTM(ClosureList, output_cis_gen);
  if (output_cis_gen) {
    std::ofstream f;
    f.open(output_cis_gen);
    //printAllGens(root, f);
    printAllClosuresWithGensTM(ClosureList, f);
    f.close();
  }
  //"./output-cis-gens.txt"
  //printAllClosuresWithGens(ClosureList);
  // std::cout << "Total number of generators: " << totalGens << "\n";
  //printClosureOrder(ClosureList);

  if (output_order) {
    std::ofstream f2;
    f2.open(output_order);
    printClosureOrderTM(ClosureList, f2);
    f2.close();
  }
  //printClosureOrder(ClosureList);
  
  
  //clean CIs
  releaseClosures(ClosureList);
  // clean generators
  releaseAllGens(root);

  return 0;
}