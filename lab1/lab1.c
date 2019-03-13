/*	Simon Liu Coen 146 Lab 1
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	char buffer[10];
	
	if(argc != 3){
		printf("Wrong amount of arguments");
		return 0;
	}
	
	FILE *fr = fopen(argv[1], "rb");
	FILE *fw = fopen(argv[2], "wb");
	
	if(fr == NULL || fw == NULL){
		printf("ERROR");
		return 0;
	}
	
	int readchar = 0;
	while(!feof(fr)){
		readchar = fread(buffer, 1, 10, fr);
		fwrite(buffer, 1, readchar, fw);
	}
	fclose(fw);
	fclose(fr);
	return 0;
}
