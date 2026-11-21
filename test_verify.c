#define main symm27_main
#include "symm27.c"
#undef main
#include <assert.h>
static int tot=0,bad_=0;
static void chk(const char*n,int got,int want){ tot++; if(got!=want){bad_++;} printf("%-52s %s (beklenen %d, cikan %d)\n",n,got==want?"OK ":"HATA",want,got); }
int main(){
  // 18713: 0,6,18,30,36 simetrik 5'li, ardisik asallar. Merkez 3'lu = 6,18,30 (ofset 6)
  int f5[5]={0,6,18,30,36}, c3[3]={0,12,24};
  chk("5'li 18713, merkez 3'lu -> tam (2)", verify_gen(18719,c3,3,f5,5,6),2);
  int f5x[5]={0,6,18,30,42};
  chk("tam desen yanlis (son eleman) -> sadece merkez (1)", verify_gen(18719,c3,3,f5x,5,6),1);
  int f5y[5]={0,6,18,36,48};
  chk("tam desen atlanmis asal (30 arada) -> (1)", verify_gen(18719,c3,3,f5y,5,6),1);
  int c3x[3]={0,12,26};
  chk("merkez asal degil -> 0", verify_gen(18719,c3x,3,f5,5,6),0);
  // ardisiklik: 0,18,36 desenini 18713 uzerine dene: 18731 ve 18749 asal ama arada 18719.. asallar var
  int f3[3]={0,18,36};
  chk("ardisik degil (arada asal var) -> 0", verify_gen(18713,f3,3,f3,3,0),0);
  // 8'li simetrik ornek (spec): 17: 0,2,6,12,14,20,24,26, merkez 6'li = 2,6,12,14,20,24
  int f8[8]={0,2,6,12,14,20,24,26}, c6[6]={0,4,10,12,18,22};
  chk("8'li 17:0,2,6,12,14,20,24,26 merkez 6'li -> tam (2)", verify_gen(19,c6,6,f8,8,2),2);
  printf("\n%d test, %d hata\n",tot,bad_); return bad_!=0;
}
