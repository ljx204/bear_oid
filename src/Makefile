OID_INCLUDE = ./include
#LIB_MAD = ./libmad
#LIB_TAG = ./libtag
#LIB_ZLIB = ./zlib
OID_SRC = ./src
OID_OBJ = ./obj

OID_APP_SRC = ./APP
OID_APP_OBJ = ./APP_OBJ

SRC1 = $(wildcard ${OID_SRC}/*.c)
SRC10 = $(wildcard ${OID_APP_SRC}/*.c)

#SRC2 = $(wildcard ${LIB_MAD}/*.c)
#SRC3 = $(wildcard ${LIB_TAG}/*.c)
#SRC4 = $(wildcard ${LIB_ZLIB}/*.c)


OBJ = $(patsubst %.c, ${OID_OBJ}/%.o, $(notdir $(SRC1)))
#OBJ_MAD = $(patsubst %.c, ${LIB_MAD}/%.o, $(notdir $(SRC2)))
#OBJ_TAG = $(patsubst %.c, ${LIB_TAG}/%.o, $(notdir $(SRC3)))
#OBJ_ZLIB = $(patsubst %.c, ${LIB_ZLIB}/%.o, $(notdir $(SRC4)))

APP_OBJ = $(patsubst %.c, ${OID_APP_OBJ}/%.o, $(notdir $(SRC10)))

TARGET = bear

TARGET1 = app_suer

#CC = gcc

LFLAGS = -lpthread -lasound -lmad -lid3tag -lz -lmp3lame

CFLAGS = -I${OID_INCLUDE} 


${TARGET}:${OBJ}  $(APP_OBJ)
	$(CC) $(OBJ)  $(LFLAGS) -o $@
	$(CC) $(APP_OBJ) $(LFLAGE) -o app_user

#${LIB_ZLIB}/%.o:${LIB_ZLIB}/%.c
#	$(CC) $(CFLAGS) -c $< -o $@

#${LIB_MAD}/%.o:${LIB_MAD}/%.c
#	$(CC) $(CFLAGS) -c $< -o $@

#${LIB_TAG}/%.o:${LIB_TAG}/%.c
#	$(CC) $(CFLAGS) -c $< -o $@

${OID_OBJ}/%.o:${OID_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@


${OID_APP_OBJ}/%.o:${OID_APP_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -rf ${TARGET}
#	rm -rf ${LIB_ZLIB}/*.o
#	rm -rf ${LIB_MAD}/*.o
#	rm -rf ${LIB_TAG}/*.o
	rm -rf ${OID_OBJ}/*.o
#	find ${OID_OBJ} -name *.o



