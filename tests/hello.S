BR start

DATA 65536 # sp

string
DATA 104 # h
DATA 101 # e
DATA 108 # l
DATA 108 # l
DATA 111 # o
DATA 10  # \n

# Procedure calling:
#  LDAP return address (label following next BR)
#  BR <labeL> to procedure entry point
#
#    Procedure entry stores the return address
#    ...
#    Exit loads return address into breg
#    BRB to branch back

# Frame 0: sp[0] return address
#          sp[1] arg0
#          sp[2] arg1
#          sp[3] arg2
#          sp[4] temp0
#          sp[4] temp1
# Frame 1: sp[5] return address
#          ...

putchar
## Entry
LDAM 1 # breg <- sp
STAI 0 # sp[0] <- areg
#LDBC 3 # breg <- 3
#OPR SUB # areg <- areg - breg (3)
#STAM 1 # Store sp
## Body
#LDBM 1 # areg <- sp
#LDBI 1 # breg <- sp[1] # Load char
#OPR ADD # areg <- breg
#LDBM 1 # breg <- sp
#STAI 2 # sp[2] <- areg
#LDAC 0 # areg <- 0
#STAI 3 # sp[3] <- areg
#LDAC 1 # areg <- 1 (write)
#OPR SVC
## Exit
#LDBM 1 # breg <- sp
#LDAC 3 # reg <- 3
#OPR ADD # areg <- areg + breg (3)
#STAM 1 # Store sp
LDAM 1 # areg <- sp
LDBI 0 # breg <- return address
OPR BRB

start
LDBM 1 # breg <- sp
# h
LDAP string # areg = string
LDAI 0 # areg <- 'h'
STAI 2 # sp[1] <- areg
LDAP return1
BR putchar
return1
# e
# l
# l
# o
# \n
BR end

end
LDAC 0
OPR SVC
