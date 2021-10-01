$200:   argc
        blt @$400 $2ff
        beq @$400 $2ff
        
$208:   argb @$400 @$401
        blt $0 $210
        setb $4ff $0
        write $1 $4ff $1
        getw $401 $0
        add $1
        setw $401 $0
        jmp $208

$210:   getw $402 $0
        setw $401 $0
        getw $400 $0
        add $1
        setw $400 $0
        jmp $200

$2ff:   exit $0


$400:   word $0


$401:   word $0


$402:   word $0


$4ff:   resb $1
