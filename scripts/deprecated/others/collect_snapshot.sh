set -x

roundstr="round4"

mkdir output/snapshot
ssh -i ~/.ssh/id_rsa proj13 "cd projects/covered-private/output/snapshot/ && find . -name \"*${roundstr}*\" -exec scp -i ~/.ssh/id_rsa_for_covered {} 192.168.11.215:~/projects/covered-private/output/snapshot/ \;"
ssh -i ~/.ssh/id_rsa proj14 "cd projects/covered-private/output/snapshot/ && find . -name \"*${roundstr}*\" -exec scp -i ~/.ssh/id_rsa_for_covered {} 192.168.11.215:~/projects/covered-private/output/snapshot/ \;"
ssh -i ~/.ssh/id_rsa proj15 "cd projects/covered-private/output/snapshot/ && find . -name \"*${roundstr}*\" -exec scp -i ~/.ssh/id_rsa_for_covered {} 192.168.11.215:~/projects/covered-private/output/snapshot/ \;"
ssh -i ~/.ssh/id_rsa proj16 "cd projects/covered-private/output/snapshot/ && find . -name \"*${roundstr}*\" -exec scp -i ~/.ssh/id_rsa_for_covered {} 192.168.11.215:~/projects/covered-private/output/snapshot/ \;"