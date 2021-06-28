%macro gensys 2
        global sys_%2:function
sys_%2:
        push    r10
        mov     r10, rcx
        mov     rax, %1
        syscall
        pop     r10
        ret
%endmacro

extern  errno
        section .data

        section .text

        gensys   0, read
        gensys   1, write
        gensys   2, open
        gensys   3, close
        gensys   9, mmap
        gensys  10, mprotect
        gensys  11, munmap
        gensys  22, pipe
        gensys  32, dup
        gensys  33, dup2
        gensys  34, pause
        gensys  35, nanosleep
        gensys  57, fork
        gensys  60, exit
        gensys  79, getcwd
        gensys  80, chdir
        gensys  82, rename
        gensys  83, mkdir
        gensys  84, rmdir
        gensys  85, creat
        gensys  86, link
        gensys  88, unlink
        gensys  89, readlink
        gensys  90, chmod
        gensys  92, chown
        gensys  95, umask
        gensys  96, gettimeofday
        gensys 102, getuid
        gensys 104, getgid
        gensys 105, setuid
        gensys 106, setgid
        gensys 107, geteuid
        gensys 108, getegid

        gensys  13, rt_sigaction
        gensys  14, rt_sigprocmask
        gensys  37, alarm
        gensys 127, rt_sigpending

        global open:function
open:
        call    sys_open
        cmp     rax, 0
        jge     open_success    ; no error :)
open_error:
        neg     rax
%ifdef NASM
        mov     rdi, [rel errno wrt ..gotpc]
%else
        mov     rdi, [rel errno wrt ..gotpcrel]
%endif
        mov     [rdi], rax      ; errno = -rax
        mov     rax, -1
        jmp     open_quit
open_success:
%ifdef NASM
        mov     rdi, [rel errno wrt ..gotpc]
%else
        mov     rdi, [rel errno wrt ..gotpcrel]
%endif
        mov     QWORD [rdi], 0  ; errno = 0
open_quit:
        ret


        global setjmp:function
setjmp:
        pop rsi                 ;return address, and adjust the stack
        mov [rdi], rbx
        mov [rdi+8], rsp
        push rsi
        mov [rdi+16], rbp
        mov [rdi+24], r12
        mov [rdi+32], r13
        mov [rdi+40], r14
        mov [rdi+48], r15
        mov [rdi+56], rsi
        push rdi

        mov rax, 14             ;system call number
        mov rdi, 0              ;how
        mov rsi, 0              ;nset
        mov rdx, 0              ;oset
        mov r10, 32             ;sigset_t size
        syscall                 ;call sigprocmask 

        pop rdi
        mov [rdi+64], rdx       ;save sigmask
        mov eax, 0              ;return value

        ret

        global longjmp:function
longjmp:
        mov rbx, [rdi]
        mov rsp, [rdi+8]
        mov rbp, [rdi+16]
        mov r12, [rdi+24]
        mov r13, [rdi+32]
        mov r14, [rdi+40]
        mov r15, [rdi+48]

        push rdi
        push rsi
       
        mov rax, 14             ;system call number
        mov rsi, [rdi+64]       ;nset (get jmp_buf.mask)
        mov rdi, 1              ;how (block)
        mov rdx, 0              ;oset
        mov r10, 32             ;sigset_t size
        syscall                 ;call sigprocmask

        pop rax                 ;return value
        pop rdi
        jmp [rdi+56]

        ret

        global sys_sigreturn:function
sys_sigreturn:
        mov  rax, 15
        syscall
        ret


