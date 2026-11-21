// Symmetric 27-tuplets - BOINC uygulamasi (tek is parcacigi, checkpoint'li)
// Girdi dosyasi "in":  "kstart kend"
// Cikti dosyasi "out": basliktaki sayaclar + bulunan 11'liler (artan sirada)
// Derleme: bkz. Makefile  (-DUSE_BOINC ile BOINC API kullanilir, yoksa bagimsiz calisir)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gmp.h>
#include <pthread.h>
#include <time.h>
#ifdef USE_BOINC
#include "boinc_api.h"
#endif
typedef unsigned __int128 u128;
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
static int isp(const mpz_t n){ return mpz_probab_prime_p(n,1)!=0; }
static int fermat2(const mpz_t n, mpz_t a, mpz_t e, mpz_t r){ mpz_sub_ui(e,n,1); mpz_set_ui(a,2); mpz_powm(r,a,e,n); return mpz_cmp_ui(r,1)==0; }
static void u128_to_mpz(mpz_t r, u128 v){ mpz_set_ui(r,(uint64_t)(v>>64)); mpz_mul_2exp(r,r,64); mpz_add_ui(r,r,(uint64_t)v); }
static void pr128(char *s, u128 v){ char t[64]; int n=0; if(!v){strcpy(s,"0");return;} while(v){t[n++]='0'+(int)(v%10); v/=10;} for(int i=0;i<n;i++) s[i]=t[n-1-i]; s[n]=0; }
static int consecutive(const mpz_t x,const int*pt,int n,mpz_t t){ for(int i=0;i+1<n;i++) for(int d=pt[i]+2;d<pt[i+1];d+=2){ mpz_add_ui(t,x,d); if(isp(t)) return 0; } return 1; }
// genel dogrulama: merkez desen (pc,nc) X'te; tam desen (pf,nf) X-off'ta baslar
// donus: 0 = merkez degil, 1 = merkez (ardisik), 2 = tam desen de ardisik asal
static int verify_gen(u128 X,const int*pc,int nc,const int*pf,int nf,int off){
  mpz_t x,t,y; mpz_init(x); mpz_init(t); mpz_init(y); u128_to_mpz(x,X); int res=0, ok=1;
  for(int i=0;i<nc&&ok;i++){ mpz_add_ui(t,x,pc[i]); if(!isp(t)) ok=0; }
  if(ok && consecutive(x,pc,nc,t)){ res=1; mpz_sub_ui(y,x,off); int f=1;
    for(int i=0;i<nf&&f;i++){ mpz_add_ui(t,y,pf[i]); if(!isp(t)) f=0; }
    if(f && consecutive(y,pf,nf,t)) res=2; }
  mpz_clear(x); mpz_clear(t); mpz_clear(y); return res;
}
// donus: 0 = merkez 11'li degil; aksi halde en uzun simetrik ardisik uzanti uzunlugu (11,13,...,27)
static int verify(u128 X){
  mpz_t x,t,y; mpz_init(x); mpz_init(t); mpz_init(y); u128_to_mpz(x,X); int L=0, ok=1;
  for(int i=0;i<11&&ok;i++){ mpz_add_ui(t,x,pt11[i]); if(!isp(t)) ok=0; }
  if(ok && consecutive(x,pt11,11,t)){ L=11; mpz_sub_ui(y,x,132);
    for(int h=6;h<=13;h++){ int a=pt27[13-h], b=pt27[13+h]; int f=1;
      mpz_add_ui(t,y,a); if(!isp(t)) f=0;
      if(f){ mpz_add_ui(t,y,b); if(!isp(t)) f=0; }
      for(int d=a+2;f&&d<pt27[14-h];d+=2){ mpz_add_ui(t,y,d); if(isp(t)) f=0; }
      for(int d=pt27[12+h]+2;f&&d<b;d+=2){ mpz_add_ui(t,y,d); if(isp(t)) f=0; }
      if(!f) break; L=2*h+1; } }
  mpz_clear(x); mpz_clear(t); mpz_clear(y); return L;
}
// sonuc dosyasi: once tum 11'liler ("X,"), sonra 13..27'li uzantilar ("X: offsetler")
static u128 parse128(const char*s){ u128 v=0; while(*s>='0'&&*s<='9') v=v*10+(*s++-'0'); return v; }
static void write_hits(FILE*fo,char (*H)[160],int n){
  for(int i=0;i<n;i++){ char s[64]; sscanf(H[i],"%63s",s); fprintf(fo,"%s,\n",s); }
  for(int L=13;L<=27;L+=2){ int h=(L-1)/2, first=1;
    for(int i=0;i<n;i++){ char s[64]; int l; sscanf(H[i],"%63s %d",s,&l); if(l<L) continue;
      if(first){ fprintf(fo,"%d-tuplet\n",L); first=0; }
      u128 st=parse128(s)-132+pt27[13-h]; char t[64]; pr128(t,st); fprintf(fo,"%s: ",t);
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
  char tmp[600]; snprintf(tmp,sizeof tmp,"%s.tmp",cp); FILE*f=fopen(tmp,"w"); if(!f) return;
  fprintf(f,"%d %llu %llu %llu %llu %llu %d\n",APPVER,(unsigned long long)k0,(unsigned long long)k1,(unsigned long long)(k0+committed*CH),(unsigned long long)surv,(unsigned long long)fpass,nh);
  for(int i=0;i<nh;i++) fprintf(f,"%s\n",hits[i]);
  fclose(f); rename(tmp,cp);
}
static int load_ckpt(const char*cp){
  FILE*f=fopen(cp,"r"); if(!f) return 0; int v,n; unsigned long long a,b,c,d,e;
  if(fscanf(f,"%d %llu %llu %llu %llu %llu %d\n",&v,&a,&b,&c,&d,&e,&n)!=7||v!=APPVER||a!=k0||b!=k1||n<0||n>=MAXHITS||c<k0||(c-k0)%CH){fclose(f);return 0;}
  nh=0; char line[200]; for(int i=0;i<n;i++){ if(!fgets(line,sizeof line,f)){fclose(f);nh=0;return 0;} line[strcspn(line,"\n")]=0; strncpy(hits[nh++],line,159); }
  committed=(c-k0)/CH; surv=d; fpass=e; fclose(f); return 1;
}
// ---- is parcacigi: parca basina k dongusu ----
static void *worker(void*arg){
  (void)arg; uint32_t*ix=malloc(nd*4); uint64_t*acc=malloc(words*8),*dv=malloc(nd*8);
  mpz_t x,t,ta,te,tr; mpz_init(x); mpz_init(t); mpz_init(ta); mpz_init(te); mpz_init(tr);
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
        for(size_t j=0;j<m;j++){ uint64_t v=dv[j]; uint64_t qt=(uint64_t)(((u128)v*mg)>>64); uint64_t r=v-qt*qq+cc; r-=(r>=qq)?qq:0; dv[n2]=v; n2+=!((B[r>>6]>>(r&63))&1); }
        m=n2; }
      ls+=m;
      for(size_t j=0;j<m;j++){
        u128 X=(u128)k*P37+dv[j]; u128_to_mpz(x,X);
        int ok=1; for(int i=0;i<11&&ok;i++){ mpz_add_ui(t,x,pt11[i]); ok=fermat2(t,ta,te,tr); if(i==0&&ok) lf++; }
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
  mpz_clear(x); mpz_clear(t); mpz_clear(ta); mpz_clear(te); mpz_clear(tr); free(ix); free(acc); free(dv); return NULL;
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
  FILE*fo=fopen(outp,"w"); if(!fo) return 2;
  fprintf(fo,"symm27 v%d k=[%llu,%llu] dobs=%zu surv=%llu fermat1=%llu hits=%d\n",APPVER,(unsigned long long)k0,(unsigned long long)k1,nd,(unsigned long long)surv,(unsigned long long)fpass,nh);
  write_hits(fo,hits,nh);
  fclose(fo); remove(cp);
#ifdef USE_BOINC
  boinc_finish(0);
#endif
  return 0;
}
