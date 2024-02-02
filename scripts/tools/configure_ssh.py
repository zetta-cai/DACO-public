#!/usr/bin/env python3
# configure_ssh: configure passfree ssh connections between each pair of machines

import os

from ..common import *

def checkDuplicateMachine(machine_idx, machine_ip):
    global physical_machine_ips
    if machine_idx < len(physical_machine_ips):
        LogUtil.die(Common.scriptname, "duplicate physical machine index: " + machine_idx)
        return
    for tmp_machine_ip in physical_machine_ips:
        if tmp_machine_ip == machine_ip:
            LogUtil.die(Common.scriptname, "duplicate physical machine ip: " + machine_ip)
            return
    return

# (0) Load config.json and mkdir for preparation

physical_machines = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

# Load non-duplicate machine IPs from config.json
LogUtil.prompt(Common.scriptname, "load non-duplicate machine IPs from config.json")
physical_machine_ips = [] # Follow the order of machine indexes
for tmp_machine_idx in range(len(physical_machines)):
    tmp_machine_ip = physical_machines[tmp_machine_idx]["public_ipstr"]
    checkDuplicateMachine(tmp_machine_idx, tmp_machine_ip)
    physical_machine_ips.append(tmp_machine_ip)
print("")

# Mkdir for preparation
sshkey_dirpath = os.path.dirname(Common.sshkey_filepath)
LogUtil.prompt(Common.scriptname, "create {} if not exist for preparation".format(sshkey_dirpath))
for tmp_machine_idx in range(len(physical_machines)):
    tmp_machine_public_ip = physical_machines[tmp_machine_idx]["public_ipstr"]
    if tmp_machine_idx == Common.cur_machine_idx:
        mkdir_cmd = "mkdir -p {}".format(sshkey_dirpath)
    else:
        mkdir_cmd = "ssh -i {} {}@{} 'mkdir -p {}'".format(Common.sshkey_filepath, Common.username, tmp_machine_public_ip, sshkey_dirpath)
    mkdir_subprocess = SubprocessUtil.runCmd(mkdir_cmd)
    if mkdir_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(mkdir_subprocess))
print("")

# (1) Generate SSH key pair

# Generate passfree SSH key pair if not exist to prepare for connection building
if not os.path.exists(Common.sshkey_filepath):
    LogUtil.prompt(Common.scriptname, "generate passfree SSH key pair into {}".format(Common.sshkey_filepath))
    generate_passfree_sshkey_cmd = "ssh-keygen -t rsa -P '' -f {}".format(Common.sshkey_filepath)
    generate_passfree_sshkey_subprocess = SubprocessUtil.runCmd(generate_passfree_sshkey_cmd)
    if generate_passfree_sshkey_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(generate_passfree_sshkey_subprocess))
else:
    LogUtil.prompt(Common.scriptname, "passfree SSH key pair already exist in {}".format(Common.sshkey_filepath))
print("")

# (2) Copy SSH private key

# Copy passfree SSH private key to other machines
for tmp_machine_idx in range(len(physical_machines)):
    if tmp_machine_idx == Common.cur_machine_idx:
        continue
    tmp_machine_public_ip = physical_machines[tmp_machine_idx]["public_ipstr"]
    LogUtil.prompt(Common.scriptname, "copy passfree SSH private key to machine {}".format(tmp_machine_idx))
    copy_private_sshkey_cmd = "scp -i {} {} {}@{}:{}".format(Common.sshkey_filepath, Common.sshkey_filepath, Common.username, tmp_machine_public_ip, Common.sshkey_filepath)
    copy_private_sshkey_subprocess = SubprocessUtil.runCmd(copy_private_sshkey_cmd)
    if copy_private_sshkey_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(copy_private_sshkey_subprocess))
print("")

# (3) Build passfree SSH connections

# Get SSH public key content
LogUtil.prompt(Common.scriptname, "get SSH public key content")
get_public_sshkey_cmd = "cat {}.pub".format(Common.sshkey_filepath)
get_public_sshkey_subprocess = SubprocessUtil.runCmd(get_public_sshkey_cmd)
if get_public_sshkey_subprocess.returncode != 0:
    LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(get_public_sshkey_subprocess))
public_sshkey_content = SubprocessUtil.getSubprocessOutputstr(get_public_sshkey_subprocess).strip('\n')

# Append SSH public key to authorized_keys of each machine if not exist
authorized_keys_filepath = "/home/{}/.ssh/authorized_keys".format(Common.username)
LogUtil.prompt(Common.scriptname, "append SSH public key to {} of each machine if not exist".format(authorized_keys_filepath))
for tmp_machine_idx in range(len(physical_machines)):
    tmp_machine_public_ip = physical_machines[tmp_machine_idx]["public_ipstr"]

    # Check authorized_keys filepath
    LogUtil.dump(Common.scriptname, "check {} in machine {}".format(authorized_keys_filepath, tmp_machine_public_ip))
    check_authorized_keys_filepath_cmd = ""
    if tmp_machine_idx == Common.cur_machine_idx:
        check_authorized_keys_filepath_cmd = "ls {}".format(authorized_keys_filepath)
    else:
        check_authorized_keys_filepath_cmd = "ssh -i {} {}@{} 'ls {}'".format(Common.sshkey_filepath, Common.username, tmp_machine_public_ip, authorized_keys_filepath)
    need_create_authorized_keys = False
    check_authorized_keys_filepath_subprocess = SubprocessUtil.runCmd(check_authorized_keys_filepath_cmd)
    if check_authorized_keys_filepath_subprocess.returncode != 0: # authorized_keys_filepath not found
        need_create_authorized_keys = True
    elif SubprocessUtil.getSubprocessOutputstr(check_authorized_keys_filepath_subprocess) == "": # authorized_keys_filepath not found
        need_create_authorized_keys = True
    else: # authorized_keys_filepath is found
        need_create_authorized_keys = False
    
    if need_create_authorized_keys:
        # Create authorized_keys if not exist
        LogUtil.dump(Common.scriptname, "create {} in machine {}".format(authorized_keys_filepath, tmp_machine_idx))
        create_authorized_keys_cmd = ""
        if tmp_machine_idx == Common.cur_machine_idx:
            create_authorized_keys_cmd = "touch {}".format(authorized_keys_filepath)
        else:
            create_authorized_keys_cmd = "ssh -i {} {}@{} 'touch {}'".format(Common.sshkey_filepath, Common.username, tmp_machine_public_ip, authorized_keys_filepath)
        create_authorized_keys_subprocess = SubprocessUtil.runCmd(create_authorized_keys_cmd)
        if create_authorized_keys_subprocess.returncode != 0:
            LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(create_authorized_keys_subprocess))
        
        need_append_public_sshkey = True
    else:
        # Check SSH public key in authorized_keys
        LogUtil.dump(Common.scriptname, "check SSH public key in {} of machine {}".format(authorized_keys_filepath, tmp_machine_idx))
        check_public_sshkey_in_authorized_keys_cmd = ""
        if tmp_machine_idx == Common.cur_machine_idx:
            check_public_sshkey_in_authorized_keys_cmd = "grep -w '{}' {}".format(public_sshkey_content, authorized_keys_filepath)
        else:
            check_public_sshkey_in_authorized_keys_cmd = "ssh -i {} {}@{} 'grep -w \"{}\" {}'".format(Common.sshkey_filepath, Common.username, tmp_machine_public_ip, public_sshkey_content, authorized_keys_filepath)
        need_append_public_sshkey = False
        check_public_sshkey_in_authorized_keys_subprocess = SubprocessUtil.runCmd(check_public_sshkey_in_authorized_keys_cmd)
        if check_public_sshkey_in_authorized_keys_subprocess.returncode != 0: # Error of public_sshkey_content not found
            if SubprocessUtil.getSubprocessErrstr(check_public_sshkey_in_authorized_keys_subprocess) != "": # Error
                LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(check_public_sshkey_in_authorized_keys_subprocess))
            else: # public_sshkey_content not found
                need_append_public_sshkey = True
        elif SubprocessUtil.getSubprocessOutputstr(check_public_sshkey_in_authorized_keys_subprocess) == "": # public_sshkey_content not found
            need_append_public_sshkey = True
        else: # public_sshkey_content is found
            need_append_public_sshkey = False

    # Append SSH public key to authorized_keys
    if need_append_public_sshkey:
        LogUtil.dump(Common.scriptname, "append SSH public key to {} of machine {}".format(authorized_keys_filepath, tmp_machine_idx))
        append_public_sshkey_cmd = ""
        if tmp_machine_idx == Common.cur_machine_idx:
            append_public_sshkey_cmd = "echo {} >> {}".format(public_sshkey_content, authorized_keys_filepath)
        else:
            append_public_sshkey_cmd = "ssh -i {} {}@{} 'echo {} >> {}'".format(Common.sshkey_filepath, Common.username, tmp_machine_public_ip, public_sshkey_content, authorized_keys_filepath)
        append_public_sshkey_subprocess = SubprocessUtil.runCmd(append_public_sshkey_cmd)
        if append_public_sshkey_subprocess.returncode != 0:
            LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(append_public_sshkey_subprocess))