
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main (){

FILE *fp;
int check;
char* dirname = "GROUP/user00005";
char buff[15];



check = mkdir(dirname,0777);


fp=fopen("GROUP/user00005/user_ID.txt", "w+");

if( access( "GROUP/user00005/user_ID.txt", F_OK ) == 0 ) {
    printf("existe");
} else {
    printf("nao existe");
}
fprintf(fp, "839");

fgets(buff, 15, fp);


printf("%s\n",buff);

//fclose(fp);
return 0;
}
