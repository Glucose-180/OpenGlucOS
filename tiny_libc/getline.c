#include <stdio.h>

/*
 * Read a line and return its length.
 * Please set Len as the size of str[].
 */
int getline(char* str, const int Len)
{
	int i, c;
	
	for (i = 0; i < Len - 1 && (c = getchar()) != NOI && c != '\n'; ++i)
		str[i] = c;
	if (c == '\n')
		str[i++] = c;
	str[i] = '\0';
	return i;
}
