; file_artifact.asm
; A03_5 in-memory payload artifact writer
; NASM x64 raw shellcode-style callable payload
;
; Expected function signature:
;   void Run(PARAMS *pParams);
;
; Windows x64 calling convention:
;   RCX = first argument
;
; This payload writes:
;   C:\Users\Public\A03_5_IN_MEMORY_PAYLOAD_EXECUTED.txt

bits 64
default rel

section .text
global _start

_start:
    ; RCX = PARAMS*
    push rbp
    mov rbp, rsp

    ; preserve nonvolatile registers
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    ; allocate local stack space
    ; includes shadow space + buffers
    sub rsp, 2048

    ; keep PARAMS* in RBX
    mov rbx, rcx

    ; ------------------------------------------------------------
    ; PARAMS layout offsets, 8 bytes each
    ; ------------------------------------------------------------
    ; +0x00 pBaseAddress
    ; +0x08 pCreateFileA
    ; +0x10 pWriteFile
    ; +0x18 pCloseHandle
    ; +0x20 pGetCurrentProcessId
    ; +0x28 pGetModuleFileNameA
    ; +0x30 pGetLocalTime
    ; +0x38 pWsprintfA
    ; +0x40 pOutputDebugStringA

    ; local buffers
    ; rsp + 0x100 = SYSTEMTIME buffer
    ; rsp + 0x140 = process path buffer
    ; rsp + 0x340 = output contents buffer
    ; rsp + 0x740 = bytesWritten DWORD

    ; ------------------------------------------------------------
    ; GetCurrentProcessId()
    ; ------------------------------------------------------------
    call qword [rbx + 0x20]
    mov r12d, eax                    ; PID

    ; ------------------------------------------------------------
    ; GetLocalTime(&SYSTEMTIME)
    ; ------------------------------------------------------------
    lea rcx, [rsp + 0x100]
    call qword [rbx + 0x30]

    ; ------------------------------------------------------------
    ; GetModuleFileNameA(NULL, pathBuffer, 260)
    ; ------------------------------------------------------------
    xor rcx, rcx
    lea rdx, [rsp + 0x140]
    mov r8d, 260
    call qword [rbx + 0x28]

    ; ------------------------------------------------------------
    ; wsprintfA(contentsBuffer, format, timestamp fields, PID, path)
    ;
    ; SYSTEMTIME layout:
    ; WORD wYear         +0
    ; WORD wMonth        +2
    ; WORD wDayOfWeek    +4
    ; WORD wDay          +6
    ; WORD wHour         +8
    ; WORD wMinute       +10
    ; WORD wSecond       +12
    ; WORD wMilliseconds +14
    ; ------------------------------------------------------------

    lea rcx, [rsp + 0x340]           ; output buffer
    lea rdx, [rel fmt_contents]      ; format string

    movzx r8d, word [rsp + 0x100]    ; year
    movzx r9d, word [rsp + 0x102]    ; month

    ; stack arguments after first 4 args
    movzx eax, word [rsp + 0x106]    ; day
    mov [rsp + 0x20], rax

    movzx eax, word [rsp + 0x108]    ; hour
    mov [rsp + 0x28], rax

    movzx eax, word [rsp + 0x10A]    ; minute
    mov [rsp + 0x30], rax

    movzx eax, word [rsp + 0x10C]    ; second
    mov [rsp + 0x38], rax

    mov eax, r12d                    ; PID
    mov [rsp + 0x40], rax

    lea rax, [rsp + 0x140]           ; process path
    mov [rsp + 0x48], rax

    call qword [rbx + 0x38]          ; wsprintfA

    ; EAX = number of chars written
    mov r13d, eax                    ; content length

    ; ------------------------------------------------------------
    ; CreateFileA(
    ;   outPath,
    ;   GENERIC_WRITE,
    ;   FILE_SHARE_READ,
    ;   NULL,
    ;   CREATE_ALWAYS,
    ;   FILE_ATTRIBUTE_NORMAL,
    ;   NULL
    ; )
    ; ------------------------------------------------------------

    lea rcx, [rel out_path]
    mov edx, 0x40000000              ; GENERIC_WRITE
    mov r8d, 0x00000001              ; FILE_SHARE_READ
    xor r9d, r9d                     ; NULL security attrs

    mov qword [rsp + 0x20], 2        ; CREATE_ALWAYS
    mov qword [rsp + 0x28], 0x80     ; FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 0x30], 0        ; template file NULL

    call qword [rbx + 0x08]          ; CreateFileA

    mov r14, rax                     ; file handle

    ; INVALID_HANDLE_VALUE = -1
    cmp r14, -1
    je .debug_only

    ; ------------------------------------------------------------
    ; WriteFile(hFile, contents, length, &bytesWritten, NULL)
    ; ------------------------------------------------------------

    mov rcx, r14
    lea rdx, [rsp + 0x340]
    mov r8d, r13d
    lea r9, [rsp + 0x740]
    mov qword [rsp + 0x20], 0

    call qword [rbx + 0x10]          ; WriteFile

    ; ------------------------------------------------------------
    ; CloseHandle(hFile)
    ; ------------------------------------------------------------

    mov rcx, r14
    call qword [rbx + 0x18]

.debug_only:
    ; ------------------------------------------------------------
    ; OutputDebugStringA(marker)
    ; ------------------------------------------------------------

    lea rcx, [rel debug_msg]
    call qword [rbx + 0x40]

.done:
    add rsp, 2048

    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx

    pop rbp
    ret

section .data

out_path:
    db "C:\Users\Public\A03_5_IN_MEMORY_PAYLOAD_EXECUTED.txt", 0

debug_msg:
    db "THESIS_A03_5_NETWORK_TO_MEMORY_PAYLOAD_EXECUTED", 13, 10, 0

fmt_contents:
    db "THESIS_A03_5_NETWORK_TO_MEMORY_PAYLOAD_EXECUTED", 13, 10
    db "Timestamp: %04d-%02d-%02d %02d:%02d:%02d", 13, 10
    db "PID: %lu", 13, 10
    db "ProcessPath: %s", 13, 10
    db "ExecutionMode: downloaded_to_memory_and_called_as_raw_payload", 13, 10
    db "PayloadStagedToDisk: false", 13, 10
    db 0