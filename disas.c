/*

Модуль дизассемблирования.

Панасенко Дмитрий Игоревич, Маткин Илья Александрович     02.05.2020

*/


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "disas.h"
#include "mz.h"
#include "system.h"

char* registersByte[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char* registersWord[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char* regmem[8][5] = { "[bx + si]", "[bx + si", "[bx + si", "al", "ax",
                       "[bx + di]", "[bx + di", "[bx + di", "cl", "cx",
                       "[bp + si]", "[bp + si", "[bp + si", "dl", "dx",
                       "[bp + di]", "[bp + di", "[bp + di", "bl", "bx",
                       "[si]", "[si", "[si", "ah", "sp",
                       "[di]", "[di", "[di", "ch", "bp",
                       "[", "[bp", "bp", "dh", "si",
                       "[bx]", "[bx", "[bx", "bh", "di" 
                     };
char* Sreg[] = { "es", "cs", "ss", "ds" };

int change_seg = -1;

void swap_strings(char* source, char* reciever)
{
    char tmp[50] = "";
    strcpy(tmp, source);
    strcpy(source, reciever);
    strcpy(reciever, tmp);
}

void get_offset(MZHeaders* mz, DWORD pos, BOOL byte, char* string)
{
    if (byte == TRUE)
    {
        int offset = *(mz->code + pos);
        sprintf(string, "%04xh", offset);
        return;
    }
    
    int offset = *(mz->code + pos);
    offset = offset << 8;
    offset += *(mz->code + pos - 1);
    sprintf(string, "%04xh", offset);
    return;
}

void reg_mod(MZHeaders* mz, DWORD pos, char* buffer)
{
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE w = cop & 1;

    if (w == 0)
    {
        strcpy(buffer, registersByte[reg]);
    }
    else
    {
        strcpy(buffer, registersWord[reg]);
    }

    return;
}

DWORD fill_jmp(MZHeaders* mz, DWORD pos, char** inst)
{
    char buffer[50] = "";

    itoa(mz->doshead->e_cs + IMAGE_BASE_SEG, buffer, 16);
    strcat(*inst, buffer);
    strcat(*inst, ":");
    char offset = *(mz->code + pos + 1);
    offset += pos + 2;
    sprintf(buffer, "%04x", offset);
    strcat(*inst, buffer);

    return 2;
}

DWORD fill_call(MZHeaders* mz, DWORD pos, char** inst, BOOL is_far)
{
    char buffer[50] = "";
    int size = 0;
    
    if (!is_far)
    {
        WORD offset = 0;
        itoa(mz->doshead->e_cs + IMAGE_BASE_SEG, buffer, 16);
        strcat(*inst, buffer);
        strcat(*inst, ":");
        size = 3;
        offset = *(mz->code + pos + 2);
        offset = offset << 8;
        offset += *(mz->code + pos + 1);
        offset += pos + 3;
        sprintf(buffer, "%04x", offset);
        strcat(*inst, buffer);
    }
    else
    {
        strcat(*inst, "far ptr ");
        DWORD offset = *(mz->code + pos + 2);
        offset = offset << 8;
        offset += *(mz->code + pos + 1);
        offset = offset << 8;
        offset += *(mz->code + pos + 4);
        offset = offset << 8;
        offset += *(mz->code + pos + 3);
        size = 5;
        sprintf(buffer, "%04x:", LOWORD(offset));
        strcat(*inst, buffer);
        sprintf(buffer, "%04x", HIWORD(offset));
        strcat(*inst, buffer);
    } 

    return size;
}

DWORD fill_wam(MZHeaders* mz, DWORD pos, char** inst, BYTE cop, BYTE ds)
{
    char left[50] = "", right[50] = "";

    DWORD size = addr_mod(mz, pos, left);
    reg_mod(mz, pos, right);

    if (ds == 1)
    {
        swap_strings(left, right);
    }

    strcat(*inst, left);
    strcat(*inst, ", ");
    strcat(*inst, right);

    return size;
}

DWORD fill_woam(MZHeaders* mz, DWORD pos, char** inst, BYTE w)
{
    char buffer[50] = "";
    DWORD size = 0;
    if (w == 0)
    {
        strcat(*inst, "al, ");
        get_offset(mz, pos + 1, TRUE, buffer);
        size = 2;
    }
    else if (w == 1)
    {
        strcat(*inst, "ax, ");
        get_offset(mz, pos + 2, FALSE, buffer);
        size = 3;
    }
    strcat(*inst, buffer);

    return size;
}

DWORD fill_group80(MZHeaders* mz, DWORD pos, char** inst, BYTE w, BYTE s)
{
    char left[50] = "", right[50] = "";

    DWORD size = addr_mod(mz, pos, left);
    if (w == 1 && s == 1)
    {
        get_offset(mz, pos + 2, TRUE, right);
        size += 1;
    }
    else if (w == 1 && s == 0)
    {
        get_offset(mz, pos + 3, FALSE, right);
        size += 2;
    }
    else
    {
        get_offset(mz, pos + 2, TRUE, right);
        size += 1;
    }

    strcat(*inst, left);
    strcat(*inst, ", ");
    strcat(*inst, right);

    return size;
}

DWORD fill_groupF6(MZHeaders* mz, DWORD pos, char** inst)
{
    char left[50] = "";

    DWORD size = addr_mod(mz, pos, left);

    strcat(*inst, left);

    return size;
}

DWORD fill_groupD0(MZHeaders* mz, DWORD pos, char** inst, BYTE cop)
{
    char left[50] = "", right[50] = "";

    DWORD size = addr_mod(mz, pos, left);
    
    if (cop == 0xD0 || cop == 0xD1)
    {
        strcpy(right, "1");
    }
    else if (cop == 0xD2 || cop == 0xD3)
    {
        strcpy(right, "cl");
    }

    strcat(*inst, left);
    strcat(*inst, ", ");
    strcat(*inst, right);

    return size;
}

DWORD addr_mod(MZHeaders* mz, DWORD pos, char* buffer)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE mod = ra & 0xC0;
    mod = mod >> 6;
    BYTE rm = ra & 0x7;
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;

    char tmp[50];

    int mmod = mod == 3 ? mod + w : mod;

    if (w == 0 && mod < 3)
    {
        strcpy(buffer, "byte ptr ");
    }
    else if (w == 1 && mod < 3)
    {
        strcpy(buffer, "word ptr ");
    }

    if (change_seg != -1)
    {
        strcat(buffer, Sreg[change_seg]);
        strcat(buffer, ":");
        change_seg = -1;
    }

    strcat(buffer, regmem[rm][mmod]);
    if (mmod == 1)
    {
        BYTE offset = *(mz->code + pos + 2);
        itoa(offset, tmp, 10);
        if (offset >= 0)
            strcat(buffer, " + ");
        else
            strcat(buffer, " - ");
        strcat(buffer, tmp);
        strcat(buffer, "]");
        size = 3;
    }
    else if (mmod == 2 || (mmod == 0 && rm == 6))
    {
        WORD offset = *(mz->code + pos + 3);
        offset = offset << 8;
        offset += *(mz->code + pos + 2);
        itoa(offset, tmp, 10);
        if (!(mmod == 0 && rm == 6))
        {
            if (offset >= 0)
                strcat(buffer, " + ");
            else 
                strcat(buffer, " - ");
        }
        strcat(buffer, tmp);
        strcat(buffer, "]");
        size = 4;
    }
    else
    {
        size = 2;
    }

    return size;
}


// массив опкодов (от 00 до FF)
d_func opcodes[] = {
    d_add,    d_add,    d_add,    d_add,    d_add,    d_add,    d_push,   d_pop,
    d_or,     d_or,     d_or,     d_or,     d_or,     d_or,     d_push,   d_unk,
    d_adc,    d_adc,    d_adc,    d_adc,    d_adc,    d_adc,    d_push,   d_pop,
    d_sbb,    d_sbb,    d_sbb,    d_sbb,    d_sbb,    d_sbb,    d_push,   d_pop,
    d_and,    d_and,    d_and,    d_and,    d_and,    d_and,    d_es,     d_daa,
    d_sub,    d_sub,    d_sub,    d_sub,    d_sub,    d_sub,    d_cs,     d_das,
    d_xor,    d_xor,    d_xor,    d_xor,    d_xor,    d_xor,    d_ss,     d_aaa,
    d_cmp,    d_cmp,    d_cmp,    d_cmp,    d_cmp,    d_cmp,    d_ds,     d_aas,
    d_inc,    d_inc,    d_inc,    d_inc,    d_inc,    d_inc,    d_inc,    d_inc,
    d_dec,    d_dec,    d_dec,    d_dec,    d_dec,    d_dec,    d_dec,    d_dec,
    d_push,   d_push,   d_push,   d_push,   d_push,   d_push,   d_push,   d_push,
    d_pop,    d_pop,    d_pop,    d_pop,    d_pop,    d_pop,    d_pop,    d_pop,
    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,
    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,
    d_jo,     d_jno,    d_jb,     d_jae,    d_jz,     d_jnz,    d_jbe,    d_ja,
    d_js,     d_jns,    d_jpe,    d_jpo,    d_jl,     d_jge,    d_jle,    d_jg,  
    d_gr80,   d_gr80,   d_gr80,   d_gr80,   d_test,   d_test,   d_xchg,   d_xchg,
    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_lea,    d_mov,    d_pop,
    d_nop,    d_xchg,   d_xchg,   d_xchg,   d_xchg,   d_xchg,   d_xchg,   d_xchg,
    d_cbw,    d_cwd,    d_call,   d_wait,   d_pushf,  d_popf,   d_sahf,   d_lahf, 
    d_mov,    d_mov,    d_mov,    d_mov,    d_movsb,  d_movsw,  d_cmpsb,  d_cmpsw,
    d_test,   d_test,   d_stosb,  d_stosw,  d_lodsb,  d_lodsw,  d_scasb,  d_scasw,
    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,
    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,    d_mov,
    d_unk,    d_unk,    d_retn,   d_retn,   d_les,    d_lds,    d_mov,    d_mov,
    d_unk,    d_unk,    d_retf,   d_retf,   d_int3,   d_int,    d_into,   d_iret,
    d_grd0,   d_grd0,   d_grd0,   d_grd0,   d_aam,    d_aad,    d_unk,    d_xlat,
    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,    d_unk,
    d_loopnz, d_loopz,  d_loop,   d_jcxz,   d_in,     d_in,     d_out,    d_out,
    d_call,   d_jmp,    d_jmp,    d_jmp,    d_in,     d_in,     d_out,    d_out,
    d_lock,   d_unk,    d_rep,    d_rep,    d_hlt,    d_cmc,    d_grf6,   d_grf6,
    d_clc,    d_stc,    d_cli,    d_sti,    d_cld,    d_std,    d_grfe,   d_grff 
};

//
// функция вывода одной дизассемблированной инстркуции на экран
//
void PrintInstruction(MZHeaders *mz, DWORD pos, DWORD inst_len, char *inst) {
    unsigned int i;

    if (change_seg == -1)
    {
        printf("%04X:%04X ", mz->doshead->e_cs + IMAGE_BASE_SEG + (SIZE_OF_SEG_P * (pos / SIZE_OF_SEG_B)), pos % SIZE_OF_SEG_B);
        if (*(mz->code + pos - 1) == 0x2E || *(mz->code + pos - 1) == 0x26 || *(mz->code + pos - 1) == 0x36 || *(mz->code + pos - 1) == 0x3E)
        {
            for (i = 0; i < inst_len + 1; i++) {
                printf("%02X", (mz->code + pos)[i-1]);
            }
            for (i = inst_len + 1; i < INSTRUCTION_LEN; i++) {
                printf("  ");
            }
        }
        else
        {
            for (i = 0; i < inst_len; i++) {
                printf("%02X", (mz->code + pos)[i]);
            }
            for (i = inst_len; i < INSTRUCTION_LEN; i++) {
                printf("  ");
            }
        }
        
        printf(" %s\n", inst);
    }

    return;
}


//
// функция дизассемблирования сегмента кода
//
void DisasCodeSeg(MZHeaders *mz) {
    DWORD pos = 0, inst_len;
    char inst[256];
    
    printf("Disassembled code segment\n");

    ApplyRelocs(mz, IMAGE_BASE_SEG);
    while (pos < mz->code_size) {
        inst_len = opcodes[*(mz->code + pos)](mz, pos, inst);
        PrintInstruction(mz, pos, inst_len, inst);
        pos += inst_len;
    }
    RemoveRelocs(mz, IMAGE_BASE_SEG);

    return;
}


//
// функции обработки команд
//
DWORD d_aaa(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "aaa");
    return 1;
}

DWORD d_aad(MZHeaders *mz, DWORD pos, char *inst){
    if (*(mz->code + pos + 1) == 10) {
        strcpy(inst, "aad");
    }
    else {
        sprintf(inst, "aad    %u", *(mz->code + pos + 1));
    }
    return 2;
}

DWORD d_aam(MZHeaders *mz, DWORD pos, char *inst){
    if (*(mz->code + pos + 1) == 10) {
        strcpy(inst, "aam");
    }
    else {
        sprintf(inst, "aam    %u", *(mz->code + pos + 1));
    }
    return 2;
}

DWORD d_aas(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "aas");
    return 1;
}

DWORD d_adc(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE mod = ra & 0xC0;
    mod = mod >> 6;
    BYTE rm = ra & 0x7;
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "adc\t");

    if (cop == 0x14 || cop == 0x15)
    {
        return fill_woam(mz, pos, &inst, w);
    }
    
    if (cop >= 0x10 && cop <= 0x13)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_add(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE mod = ra & 0xC0;
    mod = mod >> 6;
    BYTE rm = ra & 0x7;
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "add\t");

    if (cop == 0x04 || cop == 0x05)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x00 && cop <= 0x03)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_and(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "and\t");

    if (cop >= 0x20 && cop <= 0x23)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x24 && cop <= 0x25)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_call(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "call\t");

    if (cop == 0xE8)
    {
        return fill_call(mz, pos, &inst, FALSE);
    }

    if (cop == 0x9A)
    {
        return fill_call(mz, pos, &inst, TRUE);
    }

    if (cop == 0xFF)
    {
        BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
        ad_cop = ad_cop >> 3;
        char buffer[50] = "";
        int size = addr_mod(mz, pos, buffer);
        if (ad_cop == 2)
        {
            strcat(inst, buffer);
        }
        else
        {
            strcat(inst, "d");
            strcat(inst, buffer);
        }
        return size;
    }

    return d_unk(mz, pos, inst);
}

DWORD d_cbw(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cbw");
    return 1;
}

DWORD d_clc(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "clc");
    return 1;
}

DWORD d_cld(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cld");
    return 1;
}

DWORD d_cli(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cli");
    return 1;
}

DWORD d_cmc(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cmc");
    return 1;
}

DWORD d_cmp(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "cmp\t");

    if (cop >= 0x3C && cop <= 0x3D)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x38 && cop <= 0x3B)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_cmpsb(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cmpsb");
    return 1;
}

DWORD d_cmpsw(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cmpsw");
    return 1;
}

DWORD d_cwd(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "cwd");
    return 1;
}

DWORD d_daa(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "daa");
    return 1;
}

DWORD d_das(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "das");
    return 1;
}

DWORD d_dec(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "dec\t");

    if (cop >= 0x48 && cop <= 0x4F)
    {
        strcat(inst, registersWord[cop - 0x48]);

        return 1;
    }

    if (cop == 0xFE || cop == 0xFF)
    {
        return fill_groupF6(mz, pos, &inst);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_div(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "div\t");

    return fill_groupF6(mz, pos, &inst);
}

DWORD d_hlt(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "hlt");
    return 1;
}

DWORD d_idiv(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "idiv\t");

    return fill_groupF6(mz, pos, &inst);
}

DWORD d_imul(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "imul\t");

    return fill_groupF6(mz, pos, &inst);
}

DWORD d_in(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "in\t");

    if (cop == 0xEC || cop == 0xED)
    {
        if (w == 1)
        {
            strcat(inst, "ax, dx");
        }
        else
        {
            strcat(inst, "al, dx");
        }
        
        return 1;
    }

    if (cop == 0xE4 || cop == 0xE5)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_inc(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "inc\t");

    if (cop >= 0x40 && cop <= 0x47)
    {
        strcat(inst, registersWord[cop - 0x40]);

        return 1;
    }

    if (cop == 0xFE || cop == 0xFF)
    {
        return fill_groupF6(mz, pos, &inst);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_int(MZHeaders *mz, DWORD pos, char *inst){
    sprintf(inst, "int    %2Xh", *(mz->code + pos + 1));
    return 2;
}

DWORD d_int3(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "int    3");
    return 1;
}

DWORD d_into(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "into");
    return 1;
}

DWORD d_iret(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "iret");
    return 1;
}

DWORD d_ja(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "ja\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jae(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jae\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jb(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jb\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jbe(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jbe\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jg(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jg\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jge(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jge\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jl(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jle\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jle(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jle\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jno(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jno\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jns(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jns\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jnz(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jnz\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jo(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jo\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jpe(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jpe\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jpo(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jpo\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_js(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "js\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jz(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jz\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jcxz(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "jcxz\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_jmp(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "jmp\t");
    char buffer[50] = "";

    if (cop == 0xEB)
    {
        itoa(mz->doshead->e_cs + IMAGE_BASE_SEG, buffer, 16);
        strcat(inst, buffer);
        strcat(inst, ":");
        char offset = *(mz->code + pos + 1);
        offset += pos + 2;
        sprintf(buffer, "%04x", offset);
        strcat(inst, buffer);
        size = 2;
    }

    if (cop == 0xE9)
    {
        itoa(mz->doshead->e_cs + IMAGE_BASE_SEG, buffer, 16);
        strcat(inst, buffer);
        strcat(inst, ":");
        short offset = *(mz->code + pos + 2);
        offset = offset << 8;
        offset += *(mz->code + pos + 1) + 3 + pos;
        sprintf(buffer, "%04x", offset);
        strcat(inst, buffer);
        size = 3;
    }

    if (cop == 0xEA)
    {
        char buffer[50] = "";
        strcat(inst, "far ptr ");
        get_offset(mz, pos + 3, FALSE, buffer);
        strcat(inst, buffer);
        strcat(inst, ":");
        get_offset(mz, pos + 5, FALSE, buffer);
        strcat(inst, buffer);
        size = 4;
    }

    if (cop == 0xFF)
    {
        BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
        ad_cop = ad_cop >> 3;
        char buffer[50] = "";
        size = addr_mod(mz, pos, buffer);
        if (ad_cop == 4)
        {
            strcat(inst, buffer);
        }
        else
        {
            strcat(inst, "d");
            strcat(inst, buffer);
        }
    }

    return size;
}

DWORD d_lahf(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "lahf");
    return 1;
}

DWORD d_lds(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    char buffer[50] = "";
    get_offset(mz, pos + 3, FALSE, buffer);
    sprintf(inst, "lds\t%s, dword ptr [%s]", registersWord[reg], buffer);
    
    return 4;
}

DWORD d_lea(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "lea\t");

    return fill_wam(mz, pos, &inst, cop, ds);
}

DWORD d_les(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    char buffer[50] = "";
    get_offset(mz, pos + 3, FALSE, buffer);
    sprintf(inst, "les\t%s, dword ptr [%s]", registersWord[reg], buffer);
    
    return 4;
}

DWORD d_lock(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "lock");
    return 1;
}

DWORD d_lodsb(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "lodsb");
    return 1;
}

DWORD d_lodsw(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "lodsw");
    return 1;
}

DWORD d_loop(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "loop\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_loopnz(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "loopnz\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_loopz(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "loopz\t");

    return fill_jmp(mz, pos, &inst);
}

DWORD d_mov(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE mod = ra & 0xC0;
    mod = mod >> 6;
    BYTE rm = ra & 0x7;
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "mov\t");

    if (cop >= 0xB0 && cop < 0xB8)
    {
        strcat(inst, registersByte[cop - 0xB0]);
        BYTE offset = *(mz->code + pos + 2);

        char buffer[50];
        get_offset(mz, pos + 1, TRUE, buffer);

        strcat(inst, ", ");
        strcat(inst, buffer);

        return 2;
    }
    
    if (cop >= 0xB8 && cop <= 0xBF)
    {
        strcat(inst, registersWord[cop - 0xB8]);
        
        char buffer[50];
        get_offset(mz, pos + 2, FALSE, buffer);
        strcat(inst, ", ");
        strcat(inst, buffer);

        return 3;
    }

    if (cop >= 0x88 && cop <= 0x8B)
    {
        char left[50] = "", right[50] = "";
        char buffer[6] = "";

        reg_mod(mz, pos, left);

        size = addr_mod(mz, pos, right);

        if (ds == 0)
        {
            swap_strings(right, left);
        }

        strcat(inst, left);
        strcat(inst, ", ");
        strcat(inst, right);
        return size;
    }

    if (cop == 0x8E || cop == 0x8C)
    {
        BYTE SR = *(mz->code + pos + 1);
        SR = SR >> 3;
        SR = SR & 3;

        PBYTE tmp = mz->code + pos;
        *(PWORD)tmp += 1;
        char right[50] = "", left[50] = "";
        size = addr_mod(mz, pos, right);
        strcpy(left, Sreg[SR]);

        if (ds == 0)
        {
            swap_strings(right, left);
        }

        strcat(inst, left);
        strcat(inst, ", ");
        strcat(inst, right);

        *(PWORD)tmp -= 1;
        return size;
    }

    if (cop >= 0xA0 && cop <= 0xA3)
    {
        char left[50] = "", right[50] = "";
        char buffer[50] = "";

        if (w == 0)
        {
            strcpy(right, "byte ptr ");
        }
        strcpy(right, "word ptr ");

        if (change_seg != -1)
        {
            strcat(right, Sreg[change_seg]);
            strcat(right, ":[");
            change_seg = -1;
        }

        if (w == 0)
        {
            strcat(left, "al");
            get_offset(mz, pos + 1, TRUE, buffer);
            size = 2;
        }
        else
        {
            strcat(left, "ax");
            get_offset(mz, pos + 2, FALSE, buffer);
            size = 3;
        }

        strcat(right, buffer);
        strcat(right, "]");

        if (ds == 1)
        {
            swap_strings(left, right);
        }

        strcat(inst, left);
        strcat(inst, ", ");
        strcat(inst, right);

        return size;
    }

    if (cop == 0xC6 || cop == 0xC7)
    {
        char left[50] = "", right[50] = "";

        size = addr_mod(mz, pos, left);

        if (w == 0)
        {
            get_offset(mz, pos + size, TRUE, right);
        }
        else
        {
            get_offset(mz, pos + size + 1, FALSE, right);
        }

        strcat(inst, left);
        strcat(inst, ", ");
        strcat(inst, right);

        return w == 0 ? size + 1 : size + 2;
    }

    return d_unk(mz, pos, inst);
}

DWORD d_movsb(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "movsb");
    return 1;
}

DWORD d_movsw(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "movsw");
    return 1;
}

DWORD d_mul(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "mul\t");

    return fill_groupF6(mz, pos, &inst);
}

DWORD d_neg(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "neg\t");

    return fill_groupF6(mz, pos, &inst);
}

DWORD d_nop(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "nop");
    return 1;
}

DWORD d_not(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "not\t");

    return fill_groupF6(mz, pos, &inst);
}

DWORD d_or(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE mod = ra & 0xC0;
    mod = mod >> 6;
    BYTE rm = ra & 0x7;
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "or\t");

    if (cop == 0x0C || cop == 0x0D)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x08 && cop <= 0x0B)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_out(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "out\t");

    if (cop == 0xEE || cop == 0xEF)
    {
        if (w == 1)
        {
            strcat(inst, "dx, ax");
        }
        else
        {
            strcat(inst, "dx, al");
        }
        
        return 1;
    }

    if (cop == 0xE6 || cop == 0xE7)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_pop(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "pop\t");

    if (cop >= 0x58 && cop <= 0x5F)
    {
        strcat(inst, registersWord[cop - 0x58]);

        return 1;
    }

    if (cop >= 0x07 && cop <= 0x1F)
    {
        int index = ((cop - 1) / 2 - 3) / 4;
        strcat(inst, Sreg[index]);

        return 1;
    }

    return d_unk(mz, pos, inst);
}

DWORD d_popf(MZHeaders *mz, DWORD pos, char *inst)
{
    strcpy(inst, "popf");
    return 1;
}

DWORD d_push(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "push\t");

    if (cop >= 0x50 && cop <= 0x57)
    {
        strcat(inst, registersWord[cop - 0x50]);

        return 1;
    }

    if (cop >= 0x06 && cop <= 0x1E)
    {
        int index = (cop / 2 - 3) / 4;
        strcat(inst, Sreg[index]);

        return 1;
    }

    if (cop == 0xFF)
    {
        return fill_groupF6(mz, pos, &inst);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_pushf(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "pushf");
    return 1;
}

DWORD d_rcl(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "rcl\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_rcr(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "rcr\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_rep(MZHeaders *mz, DWORD pos, char *inst)
{
    char buffer[50] = "";
    strcpy(inst, "rep\t");
    opcodes[*(mz->code + pos + 1)](mz, pos + 1, buffer);
    strcat(inst, buffer);

    return 2;
}

DWORD d_retn(MZHeaders *mz, DWORD pos, char *inst){
    BYTE opc = *(mz->code + pos);
    if (opc == 0xC2) {
        sprintf(inst, "retn   %u", *((PWORD)(mz->code + pos + 1)));
        return 3;
    }
    else {
        strcpy(inst, "retn");
        return 1;
    }
}

DWORD d_retf(MZHeaders *mz, DWORD pos, char *inst){
    BYTE opc = *(mz->code + pos);
    if (opc == 0xCA) {
        sprintf(inst, "retf   %u", *((PWORD)(mz->code + pos + 1)));
        return 3;
    }
    else {
        strcpy(inst, "retf");
        return 1;
    }
}

DWORD d_rol(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "rol\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_ror(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "ror\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_sahf(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "sahf");
    return 1;
}

DWORD d_sal(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "sal\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_sar(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "sar\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_sbb(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE ra = *(mz->code + pos + 1);
    BYTE reg = ra & 0x38;
    reg = reg >> 3;
    BYTE mod = ra & 0xC0;
    mod = mod >> 6;
    BYTE rm = ra & 0x7;
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "sbb\t");

    if (cop >= 0x1C && cop <= 0x1D)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x18 && cop <= 0x1B)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_scasb(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "scasb");
    return 1;
}

DWORD d_scasw(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "scasw");
    return 1;
}

DWORD d_shl(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "shl\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_shr(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    strcpy(inst, "shr\t");

    return fill_groupD0(mz, pos, &inst, cop);
}

DWORD d_stc(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "stc");
    return 1;
}

DWORD d_std(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "std");
    return 1;
}

DWORD d_sti(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "sti");
    return 1;
}

DWORD d_stosb(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "stosb");
    return 1;
}

DWORD d_stosw(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "stosw");
    return 1;
}

DWORD d_sub(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "sub\t");

    if (cop >= 0x2C && cop <= 0x2D)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x28 && cop <= 0x2B)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_test(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "test\t");

    if (cop == 0xA8 || cop == 0xA9)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop == 0x84 || cop == 0x85)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop == 0xF6 || cop == 0xF7)
    {
        char buffer[50] = "";
        DWORD size = fill_groupF6(mz, pos, &inst);
        if (w == 1)
        {
            get_offset(mz, pos + 3, FALSE, buffer);
            size += 2;
        }
        else
        {
            get_offset(mz, pos + 2, TRUE, buffer);
            size += 1;
        }
        strcat(inst, ", ");
        strcat(inst, buffer);
        return size;
    }

    return d_unk(mz, pos, inst);
}

DWORD d_wait(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "wait");
    return 1;
}

DWORD d_xchg(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE cop = *(mz->code + pos);
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "xchg\t");

    if (cop >= 0x90 && cop <= 0x97)
    {
        strcat(inst, "ax, ");
        strcat(inst, registersWord[cop - 0x90]);

        return 1;
    }

    if (cop >= 0x86 && cop <= 0x87)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_xlat(MZHeaders *mz, DWORD pos, char *inst){
    strcpy(inst, "xlat");
    return 1;
}

DWORD d_xor(MZHeaders *mz, DWORD pos, char *inst)
{
    int size = 0;
    BYTE cop = *(mz->code + pos);
    BYTE w = cop & 1;
    BYTE ds = cop & 2;
    ds = ds >> 1;
    strcpy(inst, "xor\t");

    if (cop >= 0x34 && cop <= 0x35)
    {
        return fill_woam(mz, pos, &inst, w);
    }

    if (cop >= 0x30 && cop <= 0x33)
    {
        return fill_wam(mz, pos, &inst, cop, ds);
    }

    if (cop >= 0x80 && cop <= 0x83)
    {
        return fill_group80(mz, pos, &inst, w, ds);
    }

    return d_unk(mz, pos, inst);
}

DWORD d_cs(MZHeaders *mz, DWORD pos, char *inst)
{
    change_seg = 1;
    return 1;
}

DWORD d_ds(MZHeaders *mz, DWORD pos, char *inst)
{
    change_seg = 3;
    return 1;
}

DWORD d_es(MZHeaders *mz, DWORD pos, char *inst)
{
    change_seg = 0;
    return 1;
}

DWORD d_ss(MZHeaders *mz, DWORD pos, char *inst)
{
    change_seg = 2;
    return 1;
}

DWORD d_gr80(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
    ad_cop = ad_cop >> 3;

    switch (ad_cop)
    {
        case 0:
            return d_add(mz, pos, inst);
        case 1:
            return d_or(mz, pos, inst);
        case 2:
            return d_adc(mz, pos, inst);
        case 3:
            return d_sbb(mz, pos, inst);
        case 4:
            return d_and(mz, pos, inst);
        case 5:
            return d_sub(mz, pos, inst);
        case 6:
            return d_xor(mz, pos, inst);
        case 7:
            return d_cmp(mz, pos, inst);
        default:
            return d_unk(mz, pos, inst);
    }
}

DWORD d_grd0(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
    ad_cop = ad_cop >> 3;

    switch (ad_cop)
    {
        case 0:
            return d_rol(mz, pos, inst);
        case 1:
            return d_ror(mz, pos, inst);
        case 2:
            return d_rcl(mz, pos, inst);
        case 3:
            return d_rcr(mz, pos, inst);
        case 4:
            return d_shl(mz, pos, inst);
        case 5:
            return d_shr(mz, pos, inst);
        case 6:
            return d_unk(mz, pos, inst);
        case 7:
            return d_sar(mz, pos, inst);
        default:
            return d_unk(mz, pos, inst);
    }
}

DWORD d_grf6(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
    ad_cop = ad_cop >> 3;

    switch (ad_cop)
    {
        case 0:
            return d_test(mz, pos, inst);
        case 1:
            return d_unk(mz, pos, inst);
        case 2:
            return d_not(mz, pos, inst);
        case 3:
            return d_neg(mz, pos, inst);
        case 4:
            return d_mul(mz, pos, inst);
        case 5:
            return d_imul(mz, pos, inst);
        case 6:
            return d_div(mz, pos, inst);
        case 7:
            return d_idiv(mz, pos, inst);
        default:
            return d_unk(mz, pos, inst);
    }
}

DWORD d_grfe(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
    ad_cop = ad_cop >> 3;

    switch (ad_cop)
    {
        case 0:
            return d_inc(mz, pos, inst);
        case 1:
            return d_dec(mz, pos, inst);
        case 2:
            return d_unk(mz, pos, inst);
        case 3:
            return d_unk(mz, pos, inst);
        case 4:
            return d_unk(mz, pos, inst);
        case 5:
            return d_unk(mz, pos, inst);
        case 6:
            return d_unk(mz, pos, inst);
        case 7:
            return d_unk(mz, pos, inst);
        default:
            return d_unk(mz, pos, inst);
    }
}

DWORD d_grff(MZHeaders *mz, DWORD pos, char *inst)
{
    BYTE ad_cop = *(mz->code + pos + 1) & 0x38;
    ad_cop = ad_cop >> 3;

    switch (ad_cop)
    {
        case 0:
            return d_inc(mz, pos, inst);
        case 1:
            return d_dec(mz, pos, inst);
        case 2:
            return d_call(mz, pos, inst);
        case 3:
            return d_call(mz, pos, inst);
        case 4:
            return d_jmp(mz, pos, inst);
        case 5:
            return d_jmp(mz, pos, inst);
        case 6:
            return d_push(mz, pos, inst);
        case 7:
            return d_unk(mz, pos, inst);
        default:
            return d_unk(mz, pos, inst);
    }
}

DWORD d_unk(MZHeaders *mz, DWORD pos, char *inst){ 
    strcpy(inst, "???");
    return 1;
}