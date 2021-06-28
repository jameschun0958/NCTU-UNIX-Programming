#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <map>
#include<elf.h>

#include "ptools.h"

using namespace std;

bool operator<(range_t r1, range_t r2) {
	if(r1.begin < r2.begin && r1.end < r2.end) return true;
	return false;
}

int load_maps(pid_t pid, map<range_t, map_entry_t>& loaded) {
	char fn[128];
	char buf[256];
	FILE *fp;
	snprintf(fn, sizeof(fn), "/proc/%u/maps", pid);
	if((fp = fopen(fn, "rt")) == NULL) return -1;
	while(fgets(buf, sizeof(buf), fp) != NULL) {
		int nargs = 0;
		char *token, *saveptr, *args[8], *ptr = buf;
		map_entry_t m;
		while(nargs < 8 && (token = strtok_r(ptr, " \t\n\r", &saveptr)) != NULL) {
			args[nargs++] = token;
			ptr = NULL;
		}
		// if(nargs < 6) continue;
		if((ptr = strchr(args[0], '-')) != NULL) {
			*ptr = '\0';
			m.range.begin = strtoll(args[0], NULL, 16);
			m.range.end = strtoll(ptr+1, NULL, 16);
		}
		// m.name = basename(args[5]);
		m.name = args[5];
		m.perm = 0;
		if(args[1][0] == 'r') m.perm |= 0x04;
		if(args[1][1] == 'w') m.perm |= 0x02;
		if(args[1][2] == 'x') m.perm |= 0x01;
		m.offset = strtol(args[2], NULL, 16);
		//printf("XXX: %lx-%lx %04o %s\n", m.range.begin, m.range.end, m.perm, m.name.c_str());
		loaded[m.range] = m;
	}
	return (int) loaded.size();
}

prog_info elf_read(string path)
{
	FILE *fp;
	Elf64_Ehdr elf_header;
	prog_info info;

	fp = fopen(path.c_str(),"r");
	if(fp == NULL)
	{ 
		fprintf(stderr, "** Unable to open '%s': %s\n", path.c_str(), strerror(errno));
		exit(-1);
	}

	if(fread(&elf_header, sizeof(Elf64_Ehdr), 1, fp) != 1)
	{
		fprintf(stderr, "** fread: %s\n", strerror(errno));
		fclose(fp);
		exit(-1);
	}

	if(elf_header.e_ident[0] == 0x7F || elf_header.e_ident[1] == 'E')
	{
		// printf("header file: ");
		// for(int x =0;x<16;x++)
		// {
		// 	printf("%x ",elf_header.e_ident[x]);
		// }
		// printf("\ne_type: %hx\n",elf_header.e_type);
		// printf("e_machine: %hx\n",elf_header.e_machine);
		// printf("e_entry: 0x%x\n",elf_header.e_entry);
		// printf("e_phoff: %d(bytes)\n",elf_header.e_phoff);
		// printf("e_shoff: %d(bytes)\n",elf_header.e_shoff);
		// printf("e_flags: %d(bytes)\n",elf_header.e_flags);
		// printf("e_ehsize: %d\n",elf_header.e_ehsize);
		// printf("e_phentsize: %d\n",elf_header.e_phentsize);
		// printf("e_phnum: %d\n",elf_header.e_phnum);
		// printf("e_shentsize: %d\n",elf_header.e_shentsize);
		// printf("e_shnum: %d\n",elf_header.e_shnum);
		// printf("e_shstrndx: %d\n",elf_header.e_shstrndx);
		// printf("\n");

		int temp, shnum, x;
		Elf64_Shdr *shdr = (Elf64_Shdr*)malloc(sizeof(Elf64_Shdr) * elf_header.e_shnum);
		temp = fseek(fp, elf_header.e_shoff, SEEK_SET);
		temp = fread(shdr, sizeof(Elf64_Shdr) * elf_header.e_shnum, 1, fp);
		rewind(fp);
		fseek(fp, shdr[elf_header.e_shstrndx].sh_offset, SEEK_SET);
		char shstrtab[shdr[elf_header.e_shstrndx].sh_size];
		char *names = shstrtab;
		temp = fread(shstrtab, shdr[elf_header.e_shstrndx].sh_size, 1, fp);
		// printf("Type\tAddr\tOffsset\tSize\tName\n");
		for(shnum = 0; shnum < elf_header.e_shnum; shnum++)
		{
			names = shstrtab;
			names=names+shdr[shnum].sh_name;

			if(strcmp(names,".text")==0)
			{
				info.entry = elf_header.e_entry;
				info.size = shdr[shnum].sh_size;
				// printf("%x\t%x\t%x\t%x\t%s \n",shdr[shnum].sh_type,shdr[shnum].sh_addr,shdr[shnum].sh_offset,shdr[shnum].sh_size,names);
			}
				
		}
	}

	return info;
}

