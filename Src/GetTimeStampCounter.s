section .text

global GetTimeStampCounter

GetTimeStampCounter:
    rdtsc
    shl rdx, 32
    add rax, rdx
    ret
    