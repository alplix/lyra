// Symmetric 27-tuplets - BOINC uygulamasi (tek is parcacigi, checkpoint'li)
// Girdi dosyasi "in":  "kstart kend"
// Cikti dosyasi "out": basliktaki sayaclar + bulunan 11'liler (artan sirada)
// Derleme: bkz. Makefile  (-DUSE_BOINC ile BOINC API kullanilir, yoksa bagimsiz calisir)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#ifdef USE_BOINC
#include "boinc_api.h"
#endif
#ifndef PBIT
#define PBIT 200
#endif
#define PMAX 20000
#define APPVER 2
static const uint64_t P37 = 7420738134810ULL;
static const int pt11[11] = {0,30,48,54,78,84,90,114,120,138,168};
static const int pt27[27] = {0,6,12,36,90,96,102,120,132,162,180,186,210,216,222,246,252,270,300,312,330,336,342,396,420,426,432};
static int np, pr[2400], nbit; static size_t words, nd;
static uint8_t *bad[2400]; static uint64_t *bb[2400], *mask[2400], pmod[2400], pmg[2400];
static uint64_t *dob;

// ---- dob uretimi: 27 desenine gore uyumlu, x = 11'linin ilk asali, x mod 37# ----
static int cmpu(const void*a,const void*b){uint64_t x=*(const uint64_t*)a,y=*(const uint64_t*)b;return x<y?-1:x>y;}
static uint64_t inv_mod(uint64_t a,uint64_t p){ uint64_t r=1,e=p-2; a%=p; while(e){ if(e&1) r=r*a%p; a=a*a%p; e>>=1;} return r; }
static void gen_dob(void){
  static const int sp[12]={2,3,5,7,11,13,17,19,23,29,31,37};
  uint64_t *res=malloc(8),*nw; size_t n=1; res[0]=0; uint64_t M=1;
  for(int i=0;i<12;i++){ int p=sp[i]; int ok[64],nok=0; char b[64]={0};
    for(int j=0;j<27;j++) b[((132-pt27[j])%p+p)%p]=1;
    for(int r=0;r<p;r++) if(!b[r]) ok[nok++]=r;
    uint64_t inv=inv_mod(M,p); nw=malloc(n*nok*8); size_t c=0;
    for(size_t a=0;a<n;a++) for(int o=0;o<nok;o++){ uint64_t t=(((uint64_t)ok[o]+p-res[a]%p)%p)*inv%p; nw[c++]=res[a]+M*t; }
    free(res); res=nw; n=c; M*=p; }
  qsort(res,n,8,cmpu); dob=res; nd=n;
}
// ---- yardimcilar ----
// 128-bit tamsayi: GMP yok, __int128 zorunlu degil (32-bit hedeflerde de ayni sonuc)
typedef struct { uint64_t lo, hi; } n128;
static inline void mul64(uint64_t a,uint64_t b,uint64_t*hi,uint64_t*lo){
#if defined(__SIZEOF_INT128__) && !defined(LYRA_NO_INT128)
  unsigned __int128 p=(unsigned __int128)a*b; *lo=(uint64_t)p; *hi=(uint64_t)(p>>64);
#else
  uint64_t a0=(uint32_t)a,a1=a>>32,b0=(uint32_t)b,b1=b>>32;
  uint64_t p00=a0*b0,p01=a0*b1,p10=a1*b0,p11=a1*b1;
  uint64_t mid=(p00>>32)+(uint32_t)p01+(uint32_t)p10;
  *lo=(mid<<32)|(uint32_t)p00; *hi=p11+(p01>>32)+(p10>>32)+(mid>>32);
#endif
}
static inline uint64_t mulhi64(uint64_t a,uint64_t b){ uint64_t h,l; mul64(a,b,&h,&l); return h; }
static inline n128 mk(uint64_t lo){ n128 r={lo,0}; return r; }
static inline n128 add_u(n128 a,uint64_t d){ n128 r; r.lo=a.lo+d; r.hi=a.hi+(r.lo<a.lo); return r; }
static inline n128 sub_u(n128 a,uint64_t d){ n128 r; r.lo=a.lo-d; r.hi=a.hi-(a.lo<d); return r; }
static inline n128 sub128(n128 a,n128 b){ n128 r; r.lo=a.lo-b.lo; r.hi=a.hi-b.hi-(a.lo<b.lo); return r; }
static inline int ge128(n128 a,n128 b){ return a.hi>b.hi||(a.hi==b.hi&&a.lo>=b.lo); }
static inline int eq128(n128 a,n128 b){ return a.lo==b.lo&&a.hi==b.hi; }
static inline n128 dbl128(n128 a){ n128 r; r.hi=(a.hi<<1)|(a.lo>>63); r.lo=a.lo<<1; return r; }
static n128 mkX(uint64_t k,uint64_t d){ uint64_t h,l; mul64(k,P37,&h,&l); n128 r={l,h}; return add_u(r,d); }
static void pr128(char *s, n128 v){
  uint32_t w[4]={(uint32_t)v.lo,(uint32_t)(v.lo>>32),(uint32_t)v.hi,(uint32_t)(v.hi>>32)}; char t[48]; int n=0;
  for(;;){ if(!(w[0]|w[1]|w[2]|w[3])) break; uint64_t rem=0;
    for(int i=3;i>=0;i--){ uint64_t cur=(rem<<32)|w[i]; w[i]=(uint32_t)(cur/10); rem=cur%10; }
    t[n++]=(char)('0'+rem); }
  if(!n) t[n++]='0';
  for(int i=0;i<n;i++) s[i]=t[n-1-i]; s[n]=0;
}
static n128 parse128(const char*s){ n128 v={0,0};
  while(*s>='0'&&*s<='9'){ uint64_t h,l; mul64(v.lo,10,&h,&l); v.hi=v.hi*10+h; v.lo=l; v=add_u(v,(uint64_t)(*s++-'0')); }
  return v; }
// Montgomery (R=2^128), n tek ve n < 2^127
typedef struct { n128 n; uint64_t ninv; n128 one, r2; } mont;
static n128 mmul(n128 a,n128 b,const mont*M){
  uint64_t A[2]={a.lo,a.hi},B[2]={b.lo,b.hi},N[2]={M->n.lo,M->n.hi},p[5]={0,0,0,0,0};
  for(int i=0;i<2;i++){ uint64_t carry=0;
    for(int j=0;j<2;j++){ uint64_t h,l; mul64(A[j],B[i],&h,&l); uint64_t t=p[i+j]+l; uint64_t c=t<l; t+=carry; c+=t<carry; p[i+j]=t; carry=h+c; }
    p[i+2]+=carry; }
  for(int i=0;i<2;i++){ uint64_t m=p[i]*M->ninv, carry=0;
    for(int j=0;j<2;j++){ uint64_t h,l; mul64(m,N[j],&h,&l); uint64_t t=p[i+j]+l; uint64_t c=t<l; t+=carry; c+=t<carry; p[i+j]=t; carry=h+c; }
    for(int k=i+2;k<5&&carry;k++){ p[k]+=carry; carry=p[k]<carry; } }
  n128 r={p[2],p[3]}; if(ge128(r,M->n)) r=sub128(r,M->n); return r;
}
static inline n128 mdbl(n128 x,const n128 n){ x=dbl128(x); if(ge128(x,n)) x=sub128(x,n); return x; }
static void mont_init(mont*M,n128 n,int need_r2){
  M->n=n; uint64_t x=n.lo; for(int i=0;i<5;i++) x*=2-n.lo*x; M->ninv=(uint64_t)0-x;
  n128 r=mk(1); for(int i=0;i<128;i++) r=mdbl(r,n); M->one=r;
  if(need_r2){ for(int i=0;i<128;i++) r=mdbl(r,n); M->r2=r; }
}
static inline int bitlen128(n128 a){ int b=0; uint64_t w=a.hi?a.hi:a.lo; int base=a.hi?64:0; while(w){b++;w>>=1;} return b?base+b:0; }
static inline int bit128(n128 a,int i){ return i>=64?(int)((a.hi>>(i-64))&1):(int)((a.lo>>i)&1); }
// Fermat taban 2: 2^(n-1) = 1 (mod n), n tek, n > 2
static int fermat2(n128 n){
  mont M; mont_init(&M,n,0); n128 e=sub_u(n,1); int L=bitlen128(e);
  n128 x=mdbl(M.one,n);
  for(int i=L-2;i>=0;i--){ x=mmul(x,x,&M); if(bit128(e,i)) x=mdbl(x,n); }
  return eq128(x,M.one);
}
static uint64_t mod128_u(n128 n,uint64_t p){ uint64_t t=(UINT64_MAX%p+1)%p; return (uint64_t)((((n.hi%p)*t)%p+n.lo%p)%p); }
// Miller-Rabin, ilk 20 asal taban (n < 3.3e24 icin ilk 13 taban kesin; ustu olasiliksal, yaniltma ihtimali ihmal edilebilir)
static int isp(n128 n){
  static const int sp[20]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71};
  if(n.hi==0&&n.lo<2) return 0;
  for(int i=0;i<20;i++){ if(n.hi==0&&n.lo==(uint64_t)sp[i]) return 1; if(mod128_u(n,(uint64_t)sp[i])==0) return 0; }
  if(n.hi==0&&n.lo<73ULL*73ULL) return 1;
  mont M; mont_init(&M,n,1); n128 d=sub_u(n,1); int s=0;
  while(!bit128(d,0)){ d.lo=(d.lo>>1)|(d.hi<<63); d.hi>>=1; s++; }
  n128 negone=sub128(n,M.one); int L=bitlen128(d);
  for(int b=0;b<20;b++){
    n128 am=mmul(mk((uint64_t)sp[b]),M.r2,&M), x=M.one;
    for(int i=L-1;i>=0;i--){ x=mmul(x,x,&M); if(bit128(d,i)) x=mmul(x,am,&M); }
    if(eq128(x,M.one)||eq128(x,negone)) continue;
    int ok=0; for(int r=1;r<s;r++){ x=mmul(x,x,&M); if(eq128(x,negone)){ok=1;break;} }
    if(!ok) return 0; }
  return 1;
}
static int consecutive(n128 x,const int*pt,int n){ for(int i=0;i+1<n;i++) for(int d=pt[i]+2;d<pt[i+1];d+=2){ if(isp(add_u(x,(uint64_t)d))) return 0; } return 1; }
// genel dogrulama: merkez desen (pc,nc) X'te; tam desen (pf,nf) X-off'ta baslar
// donus: 0 = merkez degil, 1 = merkez (ardisik), 2 = tam desen de ardisik asal
static int verify_gen(n128 X,const int*pc,int nc,const int*pf,int nf,int off){
  int res=0, ok=1;
  for(int i=0;i<nc&&ok;i++){ if(!isp(add_u(X,(uint64_t)pc[i]))) ok=0; }
  if(ok && consecutive(X,pc,nc)){ res=1; n128 y=sub_u(X,(uint64_t)off); int f=1;
    for(int i=0;i<nf&&f;i++){ if(!isp(add_u(y,(uint64_t)pf[i]))) f=0; }
    if(f && consecutive(y,pf,nf)) res=2; }
  return res;
}
// donus: 0 = merkez 11'li degil; aksi halde en uzun simetrik ardisik uzanti uzunlugu (11,13,...,27)
static int verify(n128 X){
  int L=0, ok=1;
  for(int i=0;i<11&&ok;i++){ if(!isp(add_u(X,(uint64_t)pt11[i]))) ok=0; }
  if(ok && consecutive(X,pt11,11)){ L=11; n128 y=sub_u(X,132);
    for(int h=6;h<=13;h++){ int a=pt27[13-h], b=pt27[13+h]; int f=1;
      if(!isp(add_u(y,(uint64_t)a))) f=0;
      if(f && !isp(add_u(y,(uint64_t)b))) f=0;
      for(int d=a+2;f&&d<pt27[14-h];d+=2){ if(isp(add_u(y,(uint64_t)d))) f=0; }
      for(int d=pt27[12+h]+2;f&&d<b;d+=2){ if(isp(add_u(y,(uint64_t)d))) f=0; }
      if(!f) break; L=2*h+1; } }
  return L;
}
// sonuc dosyasi: once tum 11'liler ("X,"), sonra 13..27'li uzantilar ("X: offsetler")
static void write_hits(FILE*fo,char (*H)[160],int n){
  for(int i=0;i<n;i++){ char s[64]; sscanf(H[i],"%63s",s); fprintf(fo,"%s,\n",s); }
  for(int L=13;L<=27;L+=2){ int h=(L-1)/2, first=1;
    for(int i=0;i<n;i++){ char s[64]; int l; sscanf(H[i],"%63s %d",s,&l); if(l<L) continue;
      if(first){ fprintf(fo,"%d-tuplet\n",L); first=0; }
      n128 st=add_u(sub_u(parse128(s),132),(uint64_t)pt27[13-h]); char t[64]; pr128(t,st); fprintf(fo,"%s: ",t);
      for(int j=13-h;j<=13+h;j++) fprintf(fo,"%d%s",pt27[j]-pt27[13-h],j<13+h?", ":"\n"); } }
}
static int nthreads=1;
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
static int bnext=0;
static void *build(void*arg){
  (void)arg;
  for(;;){ pthread_mutex_lock(&mu); int p=bnext++; pthread_mutex_unlock(&mu); if(p>=nbit) break; int q=pr[p];
    uint16_t *t=malloc(nd*2); for(size_t i=0;i<nd;i++) t[i]=(uint16_t)(dob[i]%q); mask[p]=calloc((size_t)q*words,8);
    for(int c=0;c<q;c++){ uint64_t *Mc=mask[p]+(size_t)c*words; for(size_t i=0;i<nd;i++){ uint32_t r=c+t[i]; if(r>=(uint32_t)q) r-=q; Mc[i>>6]|=(uint64_t)(!bad[p][r])<<(i&63); } }
    free(t); }
  return NULL;
}
static void setup(void){
  gen_dob();
#ifndef USE_BOINC
  { const char*df=getenv("SYMM27_DOBFILE"); if(df){ FILE*f=fopen(df,"rb"); if(f){ fseek(f,0,SEEK_END); nd=ftell(f)/8; rewind(f); free(dob); dob=malloc(nd*8); if(fread(dob,8,nd,f)!=nd) exit(3); fclose(f);} } }
#endif
  words=(nd+63)/64;
  static char comp[PMAX+1];
  for(int i=2;i<=PMAX;i++){ if(comp[i]) continue; for(int j=2*i;j<=PMAX;j+=i) comp[j]=1; if(i>=41) pr[np++]=i; }
  nbit=0; while(nbit<np && pr[nbit]<=PBIT) nbit++;
  for(int p=0;p<np;p++){ int q=pr[p]; bad[p]=calloc(q,1); for(int i=0;i<11;i++) bad[p][(q-pt11[i]%q)%q]=1;
    bb[p]=calloc((q+63)/64,8); for(int r=0;r<q;r++) if(bad[p][r]) bb[p][r>>6]|=1ULL<<(r&63);
    pmod[p]=P37%q; pmg[p]=UINT64_MAX/q+1; if(p>=nbit){ free(bad[p]); bad[p]=0; } }
  pthread_t th[64]; for(int i=0;i<nthreads;i++) pthread_create(&th[i],0,build,0); for(int i=0;i<nthreads;i++) pthread_join(th[i],0);
  for(int p=0;p<nbit;p++){ free(bad[p]); bad[p]=0; }
}
// ---- dosya islemleri ----
static void rpath(const char*logical,char*out,size_t n){
#ifdef USE_BOINC
  boinc_resolve_filename(logical,out,(int)n);
#else
  snprintf(out,n,"%s",logical);
#endif
}
#define MAXHITS 100000
#define CH 256            // k / parca: sonuclar parca sirasiyla birlestirilir (cikti is parcacigi sayisindan bagimsiz)
#define RING 1024
static char (*hits)[160]; static int nh;
static uint64_t k0,k1,surv,fpass;       // surv/fpass: yalnizca birlesmis (tamamlanmis) parcalar
static uint64_t nchunks, committed, next_chunk;
typedef struct { int ready; uint64_t surv,fpass; int nh; char (*h)[160]; } chres;
static chres ring[RING];
static void save_ckpt(const char*cp){   // mu kilidi altinda cagrilir
  char tmp[600]; snprintf(tmp,sizeof tmp,"%s.tmp",cp); FILE*f=fopen(tmp,"wb"); if(!f) return;
  fprintf(f,"%d %llu %llu %llu %llu %llu %d\n",APPVER,(unsigned long long)k0,(unsigned long long)k1,(unsigned long long)(k0+committed*CH),(unsigned long long)surv,(unsigned long long)fpass,nh);
  for(int i=0;i<nh;i++) fprintf(f,"%s\n",hits[i]);
  fclose(f);
#ifdef _WIN32
  remove(cp);   // Windows: rename does not overwrite
#endif
  rename(tmp,cp);
}
static int load_ckpt(const char*cp){
  FILE*f=fopen(cp,"rb"); if(!f) return 0; int v,n; unsigned long long a,b,c,d,e;
  if(fscanf(f,"%d %llu %llu %llu %llu %llu %d\n",&v,&a,&b,&c,&d,&e,&n)!=7||v!=APPVER||a!=k0||b!=k1||n<0||n>=MAXHITS||c<k0||(c-k0)%CH){fclose(f);return 0;}
  nh=0; char line[200]; for(int i=0;i<n;i++){ if(!fgets(line,sizeof line,f)){fclose(f);nh=0;return 0;} line[strcspn(line,"\n")]=0; strncpy(hits[nh++],line,159); }
  committed=(c-k0)/CH; surv=d; fpass=e; fclose(f); return 1;
}
// ---- is parcacigi: parca basina k dongusu ----
static void *worker(void*arg){
  (void)arg; uint32_t*ix=malloc(nd*4); uint64_t*acc=malloc(words*8),*dv=malloc(nd*8);
  for(;;){
    pthread_mutex_lock(&mu);
    while(next_chunk<nchunks && next_chunk-committed>=RING) pthread_cond_wait(&cv,&mu);
    if(next_chunk>=nchunks){ pthread_mutex_unlock(&mu); break; }
    uint64_t ci=next_chunk++; pthread_mutex_unlock(&mu);
    uint64_t a=k0+ci*CH, b=a+CH-1; if(b>k1||b<a) b=k1;
    uint64_t ls=0,lf=0; int lh=0,cap=0; char (*H)[160]=NULL;
    for(uint64_t k=a;k<=b;k++){
      size_t m=0; uint32_t q=pr[0]; uint32_t c=(uint32_t)(((k%q)*pmod[0])%q); memcpy(acc,mask[0]+(size_t)c*words,words*8);
      for(int p=1;p<nbit;p++){ q=pr[p]; c=(uint32_t)(((k%q)*pmod[p])%q); const uint64_t*Mp=mask[p]+(size_t)c*words; for(size_t w=0;w<words;w++) acc[w]&=Mp[w]; }
      for(size_t w=0;w<words;w++){ uint64_t v=acc[w]; while(v){ ix[m++]=(uint32_t)(w*64+__builtin_ctzll(v)); v&=v-1; } }
      for(size_t j=0;j<m;j++) dv[j]=dob[ix[j]];
      for(int p=nbit;p<np && m;p++){
        uint32_t qq=pr[p]; uint64_t cc=((k%qq)*pmod[p])%qq; const uint64_t*B=bb[p]; uint64_t mg=pmg[p]; size_t n2=0;
        for(size_t j=0;j<m;j++){ uint64_t v=dv[j]; uint64_t qt=mulhi64(v,mg); uint64_t r=v-qt*qq+cc; r-=(r>=qq)?qq:0; dv[n2]=v; n2+=!((B[r>>6]>>(r&63))&1); }
        m=n2; }
      ls+=m;
      for(size_t j=0;j<m;j++){
        n128 X=mkX(k,dv[j]);
        int ok=1; for(int i=0;i<11&&ok;i++){ ok=fermat2(add_u(X,(uint64_t)pt11[i])); if(i==0&&ok) lf++; }
        if(ok){ int v=verify(X); if(v){ if(lh==cap){ cap=cap?cap*2:4; H=realloc(H,(size_t)cap*160); } char s[64]; pr128(s,X); snprintf(H[lh++],160,"%s %d",s,v); } }
      }
    }
    pthread_mutex_lock(&mu);
    chres*r=&ring[ci%RING]; r->ready=1; r->surv=ls; r->fpass=lf; r->nh=lh; r->h=H;
    while(committed<nchunks && ring[committed%RING].ready){ chres*c=&ring[committed%RING];
      surv+=c->surv; fpass+=c->fpass; for(int i=0;i<c->nh && nh<MAXHITS;i++) memcpy(hits[nh++],c->h[i],160);
      free(c->h); c->h=NULL; c->ready=0; committed++; }
    pthread_cond_broadcast(&cv); pthread_mutex_unlock(&mu);
  }
  free(ix); free(acc); free(dv); return NULL;
}
static int want_ckpt(void){
#ifdef USE_BOINC
  return boinc_time_to_checkpoint();
#else
  static time_t last=0; time_t n=time(0); if(!last) last=n; if(n-last>=10){ last=n; return 1; } return 0;
#endif
}
int main(int argc,char**argv){
  char inp[512],outp[512],cp[512]; const char*pos[3]={"in","out","checkpoint"}; int np_=0;
#ifdef USE_BOINC
  int rc=boinc_init(); if(rc) return rc;
#endif
  for(int i=1;i<argc;i++){
    if(!strcmp(argv[i],"--nthreads")||!strcmp(argv[i],"-nthreads")){ if(i+1<argc) nthreads=atoi(argv[++i]); }
    else if(np_<3) pos[np_++]=argv[i];
  }
  if(nthreads<1) nthreads=1; if(nthreads>64) nthreads=64;
  rpath(pos[0],inp,sizeof inp); rpath(pos[1],outp,sizeof outp); rpath(pos[2],cp,sizeof cp);
  FILE*fi=fopen(inp,"r"); unsigned long long a,b;
  if(!fi||fscanf(fi,"%llu %llu",&a,&b)!=2||a>b){ fprintf(stderr,"girdi okunamadi\n");
#ifdef USE_BOINC
    boinc_finish(1);
#endif
    return 1; }
  fclose(fi); k0=a; k1=b; hits=malloc((size_t)MAXHITS*160);
  nchunks=(k1-k0)/CH+1;
  load_ckpt(cp);
  next_chunk=committed;
  setup();
#ifdef DUMPDOB
  { FILE*d=fopen("dob_c.bin","wb"); fwrite(dob,8,nd,d); fclose(d); return 0; }
#endif
  pthread_t th[64]; for(int i=0;i<nthreads;i++) pthread_create(&th[i],0,worker,0);
  pthread_mutex_lock(&mu);
  while(committed<nchunks){
    struct timespec ts; clock_gettime(CLOCK_REALTIME,&ts); ts.tv_sec+=1; pthread_cond_timedwait(&cv,&mu,&ts);
#ifdef USE_BOINC
    boinc_fraction_done((double)committed/(double)nchunks);
#endif
    if(committed<nchunks && want_ckpt()){ save_ckpt(cp);
#ifdef USE_BOINC
      boinc_checkpoint_completed();
#endif
    }
  }
  pthread_mutex_unlock(&mu);
  for(int i=0;i<nthreads;i++) pthread_join(th[i],0);
  FILE*fo=fopen(outp,"wb"); if(!fo) return 2;
  fprintf(fo,"symm27 v%d k=[%llu,%llu] dobs=%zu surv=%llu fermat1=%llu hits=%d\n",APPVER,(unsigned long long)k0,(unsigned long long)k1,nd,(unsigned long long)surv,(unsigned long long)fpass,nh);
  write_hits(fo,hits,nh);
  fclose(fo); remove(cp);
#ifdef USE_BOINC
  boinc_finish(0);
#endif
  return 0;
}
