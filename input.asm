MOVC R2,#4
MOVC R4,#5
STOREP R4,R2,#8
STOREP R4,R2,#8
LOADP R0,R4,#7
LOADP R4,R2,#4
EX-OR R2,R0,R4
HALT