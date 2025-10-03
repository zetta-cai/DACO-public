import sys
import csv
import re
from collections import defaultdict

def parse_file(file_path):
    """
    Parse cache file and return two results:
    1. Set of all keys
    2. Dictionary mapping nodes to their key sets
    """
    key_set = set()
    node_to_keys = defaultdict(set)
    
    # Regular expression for parsing the list
    list_pattern = re.compile(r'\[(.*?)\]')
    
    with open(file_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
                
            # Split key and list part
            key, list_str = line.split(':', 1)
            key = key.strip()
            key_set.add(key)
            
            # Parse list content
            match = list_pattern.search(list_str.strip())
            if match:
                nodes_str = match.group(1)
                # Handle possible empty list
                if nodes_str.strip():
                    nodes = [int(node.strip()) for node in nodes_str.split(',')]
                    for node in nodes:
                        node_to_keys[node].add(key)
    
    return key_set, node_to_keys

def main(file1_path, file2_path, output_csv):
    # Parse both files
    print("Parsing first file...")
    key_set1, node_dict1 = parse_file(file1_path)
    
    print("Parsing second file...")
    key_set2, node_dict2 = parse_file(file2_path)
    
    # Calculate overall comparison data
    total1 = len(key_set1)
    total2 = len(key_set2)
    common_total = len(key_set1 & key_set2)
    
    # Collect all node IDs that appeared
    all_nodes = set(node_dict1.keys()).union(set(node_dict2.keys()))
    sorted_nodes = sorted(all_nodes)
    
    # Write to CSV file
    with open(output_csv, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = ['Node', 'Method1 Count', 'Method2 Count', 'Common Count']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        # Write overall comparison data
        writer.writerow({
            'Node': 'Overall',
            'Method1 Count': total1,
            'Method2 Count': total2,
            'Common Count': common_total
        })
        
        # Write data for each node
        print("Calculating and writing node data...")
        for node in sorted_nodes:
            keys1 = node_dict1.get(node, set())
            keys2 = node_dict2.get(node, set())
            
            count1 = len(keys1)
            count2 = len(keys2)
            common = len(keys1 & keys2)
            
            writer.writerow({
                'Node': node,
                'Method1 Count': count1,
                'Method2 Count': count2,
                'Common Count': common
            })
    
    print(f"Comparison completed. Results saved to {output_csv}")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python cache_comparison.py <file1_path> <file2_path> <output_csv_path>")
        sys.exit(1)
    
    file1 = sys.argv[1]
    file2 = sys.argv[2]
    output = sys.argv[3]
    
    main(file1, file2, output)
    