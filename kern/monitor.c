// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

int mon_color(int argc, char **argv, struct Trapframe *tf);
int mon_dumpvm(int argc, char **argv, struct Trapframe *tf);
int mon_setperm(int argc, char **argv, struct Trapframe *tf);
int mon_showmappings(int argc, char **argv, struct Trapframe *tf);

struct Command {
    const char *name;
    const char *desc;
    // return -1 to force monitor to exit
    int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// LAB 1: added backtrace command & show command
static struct Command commands[] = {
    { "help", "Display this list of commands", mon_help },
    { "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Display a listing of function call frames", mon_backtrace },
    { "show", "Display colorful ASCII art", mon_color },
    { "showmappings", "Display physical page mappings and permissions for a range of virtual addresses", mon_showmappings },
    { "setperm", "Set or clear permission bits of a mapping", mon_setperm },
    { "dumpvm", "Dump memory contents of a virtual address range", mon_dumpvm },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

// Lab 1 Extra Credit
int
mon_color(int argc, char **argv, struct Trapframe *tf)
{
	// Array of colors
	int colors[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};

	const char *art[] = {
		"**BLUE**",
		"**GREEN**",
		"**CYAN**",
		"**RED**",
		"**MAGENTA**",
		"**YELLOW**",

	};

	int art_height = sizeof(art) / sizeof(art[0]);

	// Changes the color of each line printed
    for (int i = 0; i < art_height; i++) 
	{
        cprintf("\033[%dm%s\033[0m\n", 30 + colors[i], art[i]);
    }

    return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Starts with current ebp
    uint32_t *ebp = (uint32_t *)read_ebp();
    cprintf("Stack backtrace:\n");

    while (ebp) {
		 // Return address is EBP + 1
        uint32_t eip = ebp[1];

        // Print current frame's EBP and EIP
        cprintf("  ebp %08x  eip %08x  args", ebp, eip);

        // Print up to 5 arguments; these are stored just after the return address
        for (int i = 0; i < 5; i++) {
            cprintf(" %08x", ebp[2 + i]); 
        }
        cprintf("\n");

        // Print function names using kernel debugging information
        struct Eipdebuginfo info;
        if (debuginfo_eip(eip, &info) == 0) {  // Retrieves debugging information for EIP
            cprintf("     %s:%d: %.*s+%d\n", 
                info.eip_file, info.eip_line, info.eip_fn_namelen, 
                info.eip_fn_name, eip - info.eip_fn_addr);
        }

        // Move to previous stack frame
        ebp = (uint32_t *)ebp[0]; 
    }
    return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    // Check if the current nummber of arguments is provided
    if (argc != 3) {
        cprintf("Usage: showmappings <first virtual address> <last virtual address>\n");
        return 0;
    }

    // Parse the frst virtual address
    char *endptr;
    uintptr_t va_start = strtol(argv[1], &endptr, 0);
    if (*endptr != '\0') {
        cprintf("Invalid first virtual address\n");
        return 0;
    }

    // Parse the last virtual address
    uintptr_t va_end = strtol(argv[2], &endptr, 0);
    if (*endptr != '\0') {
        cprintf("Invalid last virtual address\n");
        return 0;
    }

    // Swap if start address is greater than end address
    if (va_start > va_end) {
        uintptr_t temp = va_start;
        va_start = va_end;
        va_end = temp;
    }

    // Round addresses down to page boundaries
    va_start = ROUNDDOWN(va_start, PGSIZE);
    va_end = ROUNDDOWN(va_end, PGSIZE);

    // Loop through the address range
    for (uintptr_t va = va_start; va <= va_end; va += PGSIZE) {
        pte_t *pte = pgdir_walk(kern_pgdir, (void *)va, 0);
        if (pte && (*pte & PTE_P)) {
            physaddr_t pa = PTE_ADDR(*pte);
            cprintf("0x%08x: 0x%08x", va, pa);

            // Print PTE flags
            if (*pte & PTE_P) cprintf(" PTE_P");
            if (*pte & PTE_W) cprintf(" PTE_W");
            if (*pte & PTE_U) cprintf(" PTE_U");
            if (*pte & PTE_PWT) cprintf(" PTE_PWT");
            if (*pte & PTE_PCD) cprintf(" PTE_PCD");
            if (*pte & PTE_A) cprintf(" PTE_A");
            if (*pte & PTE_D) cprintf(" PTE_D");
            if (*pte & PTE_PS) cprintf(" PTE_PS");
            if (*pte & PTE_G) cprintf(" PTE_G");
            cprintf("\n");
        } else {
            cprintf("0x%08x: not mapped\n", va);
        }
    }
    return 0;
}

int
mon_setperm(int argc, char **argv, struct Trapframe *tf)
{
    // Check if correct number of arguments is provided
    if (argc != 4) {
        cprintf("Usage: setperm <virtual address> <P|W|U> <0|1: clear or set>\n");
        return 0;
    }

    // Parse virtual address from the input string
    char *endptr;
    uintptr_t va = strtol(argv[1], &endptr, 0);
    if (*endptr != '\0') {
        cprintf("Invalid virtual address\n");
        return 0;
    }

    // Parse permission bit
    char perm = argv[2][0];
    if (argv[2][1] != '\0' || (perm != 'P' && perm != 'W' && perm != 'U')) {
        cprintf("Invalid permission bit: must be P, W, or U\n");
        return 0;
    }

    // Parse action 
    int action = strtol(argv[3], &endptr, 0);
    if (*endptr != '\0' || (action != 0 && action != 1)) {
        cprintf("Invalid action: must be 0 or 1\n");
        return 0;
    }

    // Retrieve the PTE
    pte_t *pte = pgdir_walk(kern_pgdir, (void *)va, 0);
    if (!pte || !(*pte & PTE_P)) {
        cprintf("Virtual address 0x%08x is not mapped\n", va);
        return 0;
    }

    // Display old permissions
    cprintf("0x%08x: ", va);
    cprintf("PTE_P");
    if (*pte & PTE_W) cprintf(" PTE_W");
    if (*pte & PTE_U) cprintf(" PTE_U");
    cprintf(" -> ");

    // Modify the permission bit
    uint32_t perm_bit = (perm == 'P') ? PTE_P : (perm == 'W') ? PTE_W : PTE_U;
    if (action == 1) {
        *pte |= perm_bit;
    } else {
        *pte &= ~perm_bit;
    }

    // Invalidate TLB entry
    invlpg((void *)va);

    // Display new permissions
    cprintf("PTE_P");
    if (*pte & PTE_W) cprintf(" PTE_W");
    if (*pte & PTE_U) cprintf(" PTE_U");
    cprintf("\n");

    return 0;
}

int
mon_dumpvm(int argc, char **argv, struct Trapframe *tf)
{
    // Check if correct number of arguments is provided
    if (argc != 3) {
        cprintf("Usage: dumpvm <start virtual address> <end virtual address>\n");
        return 0;
    }

    // Parse the start and end addresses
    char *endptr;
    uintptr_t start_addr = strtol(argv[1], &endptr, 0);
    if (*endptr != '\0') {
        cprintf("Invalid start virtual address\n");
        return 0;
    }
    uintptr_t end_addr = strtol(argv[2], &endptr, 0);
    if (*endptr != '\0') {
        cprintf("Invalid end virtual address\n");
        return 0;
    }

    // Swap if start address is greater than end address
    if (start_addr > end_addr) {
        uintptr_t temp = start_addr;
        start_addr = end_addr;
        end_addr = temp;
    }

    // Align start address to the boundary
    uintptr_t addr = start_addr & ~0xF;

    while (addr <= end_addr) {
        cprintf("%08x: ", addr);

        // Print bytes in the current line
        for (int i = 0; i < 16; i++) {
            uintptr_t curr_addr = addr + i;
            if (curr_addr >= start_addr && curr_addr <= end_addr) {
                // Check if address is mapped
                pte_t *pte = pgdir_walk(kern_pgdir, (void *)curr_addr, 0);
                if (pte && (*pte & PTE_P)) {
                    uint8_t *p = (uint8_t *)curr_addr;
                    cprintf("%02x ", *p);
                } else {
                    cprintf("XX ");
                }
            } else {
                cprintf("   "); 
            }
        }
        cprintf("\n");
        addr += 16;
    }
    return 0;
}


void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

