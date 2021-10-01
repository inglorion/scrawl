
























$200:   call $300
        bge $1 $201
        call $220
        exit $0



$201:   getb $400 $0
        blt $61 $210
        
        
        getw $405 $0
        bge $20 $210
        
        
        getb $400 $0
        setb $4fc @$405
        getw $405 $0
        add $1
        setw $405 $0
        jmp $200


$210:   getw $405 $0
        beq $0 $21f
        bne $3 $250
        locb $4f1 $4fc $0
        locb $4f2 $3f1 $0
        getw $405 $0
        setw $4f3 $0
        call $308
        bne $0 $250
        jmp $230




$21e:   call $220
$21f:   write $1 $400 $1
        jmp $200



$220:   getw $405 $0
        blt $1 $221
        write $1 $4fc @$405
        getw $3f2 $0
        setw $405 $0
$221:   ret



$230:   getb $400 $0
        bne $20 $232
        call $300
        beq $1 $230
        exit $1




$232:   getw $403 $0
        setw $4fe @$404
        getw $405 $0
        xor @$405
        setw $405 $0
$233:   getb $400 $0
        blt $61 $234
        setb $4ff @$403
        getw $403 $0
        add $1
        setw $403 $0
        getw $405 $0
        add $1
        setw $405 $0
        call $300
        beq $1 $233
        exit $1


$234:   getb $400 $0
        bne $20 $236
        call $300
        beq $1 $234
        exit $1


$236:   getw $405 $0
        setb $4fd @$404
        getw $404 $0
        add $1
        setw $404 $0



$237:   getb $400 $0
        beq $0a $238
        setb $4ff @$403
        getw $403 $0
        add $1
        setw $403 $0
        call $300
        beq $1 $237
        beq $0 $238
        exit $1


$238:   getw $3f2 $0
        setb $4ff @$403
        getw $403 $0
        add $1
        setw $403 $0
        getw $3f2 $0
        setw $405 $0
        jmp $200




$250:   getw $3f2 $0
        setw $4f7 $0
$251:   getw $4f7 $0
        bge @$404 $21e
        
        getb $4fd @$4f7
        bne @$405 $258
        
        setw $4f3 $0
        locb $4f1 $4fc $0
        getw $4fe @$4f7
        setw $4f0 $0
        locb $4f2 $4ff @$4f0
        call $308
        bne $0 $258

        
        getw $3f2 $0
        setw $405 $0
        getb $4fd @$4f7
        setw $4f0 $0
        getw $4fe @$4f7
        add @$4f0
        setw $4f5 $0
$252:   getb $4ff @$4f5
        beq $0 $258
        setb $4fc $0
        getw $4f5 $0
        add $1
        setw $4f5 $0
        write $1 $4fc $1
        jmp $252
        
$258:   getw $4f7 $0
        add $1
        setw $4f7 $0
        jmp $251



$300:   read $0 $400 $1
        ret







$308:   getw $3f2 $0
        setw $4f4 $0

$309:   getw $4f4 $0
        blt @$4f3 $30b
        getw $3f2 $0
$30a:   ret

$30b:   getb $4f2 @$4f4
        setw $4f0 $0
        getb $4f1 @$4f4
        sub @$4f0
        bne $0 $30a
        getw $4f4 $0
        add $1
        setw $4f4 $0
        jmp $309

$3f0:   byte $3
$3f1:   byte $64 $65 $66
$3f2:   word $0


$400:   resb $1


$403:   word $0


$404:   word $0


$405:   word $0


$4f0:   resw $1


$4f1:   resw $1


$4f2:   resw $1


$4f3:   resw $1


$4f4:   resw $1


$4f5:   resw $1


$4f6:   resw $1


$4f7:   resw $1


$4f8:   resw $1


$4fc:   resb $20


$4fd:   resb $400


$4fe:   resw $400






$4ff:   resb $4000
