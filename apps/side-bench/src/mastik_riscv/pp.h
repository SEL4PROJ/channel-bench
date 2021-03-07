#ifndef __PP_H__
#define __PP_H__


typedef void *pp_t;

pp_t pp_prepare(vlist_t list, int count, int offset);

void pp_prime(pp_t pp, int reps);
int pp_probe(pp_t pp);

#endif // __PP_H__
