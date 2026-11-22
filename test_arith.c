// GMP'siz aritmetigin GMP ile capraz testi (sadece test; uygulama GMP'ye bagli degil)
#include <gmp.h>
#define main app_main
#include "symm27.c"
#undef main
static uint64_t rs=88172645463325252ULL; static uint64_t rnd(void){ rs^=rs<<13; rs^=rs>>7; rs^=rs<<17; return rs; }
static void to_mpz(mpz_t r,n128 v){ mpz_set_ui(r,(unsigned long)(v.hi>>32)); mpz_mul_2exp(r,r,32); mpz_add_ui(r,r,(unsigned long)(v.hi&0xffffffffu)); mpz_mul_2exp(r,r,32); mpz_add_ui(r,r,(unsigned long)(v.lo>>32)); mpz_mul_2exp(r,r,32); mpz_add_ui(r,r,(unsigned long)(v.lo&0xffffffffu)); }
int main(void){
  int bad=0; mpz_t m,a,e,r; mpz_init(m); mpz_init(a); mpz_init(e); mpz_init(r);
  // 1) kucuk sayilar
  for(uint64_t n=0;n<200000;n++){ mpz_set_ui(m,n); int g=mpz_probab_prime_p(m,25)!=0; if(isp(mk(n))!=g){ if(bad++<5) printf("small isp %llu\n",(unsigned long long)n); } }
  // 2) rastgele 65..89 bit: fermat2 ve isp
  long np_=0; for(int it=0;it<300000;it++){
    int bits=65+(int)(rnd()%25); n128 n; n.lo=rnd()|1; n.hi=rnd()&((bits-64>=64)?~0ULL:((1ULL<<(bits-64))-1)); n.hi|=1ULL<<(bits-65);
    if(n.lo==0&&n.hi==0) continue; to_mpz(m,n);
    mpz_set_ui(a,2); mpz_sub_ui(e,m,1); mpz_powm(r,a,e,m); int gf=mpz_cmp_ui(r,1)==0;
    if(fermat2(n)!=gf){ if(bad++<5) printf("fermat2 mismatch\n"); }
    if(it%3==0){ int g=mpz_probab_prime_p(m,25)!=0; np_+=g; if(isp(n)!=g){ if(bad++<5) printf("isp mismatch\n"); } } }
  // 3) bilinen kuvvetli sahte asallar (hepsi bilesik, isp 0 vermeli)
  static const char*spsp[]={"561","3215031751","3825123056546413051","318665857834031151167461","3317044064679887385961981",0};
  for(int i=0;spsp[i];i++){ if(isp(parse128(spsp[i]))){ bad++; printf("SPSP accepted %s\n",spsp[i]); } }
  // 4) bilinen asallar ve Natalia'nin 19'lusu
  static const char*pr[]={"9425346484752129657862259","9425346484752129657862217","170141183460469231731687303715884105727",0};
  for(int i=0;pr[i];i++){ n128 v=parse128(pr[i]); if(v.hi>>63) continue; if(!isp(v)){ bad++; printf("prime rejected %s\n",pr[i]); } }
  // 5) pr128/parse128/mkX gidis-donus
  for(int it=0;it<100000;it++){ n128 v={rnd(),rnd()>>1}; char s[64]; pr128(s,v); n128 w=parse128(s); if(!eq128(v,w)){ bad++; printf("roundtrip\n"); break; }
    mpz_t z; mpz_init(z); to_mpz(z,v); char*gs=mpz_get_str(0,10,z); if(strcmp(gs,s)){ bad++; printf("pr128 differs\n"); } free(gs); mpz_clear(z); }
  for(int it=0;it<100000;it++){ uint64_t k=rnd()>>20, d=rnd()>>20; n128 x=mkX(k,d); mpz_set_ui(m,(unsigned long)k); mpz_mul_ui(m,m,7420738134810UL); mpz_add_ui(m,m,(unsigned long)d); mpz_t z; mpz_init(z); to_mpz(z,x); if(mpz_cmp(z,m)){ bad++; printf("mkX\n"); mpz_clear(z); break; } mpz_clear(z); }
  printf("asal sayisi (ornek): %ld, hata: %d\n",np_,bad); return bad!=0;
}
