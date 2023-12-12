#include <stdio.h>
#include <stdint.h>

//uint16_t fletcher16(const uint8_t *data, int len);

int main(int argc, char *argv[])
{
	FILE *fp;
	int c;
	uint16_t sum1 = 0U, sum2 = 0U;

	while (--argc)
	{
		++argv;
		if ((fp = fopen(*argv, "r")) == NULL)
		{
			fprintf(stderr, "**Failed to open %s.\n", *argv);
			continue;
		}
		sum1 = sum2 = 0U;
		while ((c = fgetc(fp)) != EOF)
		{
			sum1 = (sum1 + (uint8_t)c) % 0xffU;
			sum2 = (sum2 + sum1) % 0xffU;
		}
		fclose(fp);
		printf("%s: %u;\n", *argv, (sum2 << 8) | sum1);
	}
	return 0;
}

/*
uint16_t fletcher16(const uint8_t *data, int len)
{
	uint16_t sum1 = 0U, sum2 = 0U;
	int i;

	for (i = 0; i < len; ++i)
	{
		sum1 = (sum1 + data[i]) % 0xffU;
		sum2 = (sum2 + sum1) % 0xffU;
	}
	return (sum2 << 8) | sum1;
}
*/