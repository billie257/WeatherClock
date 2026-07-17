#include <stdio.h>

int main()
{
	char str[21], str1[21];
	snprintf(str, sizeof(str), "%20s", "11111222223333344444");
	snprintf(str1, sizeof(str1), "%20s", "vivo S30 Pro mini");
	puts(str);
	puts(str1);
	return 0;
}
