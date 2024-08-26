//  trace-prodos.c
//
//  Copyright (c) 2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

struct cnm {
    byte code;
    const char *name;
    const byte *handling;
};
struct cnm cmd_name_map[] = {
    {0xC0, "CREATE",            NULL},
    {0xC1, "DESTROY",           NULL},
    {0xC2, "RENAME",            NULL},
    {0xC3, "SET_FILE_INFO",     NULL},
    {0xC4, "GET_FILE_INFO",     NULL},
    {0xC5, "ON_LINE",           NULL},
    {0xC6, "SET_PREFIX",        NULL},
    {0xC7, "GET_PREFIX",        NULL},
    {0xC8, "OPEN",              NULL},
    {0xC9, "NEWLINE",           NULL},
    {0xCA, "READ",              NULL},
    {0xCB, "WRITE",             NULL},
    {0xCC, "CLOSE",             NULL},
    {0xCD, "FLUSH",             NULL},
    {0xCE, "SET_MARK",          NULL},
    {0xCF, "GET_MARK",          NULL},
    {0xD0, "SET_EOF",           NULL},
    {0xD1, "GET_EOF",           NULL},
    {0xD2, "SET_BUF",           NULL},
    {0xD3, "GET_BUF",           NULL},
    {0x82, "GET_TIME",          NULL},
    {0x40, "ALLOC_INTERRUPT",   NULL},
    {0x41, "DEALLOC_INTERRUPT", NULL},
    {0x80, "READ_BLOCK",        NULL},
    {0x81, "WRITE_BLOCK",       NULL},
};

struct ecm {
    byte code;
    const char *msg;
};
struct ecm err_code_map[] = {
    {0x00, "No error"},
    {0x01, "Bad syscall num"},
    {0x04, "Bad syscall param count"},
    {0x25, "Interrupt table full"},
    {0x27, "I/O Error"},
    {0x28, "No device"},
    {0x2B, "Disk write protected"},
    {0x2E, "Disk switched"},
    {0x40, "Invalid pathname"},
    {0x42, "File Control Block table full"},
    {0x43, "Invalid refnum"},
    {0x44, "Path not found"},
    {0x45, "Volume directory not found"},
    {0x46, "File not found"},
    {0x47, "Duplicate filename"},
    {0x48, "Overrun error"},
    {0x49, "Volume directory full"},
    {0x4A, "Incompatible file format"},
    {0x4B, "Unsupported storage_type"},
    {0x4C, "EOF encountered"},
    {0x4D, "Position out of range"},
    {0x4E, "Access error"},
    {0x50, "File is open"},
    {0x51, "Directory count error"},
    {0x52, "Not a ProDOS disk"},
    {0x53, "Invalid parameter"},
    {0x55, "Voume Control Block table full"},
    {0x56, "Bad buffer address"},
    {0x57, "Duplicate volume"},
    {0x5A, "Bit map disk address is impossible"},
};

struct tracker {
    struct tracker *next;
    int id;
    byte cmd;
    word loc;
    size_t aloc;
    MemAccessType type;
};

static struct tracker *head = NULL;

//

static unsigned int mli_cmd_num = 0;

//

static struct cnm *find_cmd(byte code)
{
    struct cnm *cur = cmd_name_map;
    struct cnm *end = cur + (sizeof cmd_name_map / sizeof cmd_name_map[0]);

    for (; cur != end; ++cur) {
        if (cur->code == code) return cur;
    }

    return NULL;
}

static const char *find_err_msg(byte errcode)
{
    struct ecm *cur = err_code_map;
    struct ecm *end = cur + (sizeof err_code_map / sizeof err_code_map[0]);

    for (; cur != end; ++cur) {
        if (cur->code == errcode) return cur->msg;
    }

    return "UNKNOWN ERROR";
}

static void check_returns(void)
{
    // Check to see if we're at a recorded MLI return
    struct tracker **cur_ptr = &head;

    while ((*cur_ptr) != NULL) {
        struct tracker *cur = (*cur_ptr);
        if (cur->loc == PC) {
            // PC == loc, but make sure it's the same actual memory
            size_t aloc;
            MemAccessType type;
            mem_get_true_access(cur->loc, false /* reading */, &aloc, NULL,
                                &type);
            if (aloc == cur->aloc && type == cur->type) {
                struct cnm *cmdrec = find_cmd(cur->cmd);
                const char *name = "UNKNOWN";
                if (cmdrec) name = cmdrec->name;
                fprintf(trfile, "PD MLI #%u @$%04X %s_$%02X returned: ",
                        (unsigned int)cur->id, (unsigned int)cur->loc,
                        name, (unsigned int)cur->cmd);
                if (PTEST(PZERO)) {
                    fprintf(trfile, "Success!\n");
                }
                else {
                    const char *msg = find_err_msg(ACC);
                    fprintf(trfile, "ERROR $%02X: %s\n", (unsigned int)ACC,
                            msg);
                }
                *cur_ptr = cur->next;
                free(cur);
                return;
            }
        }

        cur_ptr = &cur->next;
    }
}

static void print_handling(word plist, const byte *handling)
{
    (void) handling; // unused for now
    
    // Print two bytes per parameter, for now.
    byte nparam = peek_sneaky(plist++);
    fprintf(trfile, "%u,", (unsigned int)nparam);
    for (byte i=0; i != nparam; ++i) {
        fprintf(trfile, " $%02X", peek_sneaky(plist++));
        fprintf(trfile, " $%02X", peek_sneaky(plist++));
    }
    fprintf(trfile, "...)\n");
}

//

extern void trace_prodos(void)
{
    if (!cfg.trace_prodos)
        return;

    // Check for MLI returns
    check_returns();

    // Return unless we have JSR $BF00
    if (peek_sneaky(PC) != 0x20 /* JSR */
        || peek_sneaky(PC+1) != 0x00
        || peek_sneaky(PC+2) != 0xBF)
        return; 

    byte cmd = peek_sneaky(PC+3);
    word plist = WORD(peek_sneaky(PC+4), peek_sneaky(PC+5));

    struct cnm *cmdrec = find_cmd(cmd);
    const char *name;
    const byte *handling = NULL;
    if (cmdrec) {
        name = cmdrec->name;
        handling = cmdrec->handling;
    }
    else {
        name = "UNKNOWN";
    }

    // Display the command name
    fprintf(trfile, "PD MLI #%u @$%04X: %s_$%02X(", ++mli_cmd_num,
            (unsigned int)PC, name, (unsigned int)cmd);
    print_handling(plist, handling);
    fprintf(trfile, ")\n");

    struct tracker *tnew = xalloc(sizeof *tnew);
    tnew->next = head;
    tnew->id = mli_cmd_num;
    tnew->cmd = cmd;
    tnew->loc = PC + 6;
    mem_get_true_access(tnew->loc, false /* reading */, &tnew->aloc,
                        NULL, &tnew->type);
    head = tnew;
}
