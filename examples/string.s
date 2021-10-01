




$200:   call $220
        bge $1 $201
        exit $0


$201:   getb $400 $0
        beq $22 $202
        write $1 $400 $1
        jmp $200


$202:   call $220
        bge $1 $203
        exit $0


$203:   getb $400 $0
        beq $22 $200
        
        write $1 $3ff $1
        
        getb $400 $0
        bsr $4
        call $210
        getb $400 $0
        call $210
        
        write $1 $3fe $1
        jmp $202


$210:   and $f
        or $30
        blt $3a $211
        add $27

$211:   setb $401 $0
        write $1 $401 $1
        ret



$220:   read $0 $400 $1
        ret

$3fe:   byte $20

$3ff:   byte $24


$400:   resb $1


$401:   resb $1


$403:   resw $1
