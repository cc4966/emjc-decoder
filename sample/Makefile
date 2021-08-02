CC := gcc
CFLAGS := -I../src -I${LZFSE_DIR}/src
LDFLAGS := -L${LZFSE_DIR}/build/bin
LDLIBS := -llzfse -lpng
DEPS := ../src/sbix_emjc_decode.h
OBJS := sample.o ../src/sbix_emjc_decode.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sample: $(OBJS)
	$(CC) ${LDFLAGS} ${LDLIBS} -o $@ $^ $(CFLAGS)