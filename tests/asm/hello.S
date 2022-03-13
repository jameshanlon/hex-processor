BR start

DATA 16383 # sp

string
DATA 104 # h
DATA 101 # e
DATA 108 # l
DATA 108 # l
DATA 111 # o
DATA 10  # \n

start
LDBM 1 # breg <- sp
# h
LDAC string # areg = string
LDAI 0 # areg <- 'h'
STAI 2 # sp[2] <- areg
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg
LDAC 1 # areg <- 1 (write)
OPR SVC
# e
LDAC string # areg = string
LDAI 1 # areg <- 'e'
STAI 2 # sp[2] <- areg
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg
LDAC 1 # areg <- 1 (write)
OPR SVC
# l
LDAC string # areg = string
LDAI 2 # areg <- 'l'
STAI 2 # sp[2] <- areg
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg
LDAC 1 # areg <- 1 (write)
OPR SVC
# l
LDAC string # areg = string
LDAI 3 # areg <- 'l'
STAI 2 # sp[2] <- areg
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg
LDAC 1 # areg <- 1 (write)
OPR SVC
# o
LDAC string # areg = string
LDAI 4 # areg <- 'o'
STAI 2 # sp[2] <- areg
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg
LDAC 1 # areg <- 1 (write)
OPR SVC
# \n
LDAC string # areg = string
LDAI 5 # areg <- '\n'
STAI 2 # sp[2] <- areg
LDAC 0 # areg <- 0
STAI 3 # sp[3] <- areg
LDAC 1 # areg <- 1 (write)
OPR SVC
BR end

end
LDAC 0
OPR SVC
