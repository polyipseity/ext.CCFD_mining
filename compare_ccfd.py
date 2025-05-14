import numpy as np
import re

def parse_ccfd(ccfd_str):
    # Extract the LHS and RHS parts of the CCFD
    match = re.match(r'\[(.*?)\] => (.*?), \((.*?) \|\| (.*?)\)', ccfd_str)
    if not match:
        raise ValueError(f"Invalid CCFD format: {ccfd_str}")
    
    lhs_attributes = set([attr.strip() for attr in match.group(1).split(',')])
    rhs_attribute = set([match.group(2).strip()])
    lhs_values = set([val.strip() for val in match.group(3).split(',')])
    rhs_value = set([match.group(4).strip()])
    
    return lhs_attributes, rhs_attribute, lhs_values, rhs_value

def parse_ccfd_file(filename):
    with open(filename, 'r') as file:
        ccfd_list = file.readlines()
    return [parse_ccfd(ccfd) for ccfd in ccfd_list]

def compare_ccfd_files(filename_1, filename_2):
    ccfd_list_1 = parse_ccfd_file(filename_1)
    ccfd_list_2 = parse_ccfd_file(filename_2)
    
    # CCFDs in file 1 but not in file 2
    ccfd_1T2F = [ccfd for ccfd in ccfd_list_1 if ccfd not in ccfd_list_2]
    print(f"number of CCFDs in file 1 but not in file 2 (miss): {len(ccfd_1T2F)}")
    # CCFDs in file 2 but not in file 1
    ccfd_1F2T = [ccfd for ccfd in ccfd_list_2 if ccfd not in ccfd_list_1]
    print(f"number of CCFDs in file 2 but not in file 1 (false positive): {len(ccfd_1F2T)}")
    # CCFDs in both files
    ccfd_1T2T = [ccfd for ccfd in ccfd_list_1 if ccfd in ccfd_list_2]
    print(f"number of CCFDs in both files (hit): {len(ccfd_1T2T)}")

if __name__ == '__main__':
    file_name_1 = 'CFDMiner_ccfd.txt'
    # file_name_2 = 'FGC_Stream_CFDMiner_ccfd.txt'
    file_name_2 = 'CFDMiner_Graph_ccfd.txt'
    compare_ccfd_files(file_name_1, file_name_2)