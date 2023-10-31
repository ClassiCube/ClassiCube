#include <dirent.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Inspired by the public domain bin2c https://github.com/gwilymk/bin2c
// TODO: log errors
void convert_gxp(const char* src) {
	char* copy = strdup(src);
	char* name = strtok(copy, ".");
	char dst[256];
	sprintf(dst, "%s.h", name);	
	printf("  %s --> %s\n", src, dst);

	FILE* file_in  = fopen(src, "r");
	if (!file_in) return;

	FILE* file_out = fopen(dst, "w");
	if (!file_out) return;
	
	fseek(file_in, 0, SEEK_END);
	int file_size = ftell(file_in);
	fseek(file_in, 0, SEEK_SET);

	char* data = malloc(file_size);
	fread(data, file_size, 1, file_in);
	fclose(file_in);

	int comma = 0;
	fprintf(file_out, "const char %s[%i] = {", name, file_size);
	
	for (int i = 0; i < file_size; i++)
	{
		if (comma) fprintf(file_out, ", ");
		if ((i % 16) == 0) fprintf(file_out, "\n\t");
			
		fprintf(file_out, "0x%.2x", data[i] & 0xFF);
		comma = 1;
	}
	
	fprintf(file_out, "\n};");
	fclose(file_out);
}

int main(void) {
	struct dirent* e;
	DIR* d = opendir(".");
	if (!d) return 0;

	while ((e = readdir(d))) {
		printf("checking %s\n", e->d_name);
		
		if (strstr(e->d_name, ".gxp")) {
			convert_gxp(e->d_name);
		}
	}
	
	closedir(d);
	return 0;
}
