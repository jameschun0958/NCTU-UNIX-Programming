#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <string.h>
#include <vector>
#include <sys/stat.h>
#include <capstone/capstone.h>
#include <algorithm>

#include "ptools.h"
#include "sdb.h"

using namespace std;

#define	PEEKSIZE 8

static vector<instruction1> orig_instructions;
static map<string, int> regs_offset = init_regs_offset();
static breakpoint bp_restore = {};

void errquit(const char *msg)
{
	perror(msg);
	exit(-1);
}

vector<string> splitStr2Vec(string s, string splitSep)
{
    vector<string> result;
    int current = 0;
    int next = 0;
    while (next != -1)
    {
        next = s.find_first_of(splitSep, current);
        if (next != current)
        {
            string tmp = s.substr(current, next - current);
            if(!tmp.empty())
            {
                result.push_back(tmp);
            }
        }
        current = next + 1;
    }
    return result;
}

string hex_to_char(const long code)
{
	unsigned char* ch = (unsigned char*)(&code);
	string out = "";

	for(int i=0;i<8;i++)
	{	
		if (int(ch[i])>=32 && int(ch[i])<=126)
			out += ch[i];
		else
			out += ".";
	}

    return out;
}

void get_orig_code(const prog_info info)
{
	csh cshandle;

	if (cs_open(CS_ARCH_X86, CS_MODE_64, &cshandle) != CS_ERR_OK)
	{
       	fprintf(stderr, "** cs_open error.\n");
        return;
    }

	cs_option(cshandle, CS_OPT_DETAIL, CS_OPT_ON);
	cs_option(cshandle, CS_OPT_SKIPDATA, CS_OPT_ON);

	unsigned long long addr = info.entry;

	while(addr < info.entry+info.size)
	{
		int dis = disassemble(info.pid, cshandle, addr);
		if(dis == -1)
			return;
		addr += orig_instructions[orig_instructions.size()-1].size;
	}

	// for(int i=0;i<orig_instructions.size();i++)
	// {
	// 	print_instruction(orig_instructions[i].addr, &orig_instructions[i]);
	// }
}

void print_instruction(instruction1 *in)
{
	int i;
	char bytes[128] = "";
	if(in == NULL) {
		fprintf(stderr, "0x%012llx:\t<cannot disassemble>\n",in-> addr);
	} else {
		for(i = 0; i < in->size; i++) {
			snprintf(&bytes[i*3], 4, "%2.2x ", in->bytes[i]);
		}
		fprintf(stderr, "\t0x%llx: %-32s\t%-10s%s\n", in->addr, bytes, in->opr.c_str(), in->opnd.c_str());
	}
}

int disassemble(pid_t pid, csh cshandle, unsigned long long addr)
{
	int count;
	char buf[64] = { 0 };
	unsigned long long ptr = addr;
	cs_insn *insn;

	for(ptr = addr; ptr < addr + sizeof(buf); ptr += PEEKSIZE)
	{
		long long peek;
		errno = 0;
		peek = ptrace(PTRACE_PEEKTEXT, pid, ptr, NULL);
		if(errno != 0) break;
		memcpy(&buf[ptr-addr], &peek, PEEKSIZE);
	}

	if((count = cs_disasm(cshandle, (uint8_t*) buf, sizeof(buf)-1, addr, 0, &insn)) > 0)
	{
		instruction1 in;
		in.addr = addr;
		in.size = insn[0].size;
		in.opr  = insn[0].mnemonic;
		in.opnd = insn[0].op_str;
		memcpy(in.bytes, insn[0].bytes, insn[0].size);

		orig_instructions.push_back(in);

		// print_instruction(addr, &in);
		// addr += insn[0].size;
	}
	else
	{
		fprintf(stderr, "** cs_disasm error.\n");
		return -1;
	}
		

	cs_free(insn, count);
	return 0;
}

void set_bp(prog_info& info, unsigned long long addr)
{
	if (info.state != RUNNING)
	{
    	fprintf(stderr, "** state must be RUNNING.\n");
        return;
    }

	// if(info.entry + info.size <= addr)
	// {
	// 	fprintf(stderr, "** addr out of range\n");
	// 	return;
	// }

	/* get original text*/
	long long code = ptrace(PTRACE_PEEKTEXT, info.pid, addr, NULL);
	unsigned char *ptr = (unsigned char *) &code;
	// dump_code(addr, code);

	for(auto & it : info.bp_vec)
	{
		if(it.addr == addr)
			return;
	}
	
	breakpoint bp;
	bp.addr = addr;
	bp.orig_patch = ptr[0];
	info.bp_vec.push_back(bp);
	
	/* set break point */
	if(ptrace(PTRACE_POKETEXT, info.pid, addr, (code & 0xffffffffffffff00) | 0xcc) != 0)
		errquit("ptrace(POKETEXT)");
}

void dump(prog_info info, unsigned long long start_addr)
{	
	//dump 80 bytes data
	unsigned long long addr = start_addr;
	for(int i=0; i<5; i++)
	{
		string printable_str = "";
		fprintf(stderr, "\t%llx: ", addr);
		for(int j=0;j<2;j++)
		{
			long code;
			code = ptrace(PTRACE_PEEKTEXT, info.pid, addr, 0);
			// printable_str = string((char*) &code, 8);
			printable_str += hex_to_char(code);
			
			for(int j=0; j<8; j++)
			{
				fprintf(stderr, "%2.2x ", ((unsigned char *) (&code))[j]);
			}
			addr += 8;
		}

		fprintf(stderr, "|%s| \n", printable_str.c_str());
	}
}

void cont(prog_info& info)
{
	int status;
	if (info.state != RUNNING)
	{
        fprintf(stderr, "** state must be RUNNING.\n");
        return;
    }

	user_regs_struct regs_struct;

	// firstly, check if need to restore breakpoint 
	if(ptrace(PTRACE_GETREGS, info.pid, NULL, &regs_struct) != 0)
		errquit("ptrace(PTRACE_GETREGS)");
	
	if(regs_struct.rip == bp_restore.addr)
	{	
		// restore the orig patch and rip
		long long code = ptrace(PTRACE_PEEKTEXT, info.pid, bp_restore.addr, NULL);
		if(ptrace(PTRACE_POKETEXT, info.pid, bp_restore.addr, (code & 0xffffffffffffff00) | bp_restore.orig_patch) != 0)
			errquit("ptrace(POKETEXT)");
		
		// regs_struct.rip = regs_struct.rip-1;
		// if(ptrace(PTRACE_SETREGS, info.pid, 0, &regs_struct) != 0) errquit("ptrace(SETREGS)");

		// single step
		if(ptrace(PTRACE_SINGLESTEP, info.pid, 0, 0) < 0) errquit("ptrace@SINGLESTEP");
		if(waitpid(info.pid, &status, 0) < 0) errquit("waitpid");

		// set break point 
		if(ptrace(PTRACE_POKETEXT, info.pid, bp_restore.addr, (code & 0xffffffffffffff00) | 0xcc) != 0)
			errquit("ptrace(POKETEXT)");
		
		bp_restore={};
	}
    
    ptrace(PTRACE_CONT, info.pid, 0, 0);

	waitpid(info.pid, &status, 0);

	if(WIFSTOPPED(status))
	{
		if(ptrace(PTRACE_GETREGS, info.pid, NULL, &regs_struct) != 0)
			errquit("ptrace(PTRACE_GETREGS)");
		
		// check if stop by breakpoint
		for(auto & it : info.bp_vec)
		{
			if(it.addr == regs_struct.rip-1)
			{
				fprintf(stderr, "** breakpoint @\t");

				// find and print disasm addr in orig_instructions
				for(int i=0;i<orig_instructions.size();i++)
				{
					if(orig_instructions[i].addr == it.addr)
					{
						print_instruction(&orig_instructions[i]);
						bp_restore.addr = it.addr;
						bp_restore.orig_patch = it.orig_patch;

						regs_struct.rip = regs_struct.rip-1;
						if(ptrace(PTRACE_SETREGS, info.pid, 0, &regs_struct) != 0) errquit("ptrace(SETREGS)");
						return;
					}
				}
			}
		}
	}
	if (WIFEXITED(status))
	{
		if (WIFSIGNALED(status))
			fprintf(stderr, "** child process %d terminiated by signal (code %d)\n", info.pid, WTERMSIG(status));
		else
			fprintf(stderr, "** child process %d terminiated normally (code %d)\n", info.pid, status);
		info = {};
		info.state = NOLOAD;
	}
}

void del_bp(prog_info& info, int idx)
{
	if(info.bp_vec.size()>idx)
	{
		if(bp_restore.addr == (info.bp_vec.begin()+idx)->addr)
		{
			// fprintf(stderr, "breakpoint %llx deleted.\n", bp_restore.addr);
			bp_restore = {};
		}

		// restore the orig patch
		long long code = ptrace(PTRACE_PEEKTEXT, info.pid, (info.bp_vec.begin()+idx)->addr, NULL);
		if(ptrace(PTRACE_POKETEXT, info.pid, (info.bp_vec.begin()+idx)->addr, (code & 0xffffffffffffff00) | (info.bp_vec.begin()+idx)->orig_patch) != 0)
			errquit("ptrace(POKETEXT)");
		
		info.bp_vec.erase(info.bp_vec.begin()+idx);
		fprintf(stderr, "** breakpoint %d deleted.\n", idx);
	}
	else
	{
		fprintf(stderr, "** Not found index: %d breakpoint\n", idx);
	}
}

void disasm(prog_info& info, unsigned long long addr)
{
	if (info.state != RUNNING)
	{
        fprintf(stderr, "** state must be RUNNING.\n");
        return;
    }

	csh cshandle;

	if (cs_open(CS_ARCH_X86, CS_MODE_64, &cshandle) != CS_ERR_OK)
	{
       	fprintf(stderr, "** cs_open error.\n");
        return;
    }

	int idx = -1;
	for(int i=0;i<orig_instructions.size();i++)
	{
		if(orig_instructions[i].addr == addr)
		{
			idx = i;
			break;
		}
	}

	if(idx == -1)
	{
		fprintf(stderr, "**error addr given.\n");
        return;
	}

	for(int i=idx; i<idx+10; i++)
	{
		if(i >= orig_instructions.size())
			break;

		print_instruction(&orig_instructions[i]);
		// disassemble(info.pid, cshandle, addr);
	}
}

void quit(prog_info info)
{
	exit(0);
}

void print_reg(const prog_info info, string reg)
{
	if (info.state != RUNNING)
	{
        fprintf(stderr, "** state must be RUNNING.\n");
        return;
    }

	user_regs_struct regs_struct;
	if(ptrace(PTRACE_GETREGS, info.pid, NULL, &regs_struct) != 0)
		errquit("ptrace(PTRACE_GETREGS)");
	
	map<string, int>::iterator it = regs_offset.find(reg);
	if(it == regs_offset.end())
	{
		fprintf(stderr,  "** Not found %s\n", reg.c_str());
		return;
	}

	unsigned long long int *ptr = (unsigned long long int *) &regs_struct;
	int offset = regs_offset[reg];

	fprintf(stderr, "%s = %lld (0x%llx)\n", reg.c_str(), ptr[offset], ptr[offset]);
}

void print_all_regs(const prog_info info)
{
	if (info.state != RUNNING)
	{
        fprintf(stderr, "** state must be RUNNING.\n");
        return;
    }

	user_regs_struct regs_struct;
	if(ptrace(PTRACE_GETREGS, info.pid, NULL, &regs_struct) != 0)
		errquit("ptrace(PTRACE_GETREGS)");

	// print all regs
	unsigned long long int *ptr = (unsigned long long int *) &regs_struct;

	fprintf(stderr, "RAX %llx\tRBX %llx\tRCX %llx\tRDX %llx\n", ptr[10], ptr[4], ptr[11], ptr[12]);
	fprintf(stderr, "R8 %llx\tR9 %llx\tR10 %llx\tR11 %llx\n", ptr[9], ptr[8], ptr[7], ptr[6]);
	fprintf(stderr, "R12 %llx\tR13 %llx\tR14 %llx\tR15 %llx\n", ptr[3], ptr[2], ptr[1], ptr[0]);
	fprintf(stderr, "RDI %llx\tRSI %llx\tRBP %llx\tRSP %llx\n", ptr[14], ptr[13], ptr[4], ptr[19]);
	fprintf(stderr, "RIP %llx\tFLAGS %llx\n", ptr[16], ptr[18]);
}

void help()
{
	fprintf(stderr, "- break {instruction-address}: add a break point\n");
	fprintf(stderr, "- cont: continue execution\n");
	fprintf(stderr, "- delete {break-point-id}: remove a break point\n");
	fprintf(stderr, "- disasm addr: disassemble instructions in a file or a memory region\n");
	fprintf(stderr, "- dump addr [length]: dump memory content\n");
	fprintf(stderr, "- exit: terminate the debugger\n");
	fprintf(stderr, "- get reg: get a single value from a register\n");
	fprintf(stderr, "- getregs: show registers\n");
	fprintf(stderr, "- break {instruction-address}: add a break point\n");
	fprintf(stderr, "- help: show this message\n");
	fprintf(stderr, "- list: list break points\n");
	fprintf(stderr, "- load {path/to/a/program}: load a program\n");
	fprintf(stderr, "- run: run the program\n");
	fprintf(stderr, "- vmmap: show memory layout\n");
	fprintf(stderr, "- set reg val: get a single value to a register\n");
	fprintf(stderr, "- si: step into instruction\n");
	fprintf(stderr, "- start: start the program and stop at the first instruction\n");
}

void list(const prog_info info)
{
	if(!info.bp_vec.empty())
	{
		for(int i=0; i<info.bp_vec.size(); i++)
		{
			fprintf(stderr, " %d: %llx\n", i, info.bp_vec[i].addr);
		}
	}
	else
	{
		fprintf(stderr, "** No breakpoint\n");
	}
}

void load(prog_info& info, string path)
{
	if(info.state != NOLOAD)
	{
		fprintf(stderr, "** state must be NOT LOADED.\n");
		return;
	}

	info = elf_read(path);
	fprintf(stderr, "** program '%s' loaded. entry point 0x%llx\n", path.c_str(), info.entry);
	info.state = LOADED;
	info.path = path;
}

void run(prog_info& info)
{
	if (info.state == RUNNING) 
	{
		fprintf(stderr, "** program '%s' is already running.\n", info.path.c_str());
		cont(info);
    }
    else if (info.state == LOADED)
	{
        start(info);
        cont(info);
    }
    else
	{
		fprintf(stderr, "** state must be LOADED or RUNNING.\n");
    }
}

void vmmap(const prog_info info)
{
	if(info.state != RUNNING)
	{
		fprintf(stderr, "** state must be NOT LOADED.\n");
		return;
	}
	map<range_t, map_entry_t> vmmap;
	map<range_t, map_entry_t>::iterator vi;

	if(load_maps(info.pid, vmmap) <= 0) {
		fprintf(stderr, "** cannot load memory mappings.\n");
		return;
	}

	for(vi = vmmap.begin(); vi != vmmap.end(); vi++)
	{
		char perm[3] = {'-','-','-'};
		
		if(vi->second.perm & 0x04)
			perm[0] = 'r';
		if(vi->second.perm & 0x02)
			perm[1] = 'w';
		if(vi->second.perm & 0x01)
			perm[2] = 'x';

		fprintf(stderr, "%16llx-%16llx %s %s\n",
			vi->second.range.begin, vi->second.range.end,
			perm, vi->second.name.c_str());
	}
}

void set(prog_info& info, string reg, long long val)
{
	if(info.state != RUNNING)
	{
		fprintf(stderr,  "** program must be RUNNING.\n");
		return;
	}

	user_regs_struct regs_struct;
	if(ptrace(PTRACE_GETREGS, info.pid, NULL, &regs_struct) != 0)
		errquit("ptrace(PTRACE_GETREGS)");
	
	map<string, int>::iterator it = regs_offset.find(reg);
	if(it == regs_offset.end())
	{
		fprintf(stderr,  "** Not found %s\n", reg.c_str());
		return;
	}

	unsigned long long int *ptr = (unsigned long long int *) &regs_struct;
	int offset = regs_offset[reg];

	ptr[offset] = val;

	ptrace(PTRACE_SETREGS, info.pid, NULL, &regs_struct);
}

void si(const prog_info info)
{
	int status;
	if(info.state!=RUNNING)
	{
		fprintf(stderr,  "** state must be RUNNING.\n");
		return;
	}

	if(ptrace(PTRACE_SINGLESTEP, info.pid, 0, 0) < 0) errquit("ptrace@SINGLESTEP");
	waitpid(info.pid, &status, 0);
}

void start(prog_info& info)
{
	pid_t child;
	if (info.state != LOADED)
	{
	        fprintf(stderr,  "** state must be LOADED.\n");
			return;
    	}
	if(info.state == RUNNING)
	{
		fprintf(stderr,  "** program '%d' is already running.\n", info.pid);
		return;
	}

	// fork
	if((child = fork()) < 0) errquit("** fork error");
	if(child == 0)
	{
		if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) errquit("** ptrace error");
		execlp(info.path.c_str(), info.path.c_str(), NULL);
		errquit("** execvp error");
	}
	else
	{
		int status;
		if(waitpid(child, &status, 0) < 0) errquit("** waitpid error");
		assert(WIFSTOPPED(status));
		ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_EXITKILL);

		fprintf(stderr,  "** pid %d\n", child);
		info.state = RUNNING;
		info.pid = child;

		get_orig_code(info);
	}
}

int main(int argc, char *argv[])
{
	int cmd_opt = 0;
	bool ifprint = false;
	string arg_script;
	string arg_prog;
	prog_info info;
	FILE *fp;

	while((cmd_opt = getopt(argc, argv, "s:")) != -1)
	{
		switch(cmd_opt){
			case 's':
				arg_script = optarg;
				if((fp = freopen(arg_script.c_str(), "r", stdin)) == NULL)
					exit(-1);
				ifprint = true;
				break;
			case '?':
				help();
				return -1;
		}
	}
	
	if(argc > optind)
	{
		arg_prog = argv[optind];
		load(info, arg_prog);
	}
	
	char line[1024];
	if(!ifprint)
		fprintf(stderr, "sdb> ");

	while(fgets(line, 1024, stdin))
	{
		if(line[strlen(line)-1] == '\n')
		 	line[strlen(line)-1] = '\0';

		vector<string> input = splitStr2Vec(string(line), " ");
		if(input.empty()) continue;
		
		string cmd = input[0];

		// fprintf(stderr, "cmd: %s\n", cmd.c_str());
		if(strcmp(cmd.c_str(), "break")==0 || strcmp(cmd.c_str(), "b")==0)
		{
			if(input.size()>=2)
			{
				set_bp(info, stoull(input[1], NULL, 16));
			}
			else
			{
				fprintf(stderr, "** no addr given.\n");
			}
		}
		else if(strcmp(cmd.c_str(), "cont")==0 || strcmp(cmd.c_str(), "c")==0)
		{
			cont(info);
		}
		else if(strcmp(cmd.c_str(), "delete")==0)
		{
			del_bp(info, stoi(input[1]));
		}
		else if(strcmp(cmd.c_str(), "disasm")==0 || strcmp(cmd.c_str(), "d")==0)
		{
			if(input.size()>=2)
			{
				disasm(info, stoull(input[1], NULL, 16));
			}
			else
			{
				fprintf(stderr, "** no addr given.\n");
			}	
		}
		else if(strcmp(cmd.c_str(), "dump")==0 || strcmp(cmd.c_str(), "x")==0)
		{
			if(input.size()>=2)
			{
				dump(info, stoull(input[1], NULL, 16));
			}
			else
			{
				fprintf(stderr, "** no addr given.\n");
			}
		}
		else if(strcmp(cmd.c_str(), "exit")==0 || strcmp(cmd.c_str(), "q")==0)
		{
			quit(info);
		}
		else if(strcmp(cmd.c_str(), "get")==0 || strcmp(cmd.c_str(), "g")==0)
		{
			print_reg(info, input[1]);
		}
		else if(strcmp(cmd.c_str(), "getregs")==0)
		{	
			print_all_regs(info);
		}
		else if(strcmp(cmd.c_str(), "help")==0 || strcmp(cmd.c_str(), "h")==0)
		{
			help();
		}
		else if(strcmp(cmd.c_str(), "list")==0 || strcmp(cmd.c_str(), "l")==0)
		{
			list(info);
		}
		else if(strcmp(cmd.c_str(), "load") ==0)
		{
			// fprintf(stderr, "load\n");
			load(info, input[1]);
		}
		else if(strcmp(cmd.c_str(), "run")==0 || strcmp(cmd.c_str(), "r")==0)
		{
			run(info);
		}
		else if(strcmp(cmd.c_str(), "vmmap")==0 || strcmp(cmd.c_str(), "m")==0)
		{
			vmmap(info);
		}
		else if(strcmp(cmd.c_str(), "set")==0)
		{
			if(input.size()>=3)
			{
				set(info, input[1], stol(input[2], NULL, 16));
			}
			else
			{
				fprintf(stderr, "** input error.\n");
			}
			
		}
		else if(strcmp(cmd.c_str(), "si")==0)
		{
			si(info);
		}
		else if(strcmp(cmd.c_str(), "start")==0)
		{
			// fprintf(stderr, "start\n");
			start(info);
		}
		else
		{
			fprintf(stderr, "error commond\n");
		}

		if(!ifprint)
			fprintf(stderr, "sdb> ");
	}

	return 0;
}

