#include <stdio.h>
#include <string.h>

unsigned int split(char *src, const char Sep, char *parr[], unsigned int pmax)
{
	unsigned int i;
	char *p;
	char flag_new = 1;

	for (i = 0U, p = src; *p != '\0'; ++p)
	{
		if (flag_new != 0)
			parr[i++] = p, flag_new = 0;
		if (*p == Sep)
		{
			*p = '\0', flag_new = 1;
			if (i >= pmax)
				break;
		}
	}
	return i;
}

unsigned int path_squeeze(char *path)
{
	unsigned int n;
	char *fnames[20U];
	unsigned int i, l;
	int j;

	if (path[0] != '/')
		return 0U;
	++path;	/* Skip '/' */
	n = split(path, '/', fnames, 20U);
	/*
	 * Ignore `.`, `..` and the name before it by setting the
	 * pointers in `fnames[]` to them to `NULL`.
	 */
	for (j = -1, i = 0U; i < n; ++i)
	{	/* Scan every file name */
	/* `j` points to the last valid name before `i` */
		if (strcmp(fnames[i], "..") == 0)
		{
			fnames[i] = NULL;
			if (j >= 0)
				fnames[j--] = NULL;
		}
		else if (strcmp(fnames[i], ".") == 0)
			fnames[i] = NULL;
		else
			j = (int)i;
	}
	/*
	 * Connect the remaining names and calculate the
	 * total length.
	 */
	for (l = 0U, i = 0U; i < n; ++i)
	{
		if (fnames[i] == NULL)
			continue;
		while (*(fnames[i]) != '\0')
			path[l++] = *(fnames[i]++);
		path[l++] = '/';
	}
	if (l > 0U)
		--l;
	path[l] = '\0';
	return l + 1U;
}

int main()
{
	char path[144];
	unsigned l;

	while (scanf("%s", path) == 1)
	{
		if ((l = path_squeeze(path)) > 0U)
			printf("%s, %u\n", path, l);
	}
	return 0;
}