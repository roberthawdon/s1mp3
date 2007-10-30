;******************************************************************************************************************************************
;*
;* title:     s1debug api
;* version:   1.0 (12/31/2006)
;* creation:  11/13/2006
;* target:    Z80 / ATJ2085 (s1mp3 player core)
;* compiler:  http://sdcc.sourceforge.net/
;* file:      test.asm
;* author:    wiRe (mailto:wiRe@gmx.net)
;* descr:     example wrapper for the debug api
;*
;* copyright (c) 2006 wiRe (http://www.s1mp3.de/)
;*
;******************************************************************************************************************************************

                        .include "../../inc/s1reg.inc"
                        .include "../../inc/s1sys.inc"
                        .area CODE (ABS)


;------------------------------------------------------------------------------------------------------------------------------------------
                        .org  0x0000
                        jp    main
;------------------------------------------------------------------------------------------------------------------------------------------
                        .org  0x0030
dbg_api_entry:          jp    dbg_api_entry_
;------------------------------------------------------------------------------------------------------------------------------------------
                        .org  0x0036
                        reti
;------------------------------------------------------------------------------------------------------------------------------------------
                        .org  0x0038
dbg_brkpnt:             jp    dbg_brkpnt_
;------------------------------------------------------------------------------------------------------------------------------------------
                        .org  0x0100
                        .include "dbgapi.asm"
;------------------------------------------------------------------------------------------------------------------------------------------


;------------------------------------------------------------------------------------------------------------------------------------------
                        .org  0x0400
main:                   di                                  ; disable interrupts
                        xor   a                             ; disable watchdog
                        out   (WATCHDOG_REG), a             ;

                        ld    a, #0b00110000                ; map ZRAM2(B1+B2) to MCU
                        out   (ZRAM2_MEMMAP_REG), a         ;
                        ld    a, #0b11110111                ; page in ZRAM2
                        out   (URAM_PAGE_REG), a            ;

                        ld    a, #0b00010000                ; select MCU clock source (24MHz)
                        out   (MCU_CLKCTRL_REG1), a         ;

                        ld    sp, #ZRAM2_ADDR               ; init stack pointer
                        im    #1                            ; set interrupt mode 1

main_lp:                call  test_output
                        rst   0x38                          ; call dbg_brkpnt
                        jr    main_lp                       ; loop forever

;------------------------------------------------------------------------------------------------------------------------------------------
test_output:            xor   a                             ; command: puts
                        ld    hl, #str                      ;
test_output_lp_a:       rst   0x30                          ; call dbg_api_entry
                        or    a                             ; returned zero?
                        jr    z, test_output_lp_a           ;   yes -> loop on errors

                        ld    a, #2                         ; command: puti
                        ld    hl, #-12345                   ;
test_output_lp_b:       rst   0x30                          ; call dbg_api_entry
                        or    a                             ; returned zero?
                        jr    z, test_output_lp_b           ;   yes -> loop on errors

                        ld    a, #1                         ; command: putch
                        ld    l, #', ;'                     ;
test_output_lp_c:       rst   0x30                          ; call dbg_api_entry
                        or    a                             ; returned zero?
                        jr    z, test_output_lp_c           ;   yes -> loop on errors

                        ld    l, a
                        ld    a, #3                         ; command: putui
                        ld    h, #0xA0                      ;
test_output_lp_d:       rst   0x30                          ; call dbg_api_entry
                        or    a                             ; returned zero?
                        jr    z, test_output_lp_d           ;   yes -> loop on errors

                        ld    a, #1                         ; command: putch
                        ld    l, #'\n ;'                    ;
test_output_lp_e:       rst   0x30                          ; call dbg_api_entry
                        or    a                             ; returned zero?
                        ret   nz                            ;    no -> return on success
                        jr    test_output_lp_e              ;   yes -> loop on errors

;------------------------------------------------------------------------------------------------------------------------------------------
str:                    .asciz "\nHello world from the S1MP3 player!\n"
