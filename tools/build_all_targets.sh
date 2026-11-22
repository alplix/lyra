cd /root
export WINEDEBUG=-all WINEPREFIX=/root/.wine64
rm -rf /root/out /root/bl; mkdir -p /root/out
b() { n=$1; o=$2; p=$3; xf="$4"; lf="$5"; echo "=== $n"; bash build_one.sh $n $o $p-gcc $p-g++ $p-ar "$xf" "$lf" 2>&1 | grep -v "FAIL lib/gui_rpc\|FAIL lib/run_app\|FAIL lib/proxy" | tail -3; }
t() { n=$1; shift; bash runtest.sh $n "$@" 2>&1 | tail -3; }
b linux_x86_64 linux x86_64-linux-gnu "-march=x86-64-v2" "-lpthread"; t linux_x86_64 ""
b linux_i686 linux i686-linux-gnu "-msse2 -mfpmath=sse" "-lpthread"; t linux_i686 ""
b linux_aarch64 linux aarch64-linux-gnu "" "-lpthread"; t linux_aarch64 qemu-aarch64-static
b linux_armhf linux arm-linux-gnueabihf "" "-lpthread"; t linux_armhf qemu-arm-static
b linux_ppc64le linux powerpc64le-linux-gnu "" "-lpthread"; t linux_ppc64le qemu-ppc64le-static
b linux_riscv64 linux riscv64-linux-gnu "" "-lpthread"; t linux_riscv64 qemu-riscv64-static
WL="-lpthread -lpsapi -lws2_32 -lole32 -luser32 -ladvapi32 -lshell32 -lwinmm"
b windows_x86_64.exe win x86_64-w64-mingw32 "" "$WL"; t windows_x86_64.exe /usr/lib/wine/wine64
b windows_x86.exe win i686-w64-mingw32 "-msse2 -mfpmath=sse" "$WL"
echo ALLDONE
