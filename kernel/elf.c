/*
Copyright (C) 2018 The University of Notre Dame
This software is distributed under the GNU General Public License.
See the file LICENSE for details.
*/

#include "console.h"
#include "elf.h"
#include "fs.h"
#include "kmalloc.h"
#include "main.h"
#include "memorylayout.h"
#include "process.h"

#define ELF_MAGIC_NUMBER               0x464c457f	/*ELF Magic number, in little endian order */
#define ELF_PROG_TYPE_LOAD             0x00000001	/*LOAD segment type for the ELF program header */

#define ELF_HEADER_ENTRY_OFFSET        0x00000018	/*Location of the process entry point in the ELF header */
#define ELF_HEADER_PROGHEAD_OFFSET     0x0000001c	/*Location of the program header location in the ELF header */
#define ELF_HEADER_PROGHEADSIZE_OFFSET 0x0000002a	/*Location of the program header size in the ELF header */
#define ELF_HEADER_PROGHEADNUM_OFFSET  0x0000002c	/*Location of the number of program header segments in the ELF header */

#define ELF_PROGHEADER_FOFF_OFFSET     0x00000004	/*Location of the file offset of a program segment in the program header */
#define ELF_PROGHEADER_VADR_OFFSET     0x00000008	/*Location of the virtual address of a program segment in the program header */
#define ELF_PROGHEADER_FSIZE_OFFSET    0x00000010	/*Location of the file size of the program segment in the program header */
#define ELF_PROGHEADER_MSIZE_OFFSET    0x00000014	/*Location of the in memory size of the program segment in the program header */

static char *elf_load_image(const char *path)
{
	/* Open and find the named path, if it exists. */

	if(!root_directory)
		return 0;

	struct fs_dirent *d = fs_dirent_namei(root_directory, path);
	if(!d) {
		return 0;
	}

	int length = d->size;

	struct fs_file *f = fs_file_open(d, FS_FILE_READ);
	char *image = kmalloc(length);
	fs_file_read(f, image, length,0);
	fs_dirent_close(d);
	fs_file_close(f);

	int ei_mag = *(int *) (image);
	if(ei_mag != ELF_MAGIC_NUMBER) {
		printf("Error loading program %s, unexpected magic number 0x%x - expected 0x%x\n", path, ei_mag, ELF_MAGIC_NUMBER);
		kfree(image);
		return 0;
	}

	return image;
}

static uint32_t elf_get_brk(const char *image, const char *path)
{
	int e_phoff = *(int *) (image + ELF_HEADER_PROGHEAD_OFFSET);
	int e_phentsize = *(short *) (image + ELF_HEADER_PROGHEADSIZE_OFFSET);
	int e_phnum = *(short *) (image + ELF_HEADER_PROGHEADNUM_OFFSET);

	/* Calculate the process brk point */
	uint32_t max_mem = 0;
	int i;
	for(i = 0; i < e_phnum; ++i) {
		const char *index = image + e_phoff + e_phentsize * i;
		uint32_t vadr = *(int *) (index + ELF_PROGHEADER_VADR_OFFSET);
		uint32_t size = *(int *) (index + ELF_PROGHEADER_MSIZE_OFFSET);
		if(max_mem < vadr + size) {
			max_mem = vadr + size;
		}
		uint32_t type = *(int *) (index);
		if(type != ELF_PROG_TYPE_LOAD) {
			printf("Error loading program %s, unexpected program header type 0x%x - expected 0x%x\n", path, type, ELF_PROG_TYPE_LOAD);
			return 0;
		}
		if(vadr < PROCESS_ENTRY_POINT) {
			printf("Error loading program %s, it requested to be loaded at 0x%x, which is not allowed\n", path, vadr);
			return 0;
		}
	}
	return max_mem;
}

int elf_load_process( struct process *p, const char *image, const char *path )
{

	uint32_t e_entry = *(uint32_t *) (image + ELF_HEADER_ENTRY_OFFSET);
	uint32_t e_phoff = *(uint32_t *) (image + ELF_HEADER_PROGHEAD_OFFSET);
	uint16_t e_phentsize = *(uint16_t *) (image + ELF_HEADER_PROGHEADSIZE_OFFSET);
	uint16_t e_phnum = *(uint16_t *) (image + ELF_HEADER_PROGHEADNUM_OFFSET);

	uint32_t max_mem = elf_get_brk(image, path);
	if(!max_mem) {
		return 0;
	}

	/* Reset the process VM space to match the desired program */

	process_data_size_set(p,max_mem - PROCESS_ENTRY_POINT);
	process_stack_size_set(p,PAGE_SIZE);
	process_kstack_reset(p,e_entry);

	int i;
	for(i = 0; i < e_phnum; ++i) {
		const char *index = image + e_phoff + e_phentsize * i;
		uint32_t offs = *(int *) (index + ELF_PROGHEADER_FOFF_OFFSET);
		uint32_t vadr = *(int *) (index + ELF_PROGHEADER_VADR_OFFSET);
		uint32_t size = *(int *) (index + ELF_PROGHEADER_FSIZE_OFFSET);
		int copied = 0;
		while(copied < size) {
			unsigned paddr;
			unsigned vaddr = vadr + copied;
			pagetable_getmap(p->pagetable, vaddr, &paddr, 0);
			paddr += (vaddr % PAGE_SIZE);
			int amount = PAGE_SIZE;
			if(copied + amount > size) {
				amount = size - copied;
			}
			if(paddr / PAGE_SIZE < (paddr + amount) / PAGE_SIZE) {
				amount = ((paddr + amount) / PAGE_SIZE) * PAGE_SIZE - paddr;
			}
			memcpy((void *) paddr, image + offs + copied, amount);
			copied += amount;
		}
	}
	return 1;
}

int elf_load( struct process *p, const char *path )
{
	char *image = elf_load_image(path);
	if(!image) {
		return 0;
	}

	elf_load_process(p,image, path );
	// XXX check return value

	kfree(image);
	return 1;
}
