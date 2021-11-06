/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/****************************************************************************************
 *                                                                                      *
 *  ----------------------------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |    6    |    7    |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0x0   |   0x4   |   0x8   |   0xc   |   0x10   |   0x14  |   0x18  |   0x1c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|        R12        |         R13        |        R14        |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |    8    |    9    |   10    |   11    |    12    |    13   |    14   |    15   |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0x20  |   0x24  |   0x28  |  0x2c   |   0x30   |   0x34  |   0x38  |   0x3c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        R15        |        RBX        |         RBP        |        RIP        |  *
 *  ----------------------------------------------------------------------------------  *
 *                                                                                      *
 ****************************************************************************************/

/* 这个结构体怎么布局的不重要 只需要创建协程函数和转向协程的函数 使用相同的编解码方式即可 结构体只是中转作用*/
/* 参考日常的函数调用过程，重要的是函数的入口地址 和 函数的返回地址。入口地址即为传过来的函数 返回地址使用的finish函数直接退出 */

.file "make_x86_64_sysv_elf_gas.S"
.text
.globl make_fcontext
.type make_fcontext,@function
.align 16
make_fcontext:
    /* first arg of make_fcontext() == top of context-stack */
    /* %rdi为stack的栈底 即最高地址 保存到rax寄存器中 */
    movq  %rdi, %rax

    /* shift address in RAX to lower 16 byte boundary */
    /* 地址对齐 */
    andq  $-16, %rax

    /* reserve space for context-data on context-stack */
    /* on context-function entry: (RSP -0x8) % 16 == 0 */
    /* 将rax中的地址 也就是栈底 向下移动了64字节，保留了context的地址 */
    leaq  -0x40(%rax), %rax

    /* third arg of make_fcontext() == address of context-function */
    /* stored in RBX */
    /* 将第三个参数也就是context-function的地址 保存到  0x28(%rax) 也就是上面图中rbx中（注意栈是从高到低 向下生长） */
    movq  %rdx, 0x28(%rax)

    /* save MMX control- and status-word */
    /* 保存到 fc_mxcsr */
    stmxcsr  (%rax)
    /* save x87 control-word */
    /* 保存到 fc_x87_cw */
    fnstcw   0x4(%rax)

    /* compute abs address of label trampoline */
    leaq  trampoline(%rip), %rcx
    /* save address of trampoline as return-address for context-function */
    /* will be entered after calling jump_fcontext() first time */
    /* 保存context的入口地址到 0x38(%rax) 即RIP中 */
    movq  %rcx, 0x38(%rax)

    /* compute abs address of label finish */
    leaq  finish(%rip), %rcx
    /* save address of finish as return-address for context-function */
    /* will be entered after context-function returns */
    /* 保存返回地址到  0x30(%rax) 即 RBP中*/
    movq  %rcx, 0x30(%rax)

    ret /* return pointer to context-data */

trampoline:
    /* store return address on stack */
    /* fix stack alignment */
    push %rbp
    /* jump to context-function */
    jmp *%rbx

finish:
    /* exit code is zero */
    xorq  %rdi, %rdi
    /* exit application */
    call  _exit@PLT
    hlt
.size make_fcontext,.-make_fcontext

/* Mark that we don't need executable stack. */
.section .note.GNU-stack,"",%progbits
