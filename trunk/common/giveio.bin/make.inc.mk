GFPREF=$(COMFPREF)giveio.bin/
GNAME=giveio
GPREF=$(GFPREF)$(GNAME)


$(GPREF).obj: $(GPREF).asm inc/s1reg.inc inc/s1sys.inc
	cd $(GFPREF); as-z80 -o $(GNAME).obj $(GNAME).asm

$(GPREF).ihx: $(GPREF).obj
	link-z80 -n -- -i $(GPREF).ihx $(GPREF).obj

$(GPREF).bin: $(GPREF).ihx $(GFPREF)utils/ihx2bin
	$(GFPREF)utils/ihx2bin $(GPREF).bin $(GPREF).ihx

$(GPREF).h: $(GPREF).bin $(GFPREF)utils/bin2c$(EXT)
	$(GFPREF)utils/bin2c$(EXT) $(GPREF).bin giveio > $(GPREF).h

clean_giveio_bin: clean_utils
	$(DEL) $(GPREF).obj $(GPREF).ihx $(GPREF).bin $(GPREF).h

include $(GFPREF)utils/make.inc.mk
