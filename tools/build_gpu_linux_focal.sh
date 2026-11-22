#!/bin/bash
# runs INSIDE the focal chroot: BOINC libs + Lyra OpenCL for linux x86_64, one binary per GPU vendor
set -u
B=/root/boinc; W=/root/bl; mkdir -p $W/lib $W/api /root/out; cd $W
GEN="app_ipc base64 cc_config cert_sig coproc diagnostics filesys hostinfo keyword md5 md5_file mem_usage mfile miofile msg_log network notice opencl_boinc parse prefs procinfo proc_control project_init proxy_info sched_msgs shmem str_util url util wslinfo procinfo_unix synch unix_util"
CF="-O2 -DNDEBUG -DAPP -DHAVE_CONFIG_H -I$B -I$B/api -I$B/lib"
for f in $GEN; do g++ $CF -std=gnu++14 -c $B/lib/$f.cpp -o lib/$f.o 2>lib/$f.err || echo "FAIL lib/$f"; done
for f in boinc_api graphics2_util; do g++ $CF -std=gnu++14 -c $B/api/$f.cpp -o api/$f.o 2>api/$f.err || echo "FAIL api/$f"; done
rm -f libboinc.a libboinc_api.a; ar rcs libboinc.a lib/*.o; ar rcs libboinc_api.a api/*.o
for v in nvidia amd intel; do
  gcc -O3 -march=x86-64-v2 -DUSE_BOINC -DEXPECTED_GPU_VENDOR=\"$v\" $CF -I/root/lyra -c /root/lyra/symm27_gpu.c -o g_$v.o 2>g_$v.err || { echo "FAIL gpu $v"; head -5 g_$v.err; continue; }
  gcc -O2 -I/root/lyra -c /root/lyra/cl_dynload.c -o cld.o
  g++ g_$v.o cld.o -o /root/out/lyra_opencl_linux_x86_64_$v -static-libstdc++ -static-libgcc libboinc_api.a libboinc.a -lpthread -ldl -lrt 2>link_$v.err || { echo "FAIL link $v"; grep -v warning link_$v.err | head -5; continue; }
  echo "built $v: NEEDED $(readelf -d /root/out/lyra_opencl_linux_x86_64_$v | grep -c NEEDED), newest GLIBC: $(objdump -T /root/out/lyra_opencl_linux_x86_64_$v | grep -o 'GLIBC_[0-9.]*' | sort -V | tail -1)"
done
