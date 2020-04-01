/**************************************************
BFORTH

Copyright 2014 Brian O'Dell
**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define true 1
#define false 0

#define MAXTIB 96
#define MAXSTACK 24
#define MAXRSTACK 24
#define MAXWORD 11
#define MAXPFAS 5

#define PRIMITIVE(tok, func)		buildword(tok, true);			\
									dnext->cfa=&func;				\
									addword();

#define COMPOSITE(tok)				buildword(tok, false);			\
									pnew=malloc(sizeof(pfas));		\
									dnext->pfa=pnew;				\
									dnext->threaded=false;

#define COMPOSITEPFA(f0,f1,f2,f3,f4)		pnew->pfa[0]=&f0;			\
											pnew->pfa[1]=&f1;			\
											pnew->pfa[2]=&f2;			\
											pnew->pfa[3]=&f3;			\
											pnew->pfa[4]=&f4;

#define COMPOSITEMORE				pnew->next=malloc(sizeof(pfas));	\
									pnew->next->prev=pnew;				\
									pnew=pnew->next;					\
									pnewi=0;

#define COMPOSITEEND				addword();

#define DOWORD(w)					dofindw(w);						\
									pop();							\
									doexecute();

#define FINDWORD(w,p)				dofindw(w);pop();pop();p=d;


typedef struct pfas{
	void *pfa[MAXPFAS];
	struct pfas *prev;
	struct pfas *next;
} pfas;

typedef struct ditem{
	struct ditem *lfa;
	char nfa[MAXWORD];
	void *cfa;
	pfas *pfa;
	bool primitive;
	bool immediate;
	bool threaded;
} ditem;

bool state;
ditem *dlast, *dnext, *d;
pfas *p, *pnew;
int pi, pnewi;
bool donerunpfas;
void *stack[MAXSTACK];
int stackpointer;
void *rstack[MAXRSTACK];
int rstackpointer;
char tib[MAXTIB];
int in;
char word[MAXWORD];
bool bye;

void doexecute(void);

void push(void *x){
	stack[stackpointer++]=x;
}

void *pop(void){
	return stack[--stackpointer];
}

void rpush(void *x){
	rstack[rstackpointer++]=x;
}

void *rpop(void){
	return rstack[--rstackpointer];
}

void donop(void){
}

void dostate(void){
	push((void *)state);
}

void dodrop(void){
	pop();
}

void doswap(void){
	void *a=pop();
	void *b=pop();
	push(a);
	push(b);
}

void donot(void){
	int t=(int )pop();
	if(t){
		push((void *)0);
	}else{
		push((void *)1);
	}
}

void dofetch(void){
	push(pop());
}

void dostore(void){
	void *var=(void *)pop();
	var=(void *) pop();
}

void dodot(void){
	printf("%d", (int)pop());
}

void doexit(void){
	donerunpfas=true;
}

void dotib(void){
	push((void *)tib);
}

void doin(void){
	push((void *)in);
}

void donotib(void){
	push((void *)strlen(tib));
}

void resettib(void){
	tib[0]=0;
	in=0;
}

void doexpect(void){
	resettib();
	fgets(tib, MAXTIB, stdin);
	for(unsigned long long int i=0;i<strlen(tib); i++){
		if(tib[i]=='\n')
			tib[i]=0;
	}
}

void passws(void){
	bool w=false;
	while(w==false){
		if(tib[in]==' '){
			in++;
			if(in==MAXTIB){
				resettib();
				return;
			}
		}else{
			w=true;
		}
	}
}

void doword(void){
	bool w=true;
	int i=0;
	passws();
	while(w==true){
		if(tib[in]!=' '&&tib[in]!=0){
			word[i++]=tib[in++];
			if(i==MAXWORD){
				word[i]=0;
				passws();
				return;
			}
			if(in==MAXTIB){
				word[i]=0;
				resettib();
				return;
			}
		}else{
			w=false;
		}
	}
}

void doanother(void){
	passws();
	if(in==strlen(tib)){
		push((void *)0);
	}else{
		push((void *)1);
	}
}

void dofind(void){
	bool f=false;
	d=dlast;
	while(!f){
		if(strcmp(word, d->nfa)==0){
			f=true;
			push((void *) d);
			push((void *) 1);
			return;
		}else{
			if(d->lfa==0){
				push(0);
				return;
			}else{
				d=d->lfa;
			}
		}
	}
}

void dofindw(const char *w){
	bool f=false;
	d=dlast;
	while(!f){
		if(strcmp(w, d->nfa)==0){
			f=true;
			push((void *) d);
			push((void *) 1);
			return;
		}else{
			if(d->lfa==0){
				push(0);
				return;
			}else{
				d=d->lfa;
			}
		}
	}
}

void runpfas(void){
	void (*fpointer)(void);
	donerunpfas=false;
	while(!donerunpfas){
		for(pi; pi<MAXPFAS; pi++){
			fpointer=p->pfa[pi];
			fpointer();
			if(donerunpfas)
				return;
		}
		if(p->next!=0)
			p=p->next;
			pi=0;
	}
}

void runthreadedpfas(void){
	ditem *d0;
	donerunpfas=false;
	while(!donerunpfas){
		for(pi; pi<MAXPFAS; pi++){
			d0=p->pfa[pi];
			push((void *) d0);
			doexecute();
			if(donerunpfas)
				return;
		}
		if(p->next!=0)
			p=p->next;
			pi=0;
	}
}

void doexecute(void){
	void (*fpointer)(void);

	d=(ditem *) pop();
	if(!d->primitive){
		rpush((void *)p);
		rpush((void *)pi);
		rpush((void *) donerunpfas); 
		p=d->pfa;
		pi=0;
		if(!d->threaded){
			runpfas();
		}else{
			runthreadedpfas();
		}
		donerunpfas=(bool )rpop();
		pi=(int)rpop();
		p=(void *)rpop();
	}else{
		fpointer=d->cfa;
		fpointer();
	}
}

void doif(void){
	ditem *d0, *d1, *d2;
	int n=0;
	bool f=false;
	FINDWORD("then", d0)
	FINDWORD("else", d1)
	FINDWORD("if", d2)
	int i= (int)pop();
	if(!i){
		while(!f){
			pi++;
			if(pi==MAXPFAS){
				if(p->next==0){
					return;
				}else{
					p=p->next;
					pi=0;
				}
			}
			if((d0->cfa==p->pfa[pi]||d1->cfa==p->pfa[pi])&&n==0){
				f=true;
			}else{
				if(d2->cfa==p->pfa[pi]){
					n++;
				}else{
					if(d0->cfa==p->pfa[pi]){
						n--;
					}
				}
			}
		}
	}
}

void doelse(void){
	ditem *d0;
	bool f=false;
	FINDWORD("then", d0)
	while(!f){
		if(d0->cfa==p->pfa[pi]){
			f=true;
		}else{
			pi++;
			if(pi==MAXPFAS){
				if(p->next==0){
					return;
				}else{
					p=p->next;
					pi=0;
				}
			}
		}
	}
}

void dothen(void){
}

void dobegin(void){
}

void dountil(void){
	ditem *d0;
	bool f=false;
	int t=(int )pop();
	if(!t){
	FINDWORD("begin", d0)
		while(!f){
			if(d0->cfa==p->pfa[pi]){
				f=true;
			}else{
				pi--;
				if(pi<0){
					if(p->prev==0){
						return;
					}else{
						p=p->prev;
						pi=MAXPFAS;
					}
				}
			}
		}
	}
}

void doabort(void){
	printf("\nOK ");
	doexpect();
	DOWORD("interpret")
}

void dobye(void){
	bye=true;
}

void buildword(char *newword, bool primitive){
	int i;
	dnext=malloc(sizeof(ditem));
	dnext->lfa=dlast;
	dnext->primitive=primitive;
	dnext->immediate=false;
	for(i=0;i<MAXWORD;i++){
		dnext->nfa[i]=newword[i];
	}
}

void addword(void){
	dnext->lfa=dlast;
	dlast=dnext;
}

void docompile(void){
	d=(ditem *) pop();
	if(d->immediate){
		push((void *)d);
		doexecute();
	}else{
		if(pnewi==MAXPFAS){
			COMPOSITEMORE
		}
		pnew->pfa[pnewi++]=d;
	}
}

void docolon(void){
	state=true;
	doword();
	COMPOSITE(word);
	dnext->threaded=true;
}

void dosemicolon(void){
	ditem *d0;
	FINDWORD("exit", d0)
	push((void *) d0);
	docompile();
	COMPOSITEEND
	state=false;
}

void dorunlit(void){
	if(pi==MAXPFAS){
		p=p->next;
		pi=0;
	}
	push(p->pfa[pi]);
}

void dolit(void){
	ditem *d0;
	if(state){
		FINDWORD("[lit]",d0)
		push(d0);
		docompile();
		doword();
		if(pnewi==MAXPFAS){
			COMPOSITEMORE
		}
		pnew->pfa[pnewi++]=(void *)atoi(word);
	}else{
		push((void *) atoi(word));
	}
}

void doimmediate(void){
	dlast->immediate=true;
}

void builddictionary(void){
	PRIMITIVE("bye", dobye)
	dlast->lfa=0;
	PRIMITIVE("tip", dotib)
	PRIMITIVE("#tib", donotib)
	PRIMITIVE(">in", doin)

	PRIMITIVE("[lit]", dorunlit)
	PRIMITIVE("compile", docompile)
	PRIMITIVE("immediate", doimmediate)
	PRIMITIVE("state", dostate)
	PRIMITIVE(":", docolon)
	PRIMITIVE(";", dosemicolon)
	dlast->immediate=true;

	PRIMITIVE("not", donot)

	PRIMITIVE("!", dostore)
	PRIMITIVE("@", dofetch)
	PRIMITIVE("drop", dodrop)
	PRIMITIVE("swap", doswap)
	PRIMITIVE(".", dodot)

	PRIMITIVE("expect", doexpect)
	PRIMITIVE("abort", doabort)

	PRIMITIVE("exit", doexit)
	PRIMITIVE("lit", dolit)
	PRIMITIVE("word", doword)
	PRIMITIVE("another", doanother)
	PRIMITIVE("find", dofind)
	PRIMITIVE("execute", doexecute)
	PRIMITIVE("begin", dobegin)
	PRIMITIVE("until", dountil)
	PRIMITIVE("if", doif)
	PRIMITIVE("else", doelse)
	PRIMITIVE("then", dothen)


	COMPOSITE("interpret2")
	COMPOSITEPFA(dobegin,doword,dofind,doif,doexecute)
	COMPOSITEMORE
	COMPOSITEPFA(doelse,dolit,dothen,doanother,donot)
	COMPOSITEMORE
	COMPOSITEPFA(dountil,doexit,doexit,doexit,doexit)
	COMPOSITEEND	

	COMPOSITE("interpretorig")
	COMPOSITEPFA(doword,dofind,doif,doexecute,doelse)
	COMPOSITEMORE
	COMPOSITEPFA(dolit,dothen,doexit,doexit,doexit)
	COMPOSITEEND	

	COMPOSITE("interpret")
	COMPOSITEPFA(dobegin,doword,dofind,doif,dostate)
	COMPOSITEMORE
	COMPOSITEPFA(doif,docompile,doelse,doexecute,dothen)
	COMPOSITEMORE
	COMPOSITEPFA(doelse,dolit,dothen,doanother,donot)
	COMPOSITEMORE
	COMPOSITEPFA(dountil,doexit,doexit,doexit,doexit)
	COMPOSITEEND
}

int main(int argc, char *argv[]){
	state=false;stackpointer=rstackpointer=0;
	resettib();
	builddictionary();
	bye=false;
	printf("\nBFORTH\n\n");
	while(!bye){
		DOWORD("abort")
	}
	printf("\ngoodbye\n\n");
}

