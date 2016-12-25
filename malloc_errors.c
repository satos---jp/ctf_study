#include<stdio.h>
#include<stdlib.h>

const int psize = sizeof(void*);

void sh(void* p){ //show header
	printf("------------------malloc_chunk-------------------------\n");
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

void* main_arena_p = NULL;

unsigned int req2size(unsigned int x){
	return (x+psize+(psize*2-1))&~(psize*2-1);
}

unsigned int fastbin_index(unsigned int sz){
	return ((sz >> (psize == 8 ? 4 : 3)) - 2);
}

void show_arena(){
	printf("------------------arena_data--------------------------\n");
	const unsigned int max_fast_size = (80 * psize / 4); //x86なら0x50,x64なら0xa0
	const unsigned int nfastbins =  fastbin_index(req2size(max_fast_size)) + 1;
	//printf("nfastbins .. %d\n",nfastbins); 10で固定らしー。
	//printf("max_fast .. %p\n",*(void**)(main_arena_p + psize*2));
	
	//先頭にあるmutex,falgsの大きさはunsigned_intらしくて4byteの模様mutex,
	int suint = sizeof(unsigned int);
	int i,finded=0;
	for(i=0;i<nfastbins;i++){
		void* np = *(void**)(main_arena_p + suint*2 + psize*i);
		if(np==NULL)continue;
		finded=1;
		printf("fastbins[%d] .. %p\n",i,np);
		//0x78byteまで。fastbins[6]に繋がれるっぽい。
	}
	if(finded==0){
		printf("no chunks are connected to fastbins\n");
	}
	void* ptop = *(void**)(main_arena_p + suint*2 + psize*nfastbins);
	printf("top .. %p\n",ptop);
	printf("top_size .. %x\n",*(unsigned int*)(ptop+psize)); //topはチャンク的にtop.
	
	printf("last_reminder .. %p\n",*(void**)(main_arena_p + suint*2 + psize*(nfastbins+1)));
	
	const unsigned int nbins = 128;
	
	finded=0;
	for(i=0;i<nbins*2-2;i++){
		void* dnp = (void*)(main_arena_p + suint*2 + psize*(nfastbins+2 + i)); //npが入ってるアドレス
		void* np = *(void**)(main_arena_p + suint*2 + psize*(nfastbins+2 + i)); 
		if((i%2==0 && dnp==np+psize*2) ||         //いまはfdで、ちょうどnp->fd->bk==&npになってる
			 (i%2==1 && dnp==np+psize*3))continue;  //いまはfdで、ちょうどnp->fd->bk==&npになってる
		finded=1;
		if(i<=1){
			printf("unsorded chunks %s .. %p\n",i%2==0?"fd":"bk",np);
		}
		else printf("bins[%d](%s) .. %p\n",i,i%2==0?"fd":"bk",np);
		//0x78byteまで。fastbins[6]に繋がれるっぽい。
	}
	
	if(finded==0){
		printf("no unsorted_bins and bins\n");
	}
	puts("");
}

void init_main_arena(){
	//0x100くらいのものをfreeしたら、unsorted_chunksに繋がれるので、
	//そこからの相対位置で得る。
	void* a = malloc(0x100);
	void* b = malloc(0x100);
	free(a);
	//sh(a);

	main_arena_p = *(void**)(a+psize); // bck = unsorted_chunks(av); より、p->bkに入る。
	//main_arena_p -= 0x58; //相対アドレス。 x64だとここだった。
	
	const unsigned int max_fast_size = (80 * psize / 4); //x86なら0x50,x64なら0xa0
	const unsigned int nfastbins =  fastbin_index(req2size(max_fast_size)) + 1;
	int suint = sizeof(unsigned int);
	main_arena_p -= suint*2 + psize*nfastbins; //topのとこを指してるっぽい。(2こ下がちょうど自身)
	//printf("%p\n",main_arena_p);
	//printf("top from a .. %p\n",a-psize*2);
	free(b);
}

//<_int_free+336> .. test mmapped?

//malloc consolidate がかかる!!(heapといっしょだと)



//mallocのエラーをことごとく踏みぬいていくやつ。
//http://osxr.org:8080/glibc/source/malloc/malloc.c
//ここを参考に。


//fastbinsは、x64だと0x78まで。
//x32だと0x3cまで。

void sample2(){
	//show_arena();
	void *a,*b,*c,*d,*e,*f,*g;
	a = malloc(4000);
	sh(a);
	show_arena();
	return;
	a = malloc(0x400000);
	show_arena();
	sh(a);
	free(a);
	show_arena();
	a = malloc(0x400000);
	show_arena();
	sh(a);
	free(a);
	show_arena();
	return;
	a = malloc(0x1);
	malloc(0x1);
	b = malloc(0x1);
	malloc(0x1);
	c = malloc(0x1);
	malloc(0x1);
	e = malloc(0x1);
	free(a); free(b); free(c);
	puts("chunk a");
	//sh(a);
	puts("chunk b");
	//sh(b);
	puts("chunk c");
	//sh(c);
	show_arena();
	d = malloc(0x100);
	free(d); //ここでmalloc_consolidateが走る
	show_arena();
	f = malloc(0x100); //ここでunsorted_binsが正しいのに繋がれる
	show_arena();
	puts("ここまででbinsにつながる");
	free(e);
	show_arena();
	g = malloc(0x1);
	g = malloc(0x1);
	show_arena();
	//b = malloc(0x30);
	//c = malloc(0x300);
	//d = malloc(0x100);
	//sh(c);
	//free(c);
	//show_arena();
	
}

int main(){
	init_main_arena();
	
	sample2();
	return 0;
	//_int_mallocのエラー。

	//_int_freeのエラー。
	
	if(0){
		//free(): invalid pointer
		if(0){
			void* a = malloc(0x30);
			free(a+0x8);
		}
		else{
			void* a = malloc(0x30);  //p > -sizeだと起こる。
			*(size_t*)(a-psize) = 0x1;
			sh(a);
			free(a);
		}
	}
	if(0){
		//free(): invalid size
		void* a = malloc(0x30);
		void* b = malloc(0x30);
		*(size_t*)(a-psize) = 0x9;
		*(size_t*)(a-psize) = 0x49;
		sh(a);  sh(b);
		free(a);
	}
	if(0){
		//free(): invalid next size (fast)
		void* a = malloc(0x30);
		void* b = malloc(0x30);
		*(size_t*)(b-psize) = 0x9;
		//*(unsigned int*)(b-psize) = 0x1000001;
		sh(a);  sh(b);
		free(a);
	}
	if(0){
		//double free or corruption (fasttop)
		void* a = malloc(0x30);
		void* b = malloc(0x30);
		free(a);
		free(a);
	}
	if(0){
		//invalid fastbin entry (free) ..まだ動かない。
		void* a = malloc(0x30);
		void* b = malloc(0x30);
		void* c = malloc(0x30);
		void* d = malloc(0x30);
		free(a);
		*(size_t*)(a-psize) += 0x120;
		sh(a);
		free(c);
	}
	if(0){
		//double free or corruption (top)
		void* a = malloc(0x100);
		free(a);
		free(a);
	}
	if(0){
		//double free or corruption (out)  //動かねぇ...
		void* b[100];
		void* a[100];
		void* c[100];
		a[0] = (void*)0x30;
		a[1] = (void*)0x21;
		sh(a+2);
		void* t = a+2;
		//free(t);
		
		/*
		なんかこれで動いたがなんでや...
		void* a = malloc(0x30);
		void* b = malloc(0x30);
		*(unsigned int*)(a-psize) = 0x1000001;
		sh(a);  sh(b);
		free(a);
		*/
	}
	if(0){
		//double free or corruption (!prev)
		void* a = malloc(0x100);
		void* b = malloc(0x100);
		*(size_t*)(b-psize) ^= 0x1;
		free(a);
	}
	if(0){
		//free(): invalid next size (normal)
		void* a = malloc(0x100);
		void* b = malloc(0x100);
		*(size_t*)(b-psize) = 0x9;
		*(size_t*)(b-psize) = 0x1000001;
		sh(a);  sh(b);
		free(a);
	}
	return 0;
}



