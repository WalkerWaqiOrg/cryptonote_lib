#include "hash-ops.h"
#include "string.h"
 //void cn_slow_hash(const void *data, size_t length, char *hash);
int main(int argc, char const *argv[])
{
    /* code */
    char testStr[300];
    char data[30] = "hello,world!";
    cn_slow_hash(data,strlen(data),testStr);
    return 0;
}
