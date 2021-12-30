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
#  BR <label> to procedure entry point
#
#    Procedure entry stores the return address
#    ...
#    Exit loads return address into breg
#    BRB to branch back

# Frame 0: sp[0] return address (written by callee)
#          sp[1] arg0
#          sp[2] arg1
#          sp[3] arg2
#          sp[4] temp0
#          sp[4] temp1
# Frame 1: sp[5] return address (written by callee)
#          ...

putchar
# Entry
# Save return address
LDBM 1 # breg <- sp
STAI 0 # sp[0] <- areg
# Adjust stack pointer
LDBC 16 # breg <- 16 (4 stack words)
LDAM 1 # areg <- sp
OPR SUB # areg <- sp - breg (3)
STAM 1 # Store sp
## Body: print the character using SVC
LDBM 1 # breg <- sp
LDAI 5 # areg <- sp[5] # Load char argument from caller's frame
STAI 2 # sp[2] <- areg (character argument)
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg (stream number 0)
LDAC 1 # areg <- 1 (syscall write)
OPR SVC
# Exit
# Adjust stack pointer
LDAM 1 # areg <- sp
LDBC 16 # breg <- 16 (4 stack words)
OPR ADD # areg <- sp + breg (3)
STAM 1 # Store sp
# Restore return address
LDBM 1 # breg <- sp
LDBI 0 # breg <- sp[0] return address
OPR BRB

start
# h
LDBM 1 # breg <- sp
LDAP string # areg = string
LDAI 0 # areg <- 'h'
STAI 1 # sp[1] <- areg ('h')
LDAP returnh
BR putchar
returnh
# e
LDBM 1 # breg <- sp
LDAP string # areg = string
LDAI 1 # areg <- 'e'
STAI 1 # sp[1] <- areg ('e')
LDAP returne
BR putchar
returne
# l
LDBM 1 # breg <- sp
LDAP string # areg = string
LDAI 2 # areg <- 'l'
STAI 1 # sp[1] <- areg ('l')
LDAP returnl1
BR putchar
returnl1
# l
LDBM 1 # breg <- sp
LDAP string # areg = string
LDAI 3 # areg <- 'l'
STAI 1 # sp[1] <- areg ('l')
LDAP returnl2
BR putchar
returnl2
# o
LDBM 1 # breg <- sp
LDAP string # areg = string
LDAI 4 # areg <- 'o'
STAI 1 # sp[1] <- areg ('o')
LDAP returno
BR putchar
returno
# \n
LDBM 1 # breg <- sp
LDAP string # areg = string
LDAI 5 # areg <- '\n'
STAI 1 # sp[1] <- areg ('\n')
LDAP returnnl
BR putchar
returnnl
BR end

end
LDAC 0
OPR SVC
