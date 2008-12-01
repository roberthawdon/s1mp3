COMFPREF=common/
COMGPREF=$(COMFPREF)giveio
COMAPREF=$(COMFPREF)AdfuSession


$(COMGPREF).o: $(COMGPREF).h $(COMGPREF).cpp $(COMFPREF)AdfuSession.h $(COMFPREF)common.h $(COMFPREF)giveio.bin/giveio.h $(COMFPREF)AdfuDevice.h $(COMFPREF)AdfuException.h
	$(CC) -c $(CCFLAGS) $(COMGPREF).cpp -o $@

$(COMAPREF).o: $(COMAPREF).cpp $(COMFPREF)Adfu*.h
	$(CC) -c $(CCFLAGS) $(COMAPREF).cpp -o $@


include $(COMFPREF)giveio.bin/make.inc.mk

clean_common: clean_giveio_bin
	rm -f $(COMFPREF)*.o