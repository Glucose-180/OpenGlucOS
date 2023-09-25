#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "imagedef.h"

static task_info_t taskinfo[TASK_MAXNUM];

/* structure to store command line options */
static struct {
	int vm;
	int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr);
//static uint64_t get_entrypoint(Elf64_Ehdr ehdr);
static uint32_t get_filesz(Elf64_Phdr phdr);
//static uint32_t get_memsz(Elf64_Phdr phdr);
static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr);
static void write_padding(FILE *img, int *phyaddr, int new_phyaddr);
static void write_img_info(uint32_t kernel_size,
	uint32_t taskinfo_offset, uint32_t tasknum, task_info_t *taskinfo, FILE *img);

int main(int argc, char **argv)
{
	char *progname = argv[0];

	/* process command line options */
	options.vm = 0;
	options.extended = 0;
	while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
		char *option = &argv[1][2];

		if (strcmp(option, "vm") == 0) {
			options.vm = 1;
		} else if (strcmp(option, "extended") == 0) {
			options.extended = 1;
		} else {
			error("%s: invalid option\nusage: %s %s\n", progname,
				  progname, ARGS);
		}
		argc--;
		argv++;
	}
	if (options.vm == 1) {
		error("%s: option --vm not implemented\n", progname);
	}
	if (argc < 3) {
		/* at least 3 args (createimage bootblock main) */
		error("usage: %s %s\n", progname, ARGS);
	}
	create_image(argc - 1, argv + 1);
	return 0;
}

/* TODO: [p1-task4] assign your task_info_t somewhere in 'create_image' */
static void create_image(int nfiles, char *files[])
{
	/*
	 * nfiles is number of files used to make image,
	 * including bootblock and kernel (main).
	 */

	int tasknum = nfiles - 2;

	/*
	 * This three info will be written into image,
	 * so their sizes are important!
	 */
	int32_t nbytes_kernel = 0,
		 phyaddr = 0;	/* Address in image file */

	FILE *fp = NULL, *img = NULL;
	Elf64_Ehdr ehdr;
	Elf64_Phdr phdr;

	/* open the image file */
	img = fopen(IMAGE_FILE, "w");
	assert(img != NULL);

	/* for each input file */
	for (int fidx = 0; fidx < nfiles; ++fidx)
	{

		//int taskidx = fidx - 2;

		/* open input file */
		fp = fopen(*files, "r");
		assert(fp != NULL);

		/* read ELF header */
		read_ehdr(&ehdr, fp);
		printf("off 0x%04x: %s (e_entry: 0x%04lx)\n", phyaddr, *files, ehdr.e_entry);
		if (fidx >= 2)
		{	/* Ensure that it is an app rather than bootblock or kernel */
			taskinfo[fidx - 2].offs = phyaddr;
			strncpy(taskinfo[fidx - 2].name, *files, TASK_NAMELEN);
			taskinfo[fidx - 2].name[TASK_NAMELEN] = '\0';
		}
		/* for each program header */
		for (int ph = 0; ph < ehdr.e_phnum; ph++) {

			/* read program header */
			read_phdr(&phdr, fp, ph, ehdr);

			if (phdr.p_type != PT_LOAD) continue;

			/* write segment to the image */
			write_segment(phdr, fp, img, &phyaddr);

			/* update nbytes_kernel */
			if (strcmp(*files, "main") == 0) {
				nbytes_kernel += get_filesz(phdr);
			}
		}

		/* write padding bytes */
		/**
		 * TODO:
		 * 1. [p1-task3] do padding so that the kernel and every app program
		 *  occupies the same number of sectors
		 * 2. [p1-task4] only padding bootblock is allowed!
		 */

		assert((fidx == 0) == (strcmp(*files, "bootblock") == 0));
		if (fidx == 0) {
			write_padding(img, &phyaddr, SECTOR_SIZE);
		}
		else if (fidx >= 2)
		{
			taskinfo[fidx - 2].entr = ehdr.e_entry;
			taskinfo[fidx - 2].size = phyaddr - taskinfo[fidx - 2].offs;
		}

		fclose(fp);
		files++;
	}
	write_img_info(nbytes_kernel, phyaddr, tasknum, taskinfo, img);

	fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
	int ret;

	ret = fread(ehdr, sizeof(*ehdr), 1, fp);
	assert(ret == 1);
	assert(ehdr->e_ident[EI_MAG1] == 'E');
	assert(ehdr->e_ident[EI_MAG2] == 'L');
	assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
					  Elf64_Ehdr ehdr)
{
	int ret;

	fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
	ret = fread(phdr, sizeof(*phdr), 1, fp);
	assert(ret == 1);
	if (options.extended == 1) {
		printf("\tsegment %d\n", ph);
		printf("\t\toffset 0x%04lx", phdr->p_offset);
		printf("\t\tvaddr 0x%04lx\n", phdr->p_vaddr);
		printf("\t\tfilesz 0x%04lx", phdr->p_filesz);
		printf("\t\tmemsz 0x%04lx\n", phdr->p_memsz);
	}
}

/*static uint64_t get_entrypoint(Elf64_Ehdr ehdr)
{
	return ehdr.e_entry;
}*/

static uint32_t get_filesz(Elf64_Phdr phdr)
{
	return phdr.p_filesz;
}

/*static uint32_t get_memsz(Elf64_Phdr phdr)
{
	return phdr.p_memsz;
}*/

static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr)
{
	if (phdr.p_memsz != 0 && phdr.p_type == PT_LOAD) {
		/* write the segment itself */
		/* NOTE: expansion of .bss should be done by kernel or runtime env! */
		if (options.extended == 1) {
			printf("\t\twriting 0x%04lx bytes\n", phdr.p_filesz);
		}
		fseek(fp, phdr.p_offset, SEEK_SET);
		while (phdr.p_filesz-- > 0) {
			fputc(fgetc(fp), img);
			(*phyaddr)++;
		}
	}
}

/*
 * Write padding of 0 from *phyaddr to new_phyaddr
 * of file img
 */
static void write_padding(FILE *img, int *phyaddr, int new_phyaddr)
{
	if (options.extended == 1 && *phyaddr < new_phyaddr) {
		printf("\twrite 0x%04x bytes for padding\n", new_phyaddr - *phyaddr);
	}

	while (*phyaddr < new_phyaddr) {
		fputc(0, img);
		(*phyaddr)++;
	}
}

static void write_img_info(uint32_t kernel_size, uint32_t taskinfo_offset,
	uint32_t tasknum, task_info_t *taskinfo, FILE *img)
{
	// TODO: [p1-task3] & [p1-task4] write image info to some certain places
	// NOTE: os size, infomation about app-info sector(s) ...
	uint32_t image_size = taskinfo_offset;

	/* Write info of tasks (apps) */
	assert(fwrite(taskinfo, sizeof(task_info_t), tasknum, img) == tasknum);
	image_size += tasknum * sizeof(task_info_t);

	if (options.extended != 0)
		printf("image file size: %x bytes\n", image_size);

	/* Number of sectors occupied by kernel */
	kernel_size = NBYTES2SEC(kernel_size);

	/* Number of sectors occupied by the whole image */
	image_size = NBYTES2SEC(image_size);

	/*
	 * Write image_size, kernel_size (unit: sector),
	 * taskinfo_offset (unit: byte), and tasknum.
	 */
	fseek(img, Image_size_offset, SEEK_SET);
	fwrite(&image_size, sizeof(uint32_t), 1U, img);
	fwrite(&kernel_size, sizeof(uint32_t), 1U, img);
	fwrite(&taskinfo_offset, sizeof(uint32_t), 1U, img);
	fwrite(&tasknum, sizeof(uint32_t), 1U, img);

	/*
	 * Write BOOT_LOADER_SIG_1 and .._SIG_2.
	 */
	fseek(img, Bootloader_sig_offset, SEEK_SET);
	fputc(BOOT_LOADER_SIG_1, img);
	fputc(BOOT_LOADER_SIG_2, img);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (errno != 0) {
		perror(NULL);
	}
	exit(EXIT_FAILURE);
}
