BR start

DATA 16383 # sp

start
LDAC 255 # areg <- 255
LDBM 1 # breg <- sp
STAI 2 # sp[2] <- areg
LDAC 0
OPR SVC