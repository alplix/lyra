#!/bin/bash
# usage: build_one.sh <name> <os: linux|win> <CC> <CXX> <AR> "<extra cflags>" "<link flags>" ; builds BOINC libs from /root/boinc for the target, then Lyra
NAME=$1; OSK=$2; CC=$3; CXX=$4; AR=$5; XF="$6"; LF="$7"
B=/root/boinc; W=/root/bl/$NAME; mkdir -p $W/lib $W/api /root/out; cd $W
GEN="app_ipc base64 cc_config cert_sig coproc diagnostics filesys gui_rpc_client gui_rpc_client_ops gui_rpc_client_print hostinfo keyword md5 md5_file mem_usage mfile miofile msg_log network notice opencl_boinc parse prefs procinfo proc_control project_init proxy_info sched_msgs shmem str_util url util wslinfo"
if [ $OSK = linux ]; then GEN="$GEN procinfo_unix synch unix_util"; DEF="-DHAVE_CONFIG_H"; INC="-I$B -I$B/api -I$B/lib"
else GEN="$GEN str_replace boinc_win daemonmgt_win diagnostics_win idlemon_win procinfo_win run_app_windows win_util"; [ "$NAME" != windows_arm64 ] && GEN="$GEN stackwalker_win"; DEF="-D_WIN32_WINNT=0x0601 -DSECURITY_WIN32"; INC="-I/root/winincl -I$B/api -I$B/lib -I$B/win_build"; fi
CF="-O2 -DNDEBUG -DAPP $DEF $XF $INC"
fail=0
compile() { f=$1; src=$B/lib/$f.cpp; [ -f $src ] || { echo "nosrc $f"; return; }; $CXX $CF -std=gnu++14 -c $src -o lib/$f.o 2>lib/$f.err || echo "FAIL lib/$f"; }
export -f compile; export B CXX CF
for f in $GEN; do echo $f; done | xargs -P4 -I{} bash -c 'compile {}' 2>&1 | grep -v "^nosrc" 
for f in boinc_api graphics2_util; do $CXX $CF -std=gnu++14 -c $B/api/$f.cpp -o api/$f.o 2>api/$f.err || echo "FAIL api/$f"; done
rm -f libboinc.a libboinc_api.a; $AR rcs libboinc.a lib/*.o; $AR rcs libboinc_api.a api/*.o
# Lyra
$CC -O3 $XF -DUSE_BOINC -DAPP $INC -c /root/lyra/symm27.c -o lyra.o 2>lyra.err || { echo "FAIL lyra.c"; head -5 lyra.err; exit 1; }
OUT=/root/out/$(echo $NAME | tr -d ' ')
$CXX -static lyra.o -o $OUT libboinc_api.a libboinc.a $LF 2>link.err || { echo "FAIL link"; grep -v warning link.err | head -8; exit 1; }
ls -la $OUT | cut -c20-90
