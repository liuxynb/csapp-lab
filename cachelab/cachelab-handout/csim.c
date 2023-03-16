/*
Data structure:
+--------------+
| (cache0)     |        +--------------+
|  s sets      +--------> (set0)       |        +-----------+
+--------------+        |  lines       +--------> (line0)   |
                        +--------------+        |  valid    |
                        | (set1)       |        |  tag      |
                        |  lines       |        |  counter  |
                        +--------------+        +-----------+
                        | (set2)       |        | (line1)   |
                        |  lines       |        |  valid    |
                        +--------------+        |  tag      |
                        | (setX)       |        |  counter  |
                        |  lines       |        +-----------+
                        +--------------+        | (lineX)   |
                                                |  valid    |
                                                |  tag      |
                                                |  counter  |
                                                +-----------+
The map is from https://github.com/moranzcw/CSAPP_Lab
*/

// 2023/03/15
#include "cachelab.h"
#include <unistd.h> //getopt
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#define true 1
#define false 0
typedef uint8_t Bool;

// The data structure below is the Cache data structure
typedef struct
{
    Bool valid;
    uint64_t tag;     // tag bits
    uint64_t counter; // for LRU way
} Line;
typedef struct
{
    Line *lines;
    uint64_t length;
} Set;
typedef struct
{
    Set *sets;
    uint64_t length;
} Cache;

typedef struct
{
    int hit;
    int miss;
    int eviction;
} Result;
typedef struct
{
    uint64_t s;      // the number of sets index's bits;
    uint64_t b;      // the number of each cache blocks index 's bits;
    uint64_t S;      // number of sets;
    uint64_t E;      // number of lines;
    FILE *tracefile; // file pointer;
} Options;

/*get the last visited cache for LRU*/
uint64_t Get_System_Time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000.0 * tv.tv_sec + tv.tv_usec * 1000000;
}
/*function to create virturl cache in heap*/
Cache Create_Cache(Options op)
{
    Cache cache;
    if (NULL == (cache.sets = calloc(op.S, sizeof(Set)))) // calloc will set each bit equals to 0
    {
        perror("Failed to create sets");
        exit(EXIT_FAILURE); // define EXIT_FAILURE 1
    }
    cache.length = op.S;
    for (int i = 0; i < op.S; i++)
    {
        if (NULL == (cache.sets[i].lines = calloc(op.E, sizeof(Line))))
        {
            perror("Failed to create lines in sets");
        }
        cache.sets[i].length = op.E;
    }
    return cache;
}
/*function to release the calloced memory */
void Delete_Cache(Cache cache)
{
    for (int i = 0; i < cache.length; i++)
    {
        free(cache.sets[i].lines);
    }
    free(cache.sets);
}
/*scan the item we needed in the memory. Judge hit, miss, and viction(Almost the core function)*/
Result Updata_Set(Set set, Result result, uint64_t tag)
{
    Bool hit_flag = false;
    for (uint64_t i = 0; i < set.length; i++)
    {
        if (set.lines[i].tag == tag && set.lines[i].valid == 1) // hit! NOTICE！The valid bit 1 is OK
        {
            hit_flag = true;
            result.hit++;
            set.lines[i].counter = Get_System_Time(); // For LRU use
            break;
        }
    }
    if (hit_flag == false) // miss!
    {
        result.miss++;
        Bool empty_flag = false;
        // There are empty cache space to load, so just load it directly.
        for (uint64_t i = 0; i < set.length; i++)
        {
            if (!set.lines[i].valid) // empty line
            {
                empty_flag = true;
                set.lines[i].counter = Get_System_Time();
                set.lines[i].tag = tag; // load directly
                set.lines[i].valid = true;
                break;
            }
        }
        if (!empty_flag) // ops! eviction(驱逐)
        {
            result.eviction++;
            uint64_t oldestTime = UINT64_MAX;
            uint64_t oldestLine = 0;
            for (uint64_t i = 0; i < set.length; i++)
            {
                if (set.lines[i].counter < oldestTime)
                {
                    oldestTime = set.lines[i].counter;
                    oldestLine = i;
                }
            }
            // overwrite the memory by LRU rule
            set.lines[oldestLine].counter = Get_System_Time();
            set.lines[oldestLine].tag = tag;
        }
    }
    return result;
}
/*process the command*/
Result Run_cache(Cache cache, Options op)
{

    Result result = {0, 0, 0};
    char instruction;
    uint64_t address;
    uint64_t set_idx_mask = (1 << op.s) - 1;
    /*tag of lines------sets index-------block offset*/

    while ((fscanf(op.tracefile, " %c %lx%*[^\n]", &instruction, &address)) == 2) // read instruction and address from file, such as "M 0421c7f0,4"
    {
        if (instruction != 'I')
        {
            uint64_t set_index = (address >> op.b) & set_idx_mask;
            uint64_t tag = (address >> op.b) >> op.s;
            Set set = cache.sets[set_index];
            if (instruction == 'L' || instruction == 'S') // L is load, S is store
            {
                result = Updata_Set(set, result, tag);
            }
            if (instruction == 'M') // M is modify
            {
                result = Updata_Set(set, result, tag);
                result = Updata_Set(set, result, tag);
            }
        }
    }
    return result;
}

/*get options*/
Options Get_Options(int argc, char *const argv[])
{
    const char *help_message = "Usage: \"Your complied program\" [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
                               "<s> <E> <b> should all above zero and below 64.\n"
                               "Complied with std=c99\n";
    const char *command_options = "hvs:E:b:t:";
    Options opt;
    char ch;
    while ((ch = getopt(argc, argv, command_options)) != -1)
    {
        switch (ch)
        {
        case 'h':
        {
            printf("%s", help_message);
            exit(EXIT_SUCCESS);
        }

        case 's':
        {

            if (atol(optarg) <= 0) // at least two sets
            {
                printf("%s", help_message);
                exit(EXIT_FAILURE);
            }
            opt.s = atol(optarg);
            opt.S = 1 << opt.s;
            break;
        }

        case 'E':
        {
            if (atol(optarg) <= 0)
            {
                printf("%s", help_message);
                exit(EXIT_FAILURE);
            }
            opt.E = atol(optarg);
            break;
        }

        case 'b':
        {
            if (atol(optarg) <= 0) // at least two sets
            {
                printf("%s", help_message);
                exit(EXIT_FAILURE);
            }
            opt.b = atol(optarg);
            break;
        }

        case 't':
        {
            if ((opt.tracefile = fopen(optarg, "r")) == NULL)
            {
                perror("Failed to open tracefile");
                exit(EXIT_FAILURE);
            }
            break;
        }

        default:
        {
            printf("%s", help_message);
            exit(EXIT_FAILURE);
        }
        }
    }

    if (opt.s == 0 || opt.b == 0 || opt.E == 0 || opt.tracefile == NULL)
    {
        printf("%s", help_message);
        exit(EXIT_FAILURE);
    }

    return opt;
}
int main(int argc, char *const argv[])
{
    Options op = Get_Options(argc, argv);
    Cache cache = Create_Cache(op);
    Result result = Run_cache(cache, op);
    Delete_Cache(cache);
    printSummary(result.hit, result.miss, result.eviction);
    return 0;
}
