#include <stdio.h>
#include <stdint.h>

//uint16_t fletcher16(const uint8_t *data, int len);

int main(int argc, char *argv[])
{
	FILE *fp;
	uint8_t c;
	uint16_t sum1 = 0U, sum2 = 0U;
	unsigned int fsize = 0U;

	while (--argc)
	{
		++argv;
		if ((fp = fopen(*argv, "rb")) == NULL)
		{
			fprintf(stderr, "**Failed to open %s.\n", *argv);
			continue;
		}
		sum1 = sum2 = 0U;
		while (fread(&c, sizeof(uint8_t), 1UL, fp) == 1UL)
		{
			sum1 = (sum1 + (uint8_t)c) % 0xffU;
			sum2 = (sum2 + sum1) % 0xffU;
			++fsize;
		}
		fclose(fp);
		printf("%s(%u B): %u;\n", *argv, fsize, (sum2 << 8) | sum1);
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