#include "hash.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    /* code */
    char testStr[300]={0};
    char data[30] = "hello,world!";
    crypto::cn_slow_hash(data,strlen(data),testStr);
    for(int i = 0 ;i < 32; ++i)
    	printf("%x",testStr[i]);
    printf("\n");
    return 0;
}
