
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main (){
printf("existe1");

FILE *fp;
int check;
char* dirname = "GROUP/user00009";
char buff[15];

printf("existe2");

check = mkdir(dirname,0777);

printf("existe3");
fp=fopen("GROUP/user00009/user_ID.txt", "rw");

/*if( access( "USERS/user00005/user_ID.txt", F_OK ) == 0 ) {
    printf("existe");
} else {
    printf("nao existe");
}*/
fprintf(fp, "839");

fgets(buff, 15, fp);


printf("%s\n",buff);

//fclose(fp);

return 0;
}
