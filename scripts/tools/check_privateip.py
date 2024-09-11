#!/usr/bin/env python3
# check_privateip: check if there exist any duplicate private IPs in config files.

import json

def check_duplicate_private_ips_from_file(file_path):
    try:
        # Open the JSON file and load the data
        with open(file_path, 'r') as file:
            data = json.load(file)
        
        # Extract the list of physical machines
        physical_machines = data.get("physical_machines", [])
        
        # Create a set to store unique private IPs
        private_ips = set()
        
        # Iterate through each machine in the list
        for machine in physical_machines:
            # Get the private IP of the current machine
            private_ip = machine.get("private_ipstr")
            
            # Check if the private IP is already in the set
            if private_ip in private_ips: # Duplicate found
                print("Detect duplicate private ipstr {} in {}".format(private_ip, file_path))
            else:
                # Add the private IP to the set
                private_ips.add(private_ip)
        
        return
    
    except FileNotFoundError:
        print(f"The file at {file_path} was not found.")
        return None
    except json.JSONDecodeError:
        print(f"The file at {file_path} is not a valid JSON file.")
        return None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None

print("Check duplicate private IPs in config.json ...")
check_duplicate_private_ips_from_file("config.json")
print("")

print("Check duplicate private IPs in alicloud_warmup_config.json ...")
check_duplicate_private_ips_from_file("alicloud_warmup_config.json")
print("")

print("Check duplicate private IPs in alicloud_eval_config.json ...")
check_duplicate_private_ips_from_file("shanghai_alicloud_eval_config.json")
print("")

print("Check duplicate private IPs in alicloud_eval_config.json ...")
check_duplicate_private_ips_from_file("silicon_alicloud_eval_config.json")
print("")

print("Check duplicate private IPs in alicloud_eval_config.json ...")
check_duplicate_private_ips_from_file("singapore_alicloud_eval_config.json")
print("")