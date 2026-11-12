#include <stdio.h>
#include <stdint.h>
int main(int argc,char**argv){ for(int i=1;i<argc;i++){ FILE*f=fopen(argv[i],"rb"); uint64_t h=0xcbf29ce484222325ULL; int c; long n=0; while((c=fgetc(f))!=EOF){ h^=(uint64_t)(unsigned char)c; h*=1099511628211ULL; n++; } printf("%s bytes %ld fnv1a64 %016llx\n",argv[i],n,(unsigned long long)h); fclose(f);} }
