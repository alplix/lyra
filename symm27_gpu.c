// Symmetric 27-tuplets - OpenCL (GPU) surumu
// symm27.c ile ayni girdi/cikti/checkpoint bicimi; cikti CPU surumuyle birebir ayni olmalidir.
// Eleme + Fermat testi GPU'da (96 bit Montgomery); kesin dogrulama GMP ile CPU'da.
// Derleme: make gpu    Kullanim: ./symm27_gpu in out checkpoint [--device N]
#define main cpu_main
#include "symm27.c"
#undef main
#ifdef __APPLE__
#define CL_SILENCE_DEPRECATION
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <sys/time.h>

#define CK(x) do{ cl_int e_=(x); if(e_!=CL_SUCCESS){ fprintf(stderr,"OpenCL hata %d (%s:%d)\n",e_,__FILE__,__LINE__); exit(5);} }while(0)

static const char *KSRC =
"#define P37 7420738134810UL\n"
"__constant uint PT[11]={0,30,48,54,78,84,90,114,120,138,168};\n"
"__kernel void mkctab(ulong a,uint kc,uint np,__global const uint*pr,__global const uint*pm,__global uint*ctab){\n"
"  uint p=get_global_id(0); if(p>=np) return; uint q=pr[p]; uint pmv=pm[p]; ulong c0=((a%q)*(ulong)pmv)%q; uint c=(uint)c0;\n"
"  for(uint kl=0;kl<kc;kl++){ ctab[(size_t)kl*np+p]=c; c+=pmv; if(c>=q) c-=q; }\n"
"}\n"
"__kernel void sieve1(uint words,uint nbit,uint np,__global const ulong*moff,__global const ulong*masks,__global const uint*ctab,\n"
"  __global uint*cnt,__global uint2*out,uint cap,ulong total){\n"
"  ulong i=get_global_id(0); if(i>=total) return;\n"
"  uint kl=(uint)(i/words); uint w=(uint)(i-(ulong)kl*words);\n"
"  __global const uint*c=ctab+(size_t)kl*np; ulong acc=~0UL;\n"
"  for(uint p=0;p<nbit;p++){ acc&=masks[moff[p]+(ulong)c[p]*words+w]; if(!acc) return; }\n"
"  while(acc){ uint bpos=63u-(uint)clz(acc&(0UL-acc)); acc&=acc-1UL; uint idx=atomic_inc(cnt); if(idx<cap) out[idx]=(uint2)(kl,w*64u+bpos); }\n"
"}\n"
"__kernel void seg(__global const ulong*dob,__global const uint2*in,__global const uint*cin,__global uint2*out,__global uint*cnt,uint cap,uint pb,uint pe,uint np,\n"
"  __global const uint*pr,__global const ulong*pmg,__global const uint*boff,__global const ulong*bits,__global const uint*ctab){\n"
"  uint j=get_global_id(0); uint n=cin[0]; if(n>cap) n=cap; if(j>=n) return; uint2 s=in[j]; ulong v=dob[s.y]; __global const uint*c=ctab+(size_t)s.x*np;\n"
"  for(uint p=pb;p<pe;p++){ uint q=pr[p]; ulong qt=mul_hi(v,pmg[p]); uint r=(uint)(v-qt*q)+c[p]; r-=(r>=q)?q:0;\n"
"    if((bits[boff[p]+(r>>6)]>>(r&63))&1UL) return; }\n"
"  uint idx=atomic_inc(cnt); if(idx<cap) out[idx]=s;\n"
"}\n"
"int ge3(const uint*a,const uint*n){ for(int i=2;i>=0;i--){ if(a[i]!=n[i]) return a[i]>n[i]; } return 1; }\n"
"void sub3(uint*a,const uint*n){ ulong b=0; for(int i=0;i<3;i++){ ulong s=(ulong)a[i]-(ulong)n[i]-b; a[i]=(uint)s; b=(s>>63)&1UL; } }\n"
"void dbl3(uint*x,const uint*n){ x[2]=(x[2]<<1)|(x[1]>>31); x[1]=(x[1]<<1)|(x[0]>>31); x[0]<<=1; if(ge3(x,n)) sub3(x,n); }\n"
"void montmul(uint*r,const uint*a,const uint*b,const uint*n,uint ninv){\n"
"  uint t[5]={0,0,0,0,0};\n"
"  for(int i=0;i<3;i++){\n"
"    ulong C=0; for(int j=0;j<3;j++){ ulong s=(ulong)t[j]+(ulong)a[j]*b[i]+C; t[j]=(uint)s; C=s>>32; }\n"
"    ulong s=(ulong)t[3]+C; t[3]=(uint)s; t[4]=(uint)(s>>32);\n"
"    uint m=t[0]*ninv; ulong s0=(ulong)t[0]+(ulong)m*n[0]; C=s0>>32;\n"
"    for(int j=1;j<3;j++){ ulong s2=(ulong)t[j]+(ulong)m*n[j]+C; t[j-1]=(uint)s2; C=s2>>32; }\n"
"    ulong s3=(ulong)t[3]+C; t[2]=(uint)s3; t[3]=t[4]+(uint)(s3>>32);\n"
"  }\n"
"  uint q[3]={t[0],t[1],t[2]}; if(t[3]||ge3(q,n)) sub3(q,n); r[0]=q[0]; r[1]=q[1]; r[2]=q[2];\n"
"}\n"
"int fermat2(const uint*n){\n"
"  uint x=n[0]; for(int i=0;i<5;i++) x*=2u-n[0]*x; uint ninv=0u-x;\n"
"  uint one[3]={1,0,0}; for(int i=0;i<96;i++) dbl3(one,n);\n"
"  uint two[3]={one[0],one[1],one[2]}; dbl3(two,n);\n"
"  uint e[3]={n[0]&~1u,n[1],n[2]}; int top=95; while(top>=0 && !((e[top>>5]>>(top&31))&1u)) top--;\n"
"  uint r[3]={two[0],two[1],two[2]};\n"
"  for(int b=top-1;b>=0;b--){ montmul(r,r,r,n,ninv); if((e[b>>5]>>(b&31))&1u) dbl3(r,n); }\n"
"  return r[0]==one[0]&&r[1]==one[1]&&r[2]==one[2];\n"
"}\n"
"__kernel void fermat(__global const ulong*dob,__global const uint2*sv,__global const uint*cin,ulong kbase,__global uchar*res,uint lim){\n"
"  uint j=get_global_id(0); uint n=cin[0]; if(n>lim) n=lim; if(j>=n) return; uint2 s=sv[j]; ulong k=kbase+s.x; ulong dd=dob[s.y];\n"
"  ulong lo=k*P37; ulong hi=mul_hi(k,P37); ulong lo2=lo+dd; hi+=(lo2<lo)?1UL:0UL;\n"
"  uint X[3]={(uint)lo2,(uint)(lo2>>32),(uint)hi}; uchar r=0; int ok=1;\n"
"  for(int i=0;i<11&&ok;i++){ uint N[3]; ulong c=(ulong)X[0]+PT[i]; N[0]=(uint)c; c>>=32; c+=X[1]; N[1]=(uint)c; c>>=32; c+=X[2]; N[2]=(uint)c;\n"
"    ok=fermat2(N); if(i==0&&ok) r|=1; }\n"
"  if(ok) r|=2; res[j]=r;\n"
"}\n";

static double wall(void){ struct timeval t; gettimeofday(&t,0); return t.tv_sec+t.tv_usec*1e-6; }
typedef struct { cl_uint a,b; } u2;
static int cmp_u2(const void*x,const void*y){ const u2*p=x,*q=y; if(p->a!=q->a) return p->a<q->a?-1:1; return p->b<q->b?-1:(p->b>q->b); }

int main(int argc,char**argv){
  char inp[512],outp[512],cp[512]; const char*pos[3]={"in","out","checkpoint"}; int np_=0, dev=0;
#ifdef USE_BOINC
  int rc=boinc_init(); if(rc) return rc;
#endif
  for(int i=1;i<argc;i++){
    if(!strcmp(argv[i],"--device")&&i+1<argc) dev=atoi(argv[++i]);
    else if(!strcmp(argv[i],"--nthreads")&&i+1<argc) i++;
    else if(np_<3) pos[np_++]=argv[i];
  }
  rpath(pos[0],inp,sizeof inp); rpath(pos[1],outp,sizeof outp); rpath(pos[2],cp,sizeof cp);
  FILE*fi=fopen(inp,"r"); unsigned long long a_,b_;
  if(!fi||fscanf(fi,"%llu %llu",&a_,&b_)!=2||a_>b_){ fprintf(stderr,"girdi okunamadi\n");
#ifdef USE_BOINC
    boinc_finish(1);
#endif
    return 1; }
  fclose(fi); k0=a_; k1=b_; hits=malloc((size_t)MAXHITS*160); nchunks=(k1-k0)/CH+1;
  load_ckpt(cp);
  nthreads=4; setup();
  // ---- tablolar ----
  uint32_t*boff=malloc(np*4); size_t tw=0; for(int p=0;p<np;p++){ boff[p]=(uint32_t)tw; tw+=(pr[p]+63)/64; }
  uint64_t*bits=malloc(tw*8); for(int p=0;p<np;p++) memcpy(bits+boff[p],bb[p],((pr[p]+63)/64)*8);
  uint32_t*prv=malloc(np*4); for(int p=0;p<np;p++) prv[p]=pr[p];
    // ---- OpenCL ----
  cl_platform_id pl[4]; cl_uint npl=0; CK(clGetPlatformIDs(4,pl,&npl)); cl_device_id ds[16]; cl_uint nds=0;
  for(cl_uint i=0;i<npl;i++){ cl_device_id d[8]; cl_uint n=0; if(clGetDeviceIDs(pl[i],CL_DEVICE_TYPE_GPU,8,d,&n)==CL_SUCCESS) for(cl_uint j=0;j<n&&nds<16;j++) ds[nds++]=d[j]; }
  if(!nds||dev>=(int)nds){ fprintf(stderr,"GPU bulunamadi\n"); return 4; }
  char dn[256]; clGetDeviceInfo(ds[dev],CL_DEVICE_NAME,256,dn,0); fprintf(stderr,"GPU: %s\n",dn);
  // yigin boyutu GPU belleginden: bchv parca (bchv*CH k) / yigin; adaylar icin capv girdilik 4 liste
  cl_ulong gmem=0; clGetDeviceInfo(ds[dev],CL_DEVICE_GLOBAL_MEM_SIZE,8,&gmem,0);
  int bchv = gmem>=(3ULL<<30)?8 : gmem>=(3ULL<<29)?4 : 2;
  { const char*e=getenv("SYMM27_BCH"); if(e&&atoi(e)>=1&&atoi(e)<=32) bchv=atoi(e); }
  // beklenen aday/k (maske asallari 41..PBIT icin bagimsizlik varsayimi) * 1.5 pay
  double esv=(double)nd; for(int p=0;p<nbit;p++){ int nb=0; for(size_t w=0;w<((size_t)pr[p]+63)/64;w++) nb+=__builtin_popcountll(bb[p][w]); esv*=1.0-(double)nb/pr[p]; }
  while(bchv>1 && 4.0*8.0*(double)bchv*CH*(esv*1.5+1000.0) > (double)gmem/3.0) bchv/=2;   // listeler GPU belleginin 1/3'unu asmasin
  cl_uint capv=(cl_uint)((double)bchv*CH*(esv*1.5+1000.0)); fprintf(stderr,"beklenen aday/k=%.0f\n",esv); fprintf(stderr,"GPU bellegi %.1f GB -> yigin=%d parca (%u k), aday tamponu %u\n",gmem/1073741824.0,bchv,(unsigned)bchv*CH,capv);
  cl_int er; cl_context ctx=clCreateContext(0,1,&ds[dev],0,0,&er); CK(er);
  cl_command_queue q=clCreateCommandQueue(ctx,ds[dev],0,&er); CK(er);
  double tk[4]={0,0,0,0}; cl_event ev=0;
#ifdef PROF
#define PROF(i) do{ cl_ulong s_,e_; clWaitForEvents(1,&ev); clGetEventProfilingInfo(ev,CL_PROFILING_COMMAND_START,8,&s_,0); clGetEventProfilingInfo(ev,CL_PROFILING_COMMAND_END,8,&e_,0); tk[i]+=(e_-s_)*1e-9; clReleaseEvent(ev);}while(0)
#else
#define PROF(i) do{ if(ev){ clReleaseEvent(ev); ev=0; } }while(0)
#endif
  cl_program prg=clCreateProgramWithSource(ctx,1,&KSRC,0,&er); CK(er);
  if(clBuildProgram(prg,1,&ds[dev],"",0,0)!=CL_SUCCESS){ char log[8192]; clGetProgramBuildInfo(prg,ds[dev],CL_PROGRAM_BUILD_LOG,sizeof log,log,0); fprintf(stderr,"derleme hatasi:\n%s\n",log); return 6; }
  cl_kernel ks=clCreateKernel(prg,"sieve1",&er); CK(er); cl_kernel kseg=clCreateKernel(prg,"seg",&er); CK(er); cl_kernel kmk=clCreateKernel(prg,"mkctab",&er); CK(er); cl_kernel kfm=clCreateKernel(prg,"fermat",&er); CK(er);
  #ifndef LS
  #define LS 64
  #endif
  cl_mem bdob=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,nd*8,dob,&er); CK(er);
  cl_mem bpr=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,np*4,prv,&er); CK(er);
  cl_mem bmg=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,np*8,pmg,&er); CK(er);
  cl_mem bof=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,np*4,boff,&er); CK(er);
  cl_mem bbt=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,tw*8,bits,&er); CK(er);
  uint64_t*moffh=malloc(np*8); size_t mtot=0; for(int p=0;p<nbit;p++){ moffh[p]=mtot; mtot+=(size_t)pr[p]*words; }
  cl_mem bmo=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,np*8,moffh,&er); CK(er);
  cl_mem bms=clCreateBuffer(ctx,CL_MEM_READ_ONLY,mtot*8,0,&er); CK(er);
  for(int p=0;p<nbit;p++){ CK(clEnqueueWriteBuffer(q,bms,CL_TRUE,moffh[p]*8,(size_t)pr[p]*words*8,mask[p],0,0,0)); free(mask[p]); mask[p]=0; }
  fprintf(stderr,"GPU maske bellegi: %.0f MB (nbit=%d)\n",mtot*8/1048576.0,nbit);
  #define FINMAX (1u<<17)
  #define NCNT 12
  uint32_t*pmh=malloc(np*4); for(int p=0;p<np;p++) pmh[p]=(uint32_t)pmod[p];
  cl_mem bpm=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,np*4,pmh,&er); CK(er);
  cl_uint cnp=(cl_uint)np, ccap=capv, cfin=FINMAX; (void)cnp;
  typedef struct { cl_mem ct,l[2],cnt[NCNT],rs; u2*hsv; uint8_t*hrs; cl_uint hc0,hcf; cl_event ev[4]; uint64_t ci,cend,a; } Slot;
  Slot S[2];
  for(int t=0;t<2;t++){
    S[t].ct=clCreateBuffer(ctx,CL_MEM_READ_WRITE,(size_t)bchv*CH*np*4,0,&er); CK(er);
    for(int i=0;i<2;i++){ S[t].l[i]=clCreateBuffer(ctx,CL_MEM_READ_WRITE,(size_t)capv*8,0,&er); CK(er); }
    for(int i=0;i<NCNT;i++){ S[t].cnt[i]=clCreateBuffer(ctx,CL_MEM_READ_WRITE,4,0,&er); CK(er); }
    S[t].rs=clCreateBuffer(ctx,CL_MEM_READ_WRITE,FINMAX,0,&er); CK(er);
    S[t].hsv=malloc((size_t)FINMAX*8); S[t].hrs=malloc(FINMAX); for(int i=0;i<4;i++) S[t].ev[i]=0;
  }
  // asal dilimleri (indeks): her dilimden sonra hayatta kalanlar sikistirilir (warp sapmasini onler)
  uint32_t segb[24]; int nseg=0; { uint32_t bnd[]={90,190,420,1000,0}; uint32_t cur=nbit; for(int i=0;;i++){ uint32_t e=bnd[i]?bnd[i]:np; if(e>(uint32_t)np) e=np; if(e>cur){ segb[nseg++]=cur; segb[nseg++]=e; cur=e; } if(!bnd[i]) break; } }
  nseg/=2; if(nseg+1>NCNT){ fprintf(stderr,"dilim sayisi\n"); return 8; }
  double tg0=wall(); time_t last=time(0); size_t ls=LS; cl_uint zero=0;
  int bchc=bchv, slot=0, pend=-1; uint64_t ci_next=committed;
  for(;;){
    int snew=-1;
    if(ci_next<nchunks){
      snew=slot; slot^=1; Slot*Z=&S[snew];
      uint64_t cend=ci_next+bchc; if(cend>nchunks) cend=nchunks;
      uint64_t a=k0+ci_next*CH, b=k0+cend*CH-1; if(b>k1||b<a) b=k1; cl_uint kc=(cl_uint)(b-a+1);
      Z->ci=ci_next; Z->cend=cend; Z->a=a; ci_next=cend;
      cl_ulong ka=a; cl_uint cwords=(cl_uint)words, cnb=(cl_uint)nbit; cl_ulong total=(cl_ulong)kc*words;
      CK(clSetKernelArg(kmk,0,8,&ka)); CK(clSetKernelArg(kmk,1,4,&kc)); CK(clSetKernelArg(kmk,2,4,&cnp)); CK(clSetKernelArg(kmk,3,sizeof(cl_mem),&bpr)); CK(clSetKernelArg(kmk,4,sizeof(cl_mem),&bpm)); CK(clSetKernelArg(kmk,5,sizeof(cl_mem),&Z->ct));
      size_t gm=((size_t)np+63)/64*64; CK(clEnqueueNDRangeKernel(q,kmk,1,0,&gm,&ls,0,0,0));
      for(int i=0;i<=nseg;i++) CK(clEnqueueWriteBuffer(q,Z->cnt[i],CL_FALSE,0,4,&zero,0,0,0));
      int ai=0; CK(clSetKernelArg(ks,ai++,4,&cwords)); CK(clSetKernelArg(ks,ai++,4,&cnb)); CK(clSetKernelArg(ks,ai++,4,&cnp)); CK(clSetKernelArg(ks,ai++,sizeof(cl_mem),&bmo)); CK(clSetKernelArg(ks,ai++,sizeof(cl_mem),&bms));
      CK(clSetKernelArg(ks,ai++,sizeof(cl_mem),&Z->ct)); CK(clSetKernelArg(ks,ai++,sizeof(cl_mem),&Z->cnt[0])); CK(clSetKernelArg(ks,ai++,sizeof(cl_mem),&Z->l[0])); CK(clSetKernelArg(ks,ai++,4,&ccap)); CK(clSetKernelArg(ks,ai++,8,&total));
      size_t gs=((size_t)total+ls-1)/ls*ls; CK(clEnqueueNDRangeKernel(q,ks,1,0,&gs,&ls,0,0,0));
      for(int sg=0;sg<nseg;sg++){
        cl_uint pb=segb[2*sg], pe=segb[2*sg+1]; int j=0;
        CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&bdob)); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&Z->l[sg&1])); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&Z->cnt[sg])); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&Z->l[(sg+1)&1])); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&Z->cnt[sg+1]));
        CK(clSetKernelArg(kseg,j++,4,&ccap)); CK(clSetKernelArg(kseg,j++,4,&pb)); CK(clSetKernelArg(kseg,j++,4,&pe)); CK(clSetKernelArg(kseg,j++,4,&cnp)); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&bpr)); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&bmg)); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&bof)); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&bbt)); CK(clSetKernelArg(kseg,j++,sizeof(cl_mem),&Z->ct));
        size_t g3=((size_t)capv+ls-1)/ls*ls; CK(clEnqueueNDRangeKernel(q,kseg,1,0,&g3,&ls,0,0,0)); }
      cl_mem Lf=Z->l[nseg&1];
      CK(clSetKernelArg(kfm,0,sizeof(cl_mem),&bdob)); CK(clSetKernelArg(kfm,1,sizeof(cl_mem),&Lf)); CK(clSetKernelArg(kfm,2,sizeof(cl_mem),&Z->cnt[nseg]));
      cl_ulong kb=a; CK(clSetKernelArg(kfm,3,8,&kb)); CK(clSetKernelArg(kfm,4,sizeof(cl_mem),&Z->rs)); CK(clSetKernelArg(kfm,5,4,&cfin));
      size_t g2=((size_t)FINMAX+ls-1)/ls*ls; CK(clEnqueueNDRangeKernel(q,kfm,1,0,&g2,&ls,0,0,0));
      CK(clEnqueueReadBuffer(q,Z->cnt[0],CL_FALSE,0,4,&Z->hc0,0,0,&Z->ev[0])); CK(clEnqueueReadBuffer(q,Z->cnt[nseg],CL_FALSE,0,4,&Z->hcf,0,0,&Z->ev[1]));
      CK(clEnqueueReadBuffer(q,Lf,CL_FALSE,0,(size_t)FINMAX*8,Z->hsv,0,0,&Z->ev[2])); CK(clEnqueueReadBuffer(q,Z->rs,CL_FALSE,0,FINMAX,Z->hrs,0,0,&Z->ev[3]));
      CK(clFlush(q));
    }
    if(pend>=0){
      Slot*Z=&S[pend]; CK(clWaitForEvents(4,Z->ev)); for(int i=0;i<4;i++){ clReleaseEvent(Z->ev[i]); Z->ev[i]=0; }
      if(Z->hc0>capv || Z->hcf>FINMAX || Z->hcf>Z->hc0){
        if(bchc==1){ fprintf(stderr,"aday tamponu tasti (%u/%u)\n",Z->hc0,Z->hcf); return 7; }
        fprintf(stderr,"uyari: aday tamponu tasti (%u/%u), parca kucultuluyor\n",Z->hc0,Z->hcf);
        clFinish(q); if(snew>=0) for(int i=0;i<4;i++) if(S[snew].ev[i]){ clReleaseEvent(S[snew].ev[i]); S[snew].ev[i]=0; }
        ci_next=Z->ci; pend=-1; bchc=bchc/2; if(bchc<1) bchc=1; slot=0; continue; }
      cl_uint cnt=Z->hcf; uint64_t lf=0; int lh=0; char (*H)[160]=NULL; int hcap=0;
      u2*cand=malloc((size_t)(cnt?cnt:1)*8); size_t nc=0;
      for(cl_uint j=0;j<cnt;j++){ if(Z->hrs[j]&1) lf++; if(Z->hrs[j]&2) cand[nc++]=Z->hsv[j]; }
      qsort(cand,nc,8,cmp_u2);
      for(size_t j=0;j<nc;j++){ u128 X=(u128)(Z->a+cand[j].a)*P37+dob[cand[j].b]; int v=verify(X);
        if(v){ if(lh==hcap){ hcap=hcap?hcap*2:4; H=realloc(H,(size_t)hcap*160); } char s[64]; pr128(s,X);
          snprintf(H[lh++],160,"%s %d",s,v); } }
      free(cand);
      surv+=cnt; fpass+=lf; for(int i=0;i<lh&&nh<MAXHITS;i++) memcpy(hits[nh++],H[i],160); free(H); committed=Z->cend;
#ifdef USE_BOINC
      boinc_fraction_done((double)committed/(double)nchunks);
      if(boinc_time_to_checkpoint()){ save_ckpt(cp); boinc_checkpoint_completed(); }
#else
      if(time(0)-last>=10){ last=time(0); save_ckpt(cp); }
#endif
    }
    pend=snew; if(pend<0) break;
  }
  double tt=wall()-tg0; uint64_t kdone=(k1-k0+1);
  fprintf(stderr,"GPU: %.2f s k/s=%.0f\n",tt,kdone/tt);
  FILE*fo=fopen(outp,"w"); if(!fo) return 2;
  fprintf(fo,"symm27 v%d k=[%llu,%llu] dobs=%zu surv=%llu fermat1=%llu hits=%d\n",APPVER,(unsigned long long)k0,(unsigned long long)k1,nd,(unsigned long long)surv,(unsigned long long)fpass,nh);
  write_hits(fo,hits,nh);
  fclose(fo); remove(cp);
#ifdef USE_BOINC
  boinc_finish(0);
#endif
  return 0;
}
