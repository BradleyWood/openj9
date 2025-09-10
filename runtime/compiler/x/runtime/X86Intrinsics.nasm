%include "jilconsts.inc"

%macro HORIZ_SUM_XMM 3
    vpaddd      xmm%1, xmm%1, xmm%2
    vpshufd     xmm%2, xmm%1, 0x0e
    vpaddd      xmm%1, xmm%1, xmm%2
    vpshufd     xmm%2, xmm%1, 0x01
    vpaddd      xmm%1, xmm%1, xmm%2
    movd        %3, xmm%1
%endmacro

%macro HORIZ_SUM_YMM 3
    vextractf128 xmm%1, ymm%2, 0xff
    HORIZ_SUM_XMM %1, %2, %3
%endmacro

%macro HORIZ_SUM_ZMM 3
    vextractf64x4 ymm%1, zmm%2, 0xff
    YMM_ADD_2_VECTORS %1, %2
    HORIZ_SUM_YMM %1, %2, %3
%endmacro

%macro YMM_ADD_2_VECTORS 2
    vpaddd      ymm%1, ymm%2
%endmacro

%macro YMM_ADD_4_VECTORS 4
    vpaddd      ymm%1, ymm%2
    vpaddd      ymm%1, ymm%3
    vpaddd      ymm%1, ymm%4
%endmacro

%macro ZMM_ADD_2_VECTORS 2
    vpaddd      zmm%1, zmm%2
%endmacro

%macro ZMM_ADD_4_VECTORS 4
    vpaddd      zmm%1, zmm%2
    vpaddd      zmm%1, zmm%3
    vpaddd      zmm%1, zmm%4
%endmacro

; %1 = scratch vector
; %2 = vector 1
; %3 = vector 2
; %4 = vector 3
; %5 = vector 4
; %6 = GPR result

%macro ZMM_ADD_X4 6
    ZMM_ADD_4_VECTORS %2, %3, %4, %5
    HORIZ_SUM_ZMM %1, %2, %6
%endmacro

; %1 = scratch vector
; %2 = vector 1
; %3 = vector 2
; %4 = GPR result
%macro ZMM_ADD_X2 4
    ZMM_ADD_2_VECTORS %2, %3
    HORIZ_SUM_ZMM %1, %2, %4
%endmacro

; %1 = scratch vector
; %2 = vector 1
; %3 = vector 2
; %4 = vector 3
; %5 = vector 4
; %6 = GPR result

%macro YMM_ADD_X4 6
    YMM_ADD_4_VECTORS %2, %3, %4, %5
    HORIZ_SUM_YMM %1, %2, %6
%endmacro

; %1 = scratch vector
; %2 = vector 1
; %3 = vector 2
; %4 = GPR result
%macro YMM_ADD_X2 4
    YMM_ADD_2_VECTORS %2, %3
    HORIZ_SUM_YMM %1, %2, %4
%endmacro

; %1 tmp vector
; %2 result vector
; %3 Powers of 31 vector
; %4 32^n vector
; %5 offset to load from;
%macro LOOP_ITERATION_COMPRESSED 5
        vpmovzxbd	%1, [rdi+1*rcx+%5]
        vpmulld	    %1, %4
        vpmulld	    %2, %3
        vpaddd	    %2, %1
%endmacro

; %1 tmp vector
; %2 result vector
; %3 Powers of 31 vector
; %4 32^n vector
; %5 offset to load from;
%macro LOOP_ITERATION_DECOMPRESSED 5
        vpmovzxwd	%1, [rdi+2*rcx+%5]
        vpmulld	    %1, %4
        vpmulld	    %2, %3
        vpaddd	    %2, %1
%endmacro

HASH_31_POW_64 equ 1304393729
HASH_31_POW_32 equ 2111290369
HASH_31_POW_16 equ 1353309697
HASH_31_POW_8  equ -1807454463

segment .data

HASH_CONSTS_64:
             dd 2120287199, -208698303, -1807847521, -1166696319, 100911967, 280349889, -1930619105, 630458625,
             dd 20337375, 693392705, 438009503, -1925533311, 769170015, 1133190593, -240540129, -7759359,
             dd 969581023, 1970939457, -1183347297, -1700740479, -1024693921, -448696639, 124073247, -1935660287,
             dd 1461579999, -922683583, 1632803999, 329765761, 1950300255, 1725480897, 1025491999, 2111290369
HASH_CONSTS_32:
             dd -2010103841, 350799937, 11316127, 693101697, -254736545, 961614017, 31019807, -2077209343,
             dd -67006753, 1244764481, -2038056289, 211350913, -408824225, -844471871, -997072353, 1353309697
HASH_CONSTS_16:
             dd -510534177, 1507551809, -505558625, -293403007, 129082719, -1796951359, -196513505, -1807454463
HASH_CONSTS_8:
             dd 1742810335, 887503681, 28629151, 923521
HASH_CONSTS_4:
             dd 29791, 961, 31, 1

segment .text

DECLARE_EXTERN strHashCodeDecompressed512Helper_impl
DECLARE_EXTERN strHashCodeCompressed512Helper_impl

; String hashcode for unsigned chars (decompressed) - main avx-512 loop unrolled 4x
;
align 16
strHashCodeDecompressed512Helper_impl:
; Parameters
;
; rdi -> ptr
; esi -> init_hash
; edx -> termination_index
; ecx -> start_index

; Return
; eax -> hash_value
; todo
        ;mov eax, 69
        ;int3
        ;ret

        movd	    xmm6, esi
        vpxord	    xmm7, xmm7
        vpxord	    xmm8, xmm8
        vpxord	    xmm9, xmm9

        ; LOAD 31^n...
        mov eax, HASH_31_POW_32
        vpbroadcastd ymm1, eax

        mov eax, edx
        and eax, 0xffffffe0

        vmovdqu32	    ymm2, [_rel HASH_CONSTS_32]
        vmovdqu32	    ymm3, [_rel HASH_CONSTS_32 + 32]
        vmovdqu32	    ymm4, [_rel HASH_CONSTS_32 + 2*32]
        vmovdqu32	    ymm5, [_rel HASH_CONSTS_32 + 3*32]

loop_256_x4_decompressed:
        ; unroll itr 1
        LOOP_ITERATION_DECOMPRESSED ymm0, ymm6, ymm1, ymm2, 0
        LOOP_ITERATION_DECOMPRESSED ymm0, ymm7, ymm1, ymm3, 0x10
        LOOP_ITERATION_DECOMPRESSED ymm0, ymm8, ymm1, ymm4, 0x20
        LOOP_ITERATION_DECOMPRESSED ymm0, ymm9, ymm1, ymm5, 0x30

        ; loop back
        add	        ecx, 0x00000020
        cmp	        ecx, eax
        jl	        loop_256_x4_decompressed

end_loop_decompressed:
        ; Compute total sum of zmm6, zmm7, zmm8, and zmm9, store result into eax. zmm0 is a scratch reg.
        YMM_ADD_X4 0, 6, 7, 8, 9, eax

;        movd xmm6, eax
;        mov esi, HASH_31_POW_16
;        vpbroadcastd ymm1, esi
;
;        mov esi, edx
;        and esi, 0xfffffff0
;        cmp	        ecx, esi
;        jge	        done_label
;
;        vmovdqu32	    ymm4, [_rel HASH_CONSTS_16]
;        vmovdqu32	    ymm5, [_rel HASH_CONSTS_16 + 32]
;loop_256_x2_compressed:
;        ; unroll itr 1
;        LOOP_ITERATION_COMPRESSED ymm0, ymm6, ymm1, ymm4, 0x0
;        LOOP_ITERATION_COMPRESSED ymm0, ymm7, ymm1, ymm5, 0x8
;
;        ; loop back
;        add	        ecx, 0x00000010
;        cmp	        ecx, esi
;        jl	        loop_256_x2_compressed
;
;        YMM_ADD_X2 0, 6, 7, eax

de_done_label:
        ret

; String hashcode for unsigned bytes (compressed) - main avx-512 loop unrolled 4x
;
align 16
strHashCodeCompressed512Helper_impl:
; Parameters
;
; rdi -> ptr
; esi -> init_hash
; edx -> termination_index
; ecx -> start_index
        movd	    xmm6, esi
        vpxord	    xmm7, xmm7
        vpxord	    xmm8, xmm8
        vpxord	    xmm9, xmm9

        ; LOAD 31^n...
        mov eax, HASH_31_POW_32
        vpbroadcastd ymm1, eax

        mov eax, edx
        and eax, 0xffffffe0

        vmovdqu32	    ymm2, [_rel HASH_CONSTS_32]
        vmovdqu32	    ymm3, [_rel HASH_CONSTS_32 + 32]
        vmovdqu32	    ymm4, [_rel HASH_CONSTS_32 + 2*32]
        vmovdqu32	    ymm5, [_rel HASH_CONSTS_32 + 3*32]

loop_256_x4_compressed:
        ; unroll itr 1
        LOOP_ITERATION_COMPRESSED ymm0, ymm6, ymm1, ymm2, 0
        LOOP_ITERATION_COMPRESSED ymm0, ymm7, ymm1, ymm3, 0x8
        LOOP_ITERATION_COMPRESSED ymm0, ymm8, ymm1, ymm4, 0x10
        LOOP_ITERATION_COMPRESSED ymm0, ymm9, ymm1, ymm5, 0x18

        ; loop back
        add	        ecx, 0x00000020
        cmp	        ecx, eax
        jl	        loop_256_x4_compressed

end_loop_compressed:
        ; Compute total sum of zmm6, zmm7, zmm8, and zmm9, store result into eax. zmm0 is a scratch reg.
        YMM_ADD_X4 0, 6, 7, 8, 9, eax

;        movd xmm6, eax
;        mov esi, HASH_31_POW_16
;        vpbroadcastd ymm1, esi
;
;        mov esi, edx
;        and esi, 0xfffffff0
;        cmp	        ecx, esi
;        jge	        done_label
;
;        vmovdqu32	    ymm4, [_rel HASH_CONSTS_16]
;        vmovdqu32	    ymm5, [_rel HASH_CONSTS_16 + 32]
;loop_256_x2_compressed:
;        ; unroll itr 1
;        LOOP_ITERATION_COMPRESSED ymm0, ymm6, ymm1, ymm4, 0x0
;        LOOP_ITERATION_COMPRESSED ymm0, ymm7, ymm1, ymm5, 0x8
;
;        ; loop back
;        add	        ecx, 0x00000010
;        cmp	        ecx, esi
;        jl	        loop_256_x2_compressed
;
;        YMM_ADD_X2 0, 6, 7, eax

done_label:
        ret
