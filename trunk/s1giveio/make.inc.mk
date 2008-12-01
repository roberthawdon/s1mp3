GIFPREF=s1giveio/
GIDPREF=$(GIFPREF)dasm

$(GIDPREF).o: $(GIDPREF).cpp $(GIDPREF).h common/common.h
	$(CC) -c $(CCFLAGS) -o $@ $(GIDPREF).cpp

$(GIFPREF)s1giveio: $(GIFPREF)main.cpp $(GIDPREF).h $(GIDPREF).o common/common.h common/giveio.h common/giveio.o common/AdfuSession.h common/AdfuSession.o
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $(GIFPREF)main.cpp common/AdfuSession.o common/giveio.o $(GIDPREF).o

clean_s1giveio:
	rm -f $(GIDPREF).o
	rm -f $(GIFPREF)s1giveio
