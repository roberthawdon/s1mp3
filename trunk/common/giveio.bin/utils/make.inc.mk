UTILS=$(GFPREF)utils/

$(UTILS)bin2c: $(UTILS)bin2c.cpp
	$(CC) $(CCFLAGS) $(UTILS)bin2c.cpp -o $(UTILS)bin2c$(EXT)

$(UTILS)ihx2bin: $(UTILS)ihx2bin.cpp $(UTILS)LoadHex.h
	$(CC) $(CCFLAGS) $(UTILS)ihx2bin.cpp -o $(UTILS)ihx2bin$(EXT)

clean_utils:
	$(DEL) $(UTILS)bin2c
	$(DEL) $(UTILS)ihx2bin
