#ifndef __TROJAN_H__
#define __TROJAN_H__

/*entry function for single core trojan/spy attack*/
int trojan_single(char *t_buf, int line, seL4_CPtr syn_ep);
 
void tr_init(uint64_t size);

int tr_start(int pagecount, int line, int core) ;

void tr_stop() ;

int tr_startslave(int line, int core);
void tr_callslave(int size);

#endif // __TROJAN_H__

    

