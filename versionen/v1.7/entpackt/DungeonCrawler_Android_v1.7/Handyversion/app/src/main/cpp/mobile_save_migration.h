#ifndef MOBILE_SAVE_MIGRATION_H
#define MOBILE_SAVE_MIGRATION_H
#include <stdio.h>
#include <stdlib.h>

/* v1.5 has no Gold Touch level. Its 36-skin header needs a zero inserted
   before perks, otherwise v1.7 would shift every following field. */
static inline void MobileCopyLegacyShop(FILE *src, FILE *out) {
    char line[8192];
    if (!fgets(line,sizeof(line),src)) return;
    long fields[128]; int n=0; char *cursor=line,*end;
    while(n<128){long value=strtol(cursor,&end,10);if(end==cursor)break;fields[n++]=value;cursor=end;}
    if(n==46 && fields[9]==36){
        for(int i=0;i<n;i++){if(i==6)fputs("0 ",out);fprintf(out,"%ld ",fields[i]);}
        fputc('\n',out);
    }else fputs(line,out);
    size_t bytes;while((bytes=fread(line,1,sizeof(line),src))>0)fwrite(line,1,bytes,out);
}
#endif
