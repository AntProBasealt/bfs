clean_scratch

skip_unless invoke_bfs scratch -quit -acl

"$XTOUCH" scratch/{normal,acl}
skip_unless set_acl scratch/acl
ln -s acl scratch/link

bfs_diff -L scratch -acl
