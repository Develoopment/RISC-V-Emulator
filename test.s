_start:
    addi t0, zero, 0     # t0 = sum = 0
    addi t1, zero, 1     # t1 = i = 1
    addi t2, zero, 6     # t2 = 6  (sentinel: stop when i reaches 6)

loop:
    add  t0, t0, t1      # sum += i
    addi t1, t1, 1       # i++
    bne  t1, t2, loop    # if i != 6, repeat

    addi t3, zero, 0xFF  # t3 = 255 (bitmask), not much of a functional purpose but to demonstrate that ANDing works
    and  t0, t0, t3      # mask result to lower 8 bits (sum = 15, unchanged)

    sd   t0, 0(sp)       # store result onto the stack
    ld   t4, 0(sp)       # load it back into t4 no functional reason apart from demoing that LOADing works

end:
  beq zero, zero, end #since I haven't programmed ecall yet to handle the RISC-V equivalent of a HALT