#!/usr/bin/env python3
# configure_ssh: configure passfree ssh connections between each pair of machines

import os

from ..common import *

def checkDuplicateMachine(machine_idx, machine_ip):
    global physical_machine_ips
    if machine_idx < len(physical_machine_ips):
        die(scriptname, "duplicate physical machine index: " + machine_idx)
        return
    for tmp_machine_ip in physical_machine_ips:
        if tmp_machine_ip == machine_ip:
            die(scriptname, "duplicate physical machine ip: " + machine_ip)
            return
    return

# (0) Load config.json for preparation

physical_machines = getValueForKeystr(scriptname, "physical_machines")

# Load non-duplicate machine IPs from config.json
prompt(scriptname, "load non-duplicate machine IPs from config.json")
physical_machine_ips = [] # Follow the order of machine indexes
for tmp_machine_idx in range(len(physical_machines)):
    tmp_machine_ip = physical_machines[tmp_machine_idx]["ipstr"]
    checkDuplicateMachine(tmp_machine_idx, tmp_machine_ip)
    physical_machine_ips.append(tmp_machine_ip)
print("")

# (1) Generate SSH key pair

# Generate passfree SSH key pair if not exist to prepare for connection building
sshkey_filepath = "/home/{}/.ssh/id_rsa_for_covered".format(username)
if not os.path.exists(sshkey_filepath):
    prompt(scriptname, "generate passfree SSH key pair into {}".format(sshkey_filepath))
    generate_passfree_sshkey_cmd = "ssh-keygen -t rsa -P '' -f {}".format(sshkey_filepath)
    generate_passfree_sshkey_subprocess = runCmd(generate_passfree_sshkey_cmd)
    if generate_passfree_sshkey_subprocess.returncode != 0:
        die(scriptname, getSubprocessErrstr(generate_passfree_sshkey_subprocess))
else:
    prompt(scriptname, "passfree SSH key pair already exist in {}".format(sshkey_filepath))
print("")

# (2) Copy SSH private key

# Copy passfree SSH private key to other machines
for tmp_machine_idx in range(len(physical_machines)):
    if tmp_machine_idx == cur_machine_idx:
        continue
    tmp_machine_ip = physical_machines[tmp_machine_idx]["ipstr"]
    prompt(scriptname, "copy passfree SSH private key to machine {}".format(tmp_machine_idx))
    copy_private_sshkey_cmd = "scp {} {}@{}:{}".format(sshkey_filepath, username, tmp_machine_ip, sshkey_filepath)
    copy_private_sshkey_subprocess = runCmd(copy_private_sshkey_cmd)
    if copy_private_sshkey_subprocess.returncode != 0:
        die(scriptname, getSubprocessErrstr(copy_private_sshkey_subprocess))
print("")

# (3) Build passfree SSH connections

# Get SSH public key content
prompt(scriptname, "get SSH public key content")
get_public_sshkey_cmd = "cat {}.pub".format(sshkey_filepath)
get_public_sshkey_subprocess = runCmd(get_public_sshkey_cmd)
if get_public_sshkey_subprocess.returncode != 0:
    die(scriptname, getSubprocessErrstr(get_public_sshkey_subprocess))
public_sshkey_content = getSubprocessOutputstr(get_public_sshkey_subprocess).strip('\n')

# Append SSH public key to authorized_keys of each machine if not exist
authorized_keys_filepath = "/home/{}/.ssh/authorized_keys".format(username)
prompt(scriptname, "append SSH public key to {} of each machine if not exist".format(authorized_keys_filepath))
for tmp_machine_idx in range(len(physical_machines)):
    tmp_machine_ip = physical_machines[tmp_machine_idx]["ipstr"]

    # Check authorized_keys filepath
    dump(scriptname, "check {} in machine {}".format(authorized_keys_filepath, tmp_machine_ip))
    check_authorized_keys_filepath_cmd = ""
    if tmp_machine_idx == cur_machine_idx:
        check_authorized_keys_filepath_cmd = "ls {}".format(authorized_keys_filepath)
    else:
        check_authorized_keys_filepath_cmd = "ssh {}@{} 'ls {}'".format(username, tmp_machine_ip, authorized_keys_filepath)
    need_create_authorized_keys = False
    check_authorized_keys_filepath_subprocess = runCmd(check_authorized_keys_filepath_cmd)
    if check_authorized_keys_filepath_subprocess.returncode != 0: # authorized_keys_filepath not found
        need_create_authorized_keys = True
    elif getSubprocessOutputstr(check_authorized_keys_filepath_subprocess) == "": # authorized_keys_filepath not found
        need_create_authorized_keys = True
    else: # authorized_keys_filepath is found
        need_create_authorized_keys = False
    
    # Create authorized_keys if not exist
    if need_create_authorized_keys:
        dump(scriptname, "create {} in machine {}".format(authorized_keys_filepath, tmp_machine_idx))
        create_authorized_keys_cmd = ""
        if tmp_machine_idx == cur_machine_idx:
            create_authorized_keys_cmd = "touch {}".format(authorized_keys_filepath)
        else:
            create_authorized_keys_cmd = "ssh {}@{} 'touch {}'".format(username, tmp_machine_ip, authorized_keys_filepath)
        create_authorized_keys_subprocess = runCmd(create_authorized_keys_cmd)
        if create_authorized_keys_subprocess.returncode != 0:
            die(scriptname, getSubprocessErrstr(create_authorized_keys_subprocess))

    # Check SSH public key in authorized_keys
    dump(scriptname, "check SSH public key in {} of machine {}".format(authorized_keys_filepath, tmp_machine_idx))
    check_public_sshkey_in_authorized_keys_cmd = ""
    if tmp_machine_idx == cur_machine_idx:
        check_public_sshkey_in_authorized_keys_cmd = "grep -w '{}' {}".format(public_sshkey_content, authorized_keys_filepath)
    else:
        check_public_sshkey_in_authorized_keys_cmd = "ssh {}@{} 'grep -w \"{}\" {}'".format(username, tmp_machine_ip, public_sshkey_content, authorized_keys_filepath)
    need_append_public_sshkey = False
    check_public_sshkey_in_authorized_keys_subprocess = runCmd(check_public_sshkey_in_authorized_keys_cmd)
    if check_public_sshkey_in_authorized_keys_subprocess.returncode != 0: # Error of public_sshkey_content not found
        if getSubprocessErrstr(check_public_sshkey_in_authorized_keys_subprocess) != "": # Error
            die(scriptname, getSubprocessErrstr(check_public_sshkey_in_authorized_keys_subprocess))
        else: # public_sshkey_content not found
            need_append_public_sshkey = True
    elif getSubprocessOutputstr(check_public_sshkey_in_authorized_keys_subprocess) == "": # public_sshkey_content not found
        need_append_public_sshkey = True
    else: # public_sshkey_content is found
        need_append_public_sshkey = False

    # Append SSH public key to authorized_keys
    if need_append_public_sshkey:
        dump(scriptname, "append SSH public key to {} of machine {}".format(authorized_keys_filepath, tmp_machine_idx))
        append_public_sshkey_cmd = ""
        if tmp_machine_idx == cur_machine_idx:
            append_public_sshkey_cmd = "echo {} >> {}".format(public_sshkey_content, authorized_keys_filepath)
        else:
            append_public_sshkey_cmd = "ssh {}@{} 'echo {} >> {}'".format(username, tmp_machine_ip, public_sshkey_content, authorized_keys_filepath)
        append_public_sshkey_subprocess = runCmd(append_public_sshkey_cmd)
        if append_public_sshkey_subprocess.returncode != 0:
            die(scriptname, getSubprocessErrstr(append_public_sshkey_subprocess))