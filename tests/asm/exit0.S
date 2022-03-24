BR start
DATA 16383 # sp
start
LDAC 0 # areg <- 0
LDBM 1 # breg <- sp
STAI 2 # sp[2] <- areg
LDAC 0
OPR SVC