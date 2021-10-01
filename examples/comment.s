











$200:   call $220
        bge $1 $201
        exit $0


$201:   getb $400 $0
        beq $3b $202
        write $1 $400 $1
        getb $400 $0
        beq $22 $210
        jmp $200


$202:   call $220
        bge $1 $203
        exit $0


$203:   getb $400 $0
        bne $0a $202
        write $1 $400 $1
        jmp $200





$210:   call $220
        bge $1 $211
        exit $1


$211:   write $1 $400 $1
        getb $400 $0
        bne $22 $210
        jmp $200



$220:   read $0 $400 $1
        ret


$400:  resb $1
