cd /root
B=/root/boinc; W=/root/blw; rm -rf $W; mkdir -p $W/lib $W/api /root/out; cd $W
CXX=x86_64-w64-mingw32-g++; CC=x86_64-w64-mingw32-gcc; AR=x86_64-w64-mingw32-ar
GEN="app_ipc base64 cc_config cert_sig coproc diagnostics filesys hostinfo keyword md5 md5_file mem_usage mfile miofile msg_log network notice opencl_boinc parse prefs procinfo proc_control project_init proxy_info sched_msgs shmem str_replace str_util url util wslinfo boinc_win daemonmgt_win diagnostics_win idlemon_win procinfo_win run_app_windows win_util stackwalker_win"
CF="-O2 -DNDEBUG -DAPP -D_WIN32_WINNT=0x0601 -DSECURITY_WIN32 -I/root/winincl -I$B/api -I$B/lib -I$B/win_build"
for f in $GEN; do $CXX $CF -std=gnu++14 -c $B/lib/$f.cpp -o lib/$f.o 2>lib/$f.err || echo -n "FAIL:$f "; done; echo
for f in boinc_api graphics2_util; do $CXX $CF -std=gnu++14 -c $B/api/$f.cpp -o api/$f.o 2>api/$f.err || echo "FAIL api/$f"; done
$AR rcs libboinc.a lib/*.o; $AR rcs libboinc_api.a api/*.o
$CC -O2 -DCL_TARGET_OPENCL_VERSION=120 -I/root/lyra -I/usr/include -c /root/lyra/cl_dynload.c -o cld.o
WL="-lpthread -lpsapi -lws2_32 -lole32 -luser32 -ladvapi32 -lshell32 -lwinmm"
for v in nvidia amd intel; do
  $CC -O3 -DUSE_BOINC -DCL_TARGET_OPENCL_VERSION=120 -DEXPECTED_GPU_VENDOR=\"$v\" $CF -I/root/lyra -I/usr/include -c /root/lyra/symm27_gpu.c -o g_$v.o 2>g_$v.err || { echo "FAIL gpu $v"; head -5 g_$v.err; continue; }
  $CXX -static g_$v.o cld.o -o /root/out/lyra_opencl_windows_x86_64_$v.exe libboinc_api.a libboinc.a $WL 2>link_$v.err || { echo "FAIL link $v"; grep -v warning link_$v.err | head -5; continue; }
  echo "built $v $(stat -c %s /root/out/lyra_opencl_windows_x86_64_$v.exe)"
done
