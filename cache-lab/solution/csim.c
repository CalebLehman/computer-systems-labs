#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

const char usage[] = "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
                     "Options:\n"
                     "  -h         Print this help message.\n"
                     "  -v         Optional verbose flag.\n"
                     "  -s <num>   Number of set index bits.\n"
                     "  -E <num>   Number of lines per set.\n"
                     "  -b <num>   Number of block offset bits.\n"
                     "  -t <file>  Trace file.\n";
//typedef struct options options;
typedef struct options {
    bool v_flag, s_flag, E_flag, b_flag, t_flag;
    unsigned int s, E, b;
    const char* t;
} options;
options parse_args(int argc, char** argv);

typedef unsigned long address;
typedef struct set_assoc_cache {
    unsigned int s, E, b;
    bool** vs;
    address** tags;
    unsigned int hits, misses, evictions;
    bool verbose;
} set_assoc_cache;
set_assoc_cache* create_cache(unsigned int s, unsigned int E, unsigned int b, bool verbose);
void hit_cache(set_assoc_cache* cache);
void miss_cache(set_assoc_cache* cache);
void evict_cache(set_assoc_cache* cache);
void destroy_cache(set_assoc_cache* cache);
void set_mru(set_assoc_cache* cache, unsigned int set, unsigned int e);
void access(set_assoc_cache* cache, address addr);
void process_instruction(set_assoc_cache* cache, char op, address addr, int rep);

int main(int argc, char** argv) {
    options opts = parse_args(argc, argv);
    set_assoc_cache* cache = create_cache(opts.s, opts.E, opts.b, opts.v_flag);

    FILE* trace = fopen(opts.t, "r");
    if (trace == NULL) {
        fprintf(stderr, "%s: No such file or directory\n", opts.t);
        exit(1);
    }

    char op;
    address addr;
    int rep;
    while (fscanf(trace, " %c %lx,%d", &op, &addr, &rep) == 3) {
        process_instruction(cache, op, addr, rep);
    }
    printSummary(cache->hits, cache->misses, cache->evictions);

    fclose(trace);
    destroy_cache(cache);
    return 0;
}

/* ************************* */
/* ARGUMENT PARSING ROUTINES */
/* ************************* */

options parse_args(int argc, char** argv) {
    options opts;
    int c;
    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (c) {
            case 'v': opts.v_flag = true; break;
            case 's': opts.s_flag = true; opts.s = strtol(optarg, NULL, 10); break;
            case 'E': opts.E_flag = true; opts.E = strtol(optarg, NULL, 10); break;
            case 'b': opts.b_flag = true; opts.b = strtol(optarg, NULL, 10); break;
            case 't': opts.t_flag = true; opts.t = optarg; break;
            case 'h': printf(usage, argv[0]); exit(0);
            default:
                fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], c);
                printf(usage, argv[0]);
                exit(0);
        }
    }
    if (!(opts.s_flag && opts.E_flag && opts.b_flag && opts.t_flag)) {
        fprintf(stderr, "%s: Missing required command line argument\n", argv[0]);
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }
    return opts;
}

/* ******************************** */
/* (SET ASSOCIATIVE) CACHE ROUTINES */
/* ******************************** */

set_assoc_cache* create_cache(unsigned int s, unsigned int E, unsigned int b, bool verbose) {
    set_assoc_cache* cache = malloc(sizeof(*cache));
    cache->vs   = malloc((1<<s) * sizeof(*cache->vs));
    cache->tags = malloc((1<<s) * sizeof(*cache->tags));
    for (unsigned int i = 0; i < (1<<s); ++i) {
        cache->vs[i]   = calloc(E, sizeof(**cache->vs));
        cache->tags[i] = malloc(E * sizeof(**cache->tags));
    }
    cache->hits = cache->misses = cache->evictions = 0;
    cache->s = s;
    cache->E = E;
    cache->b = b;
    cache->verbose = verbose;
    return cache;
}
void hit_cache(set_assoc_cache* cache) { ++cache->hits; if (cache->verbose) printf("hit "); }
void miss_cache(set_assoc_cache* cache) { ++cache->misses; if (cache->verbose) printf("miss "); }
void evict_cache(set_assoc_cache* cache) { ++cache->evictions; if (cache->verbose) printf("eviction "); }
void destroy_cache(set_assoc_cache* cache) {
    for (unsigned int i = 0; i < (1<<cache->s); ++i) {
        free(cache->vs[i]);
        free(cache->tags[i]);
    }
    free(cache->vs);
    free(cache->tags);
    free(cache);
}
void set_as_mru(set_assoc_cache* cache, unsigned int set, unsigned int e) {
    address tag = cache->tags[set][e];
    bool v      = cache->vs[set][e];
    for (; e > 0; --e) {
        cache->tags[set][e] = cache->tags[set][e-1];
        cache->vs[set][e]   = cache->vs[set][e-1];
    }
    cache->tags[set][0] = tag;
    cache->vs[set][0]   = v;
}
void access(set_assoc_cache* cache, address addr) {
    address set = (addr >> cache->b) & ((1<<cache->s) - 1);
    address tag = addr >> (cache->b + cache->s);
    for (unsigned int e = 0; e < cache->E; ++e) {
        if ((cache->tags[set][e] == tag) && (cache->vs[set][e])) {
            hit_cache(cache);
            set_as_mru(cache, set, e);
            return;
        }
    }
    for (unsigned int e = 0; e < cache->E; ++e) {
        if (!cache->vs[set][e]) {
            cache->tags[set][e] = tag;
            cache->vs[set][e]   = true;
            miss_cache(cache);
            set_as_mru(cache, set, e);
            return;
        }
    }
    cache->tags[set][cache->E-1] = tag;
    cache->vs[set][cache->E-1]   = true;
    miss_cache(cache);
    evict_cache(cache);
    set_as_mru(cache, set, cache->E-1);
}
void process_instruction(set_assoc_cache* cache, char op, address addr, int rep) {
        if (op == 'I') return;
        if (cache->verbose) printf("%c %lx,%d ", op, addr, rep);
        if ((op == 'L') || (op == 'M')) access(cache, addr);
        if ((op == 'S') || (op == 'M')) access(cache, addr);
        if (cache->verbose) printf("\n");
}
