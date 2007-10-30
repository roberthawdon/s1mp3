;******************************************************************************************************************************************
;*
;* title:     s1debug api
;* version:   1.0 (12/31/2006)
;* creation:  11/13/2006
;* target:    Z80 / ATJ2085 (s1mp3 player core)
;* compiler:  http://sdcc.sourceforge.net/
;* file:      dbgapi.asm
;* author:    wiRe (mailto:wiRe@gmx.net)
;* descr:     debug api for i2c debug interface
;*
;* copyright (c) 2006 wiRe (http://www.s1mp3.de/)
;*
;******************************************************************************************************************************************


;------------------------------------------------------------------------------------------------------------------------------------------
DBG_INTERFACE_ADDR = 0x70
;------------------------------------------------------------------------------------------------------------------------------------------
I2C_CTRL_INIT     = 0x80
I2C_CTRL_START    = 0x85
I2C_CTRL_RESTART  = 0x8D
I2C_CTRL_STOP     = 0x88
;------------------------------------------------------------------------------------------------------------------------------------------




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_brkpnt
; DESCRIPTION           freeze the cpu and wait for the pc to handle this breakpoint
; INPUT                 n/a
; OUTPUT                n/a
; WASTE                 time
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_:            call  dbg_enter                     ; init debug mode (waste: B, F, IX, IY, SP)
                       ;ld    hl, #0                        ; init memory address to zero

dbg_brkpnt_resync:      ld    c, #0b11111111                ; first sync for brkpnt (7)
                        call  dbg_tx_sync                   ;
                        jr    z, dbg_brkpnt_resync          ; loop until we have a sync

dbg_brkpnt_lp:          call  dbg_tx_zero                   ; wait for command from host
                        jr    z, dbg_brkpnt_resync          ;   error -> resync

                        or    a                             ; command == 0? (return)
                        jr    z, dbg_leave                  ;   yes -> leave debug mode

                        ld    bc, #dbg_brkpnt_lp            ; set return address
                        push  bc                            ;

                        dec   a                             ; command == 1?
                        jr    z, dbg_brkpnt_sma             ;   yes -> set memory address
                        dec   a                             ; command == 2?
                        jr    z, dbg_brkpnt_rdm             ;   yes -> read from memory
                        dec   a                             ; command == 3?
                        jr    z, dbg_brkpnt_wrm             ;   yes -> write to memory
                        dec   a                             ; command == 4?
                        jr    z, dbg_brkpnt_rdp             ;   yes -> read from port
                        dec   a                             ; command == 5?
                        jr    z, dbg_brkpnt_wrp             ;   yes -> write to port
                        dec   a                             ; command == 6?
                        jr    z, dbg_brkpnt_rds             ;   yes -> read from stack
                        dec   a                             ; command == 7?
                        jr    z, dbg_brkpnt_wrs             ;   yes -> write to stack
                        ret                                 ; unknown command -> loop

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_tx:          call  dbg_tx_byte                   ; transfer byte
                        ret   nz                            ;   success -> return
                        pop   de                            ;   error   -> remove return address from stack
                        pop   de                            ;              and again
                        jr    dbg_brkpnt_resync             ;              now resync for next command

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_sma:         call  dbg_brkpnt_tx                 ; receive low memory address (send zero)
                        ld    l, a                          ;
                        call  dbg_brkpnt_tx                 ; receive high memory address (send echo)
                        ld    h, a                          ;
                        ret                                 ; return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_rdm:         ld    a, (hl)                       ; read byte from memory and send
                        inc   hl                            ; increment memory address
dbg_brkpnt_tx_ret:      call  dbg_brkpnt_tx                 ; send byte
                        ret                                 ; return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_wrm:         call  dbg_brkpnt_tx                 ; receive byte and write to memory (send zero)
                        ld    (hl), a                       ;
                        inc   hl                            ; increment memory address
                        ret                                 ; return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_rdp:         call  dbg_brkpnt_tx                 ; receive port number (send zero)
                        ld    (#dbg_brkpnt_rdp_in+1), a     ;
dbg_brkpnt_rdp_in:      in    a, (0)                        ; read byte from i/o port
                        jr    dbg_brkpnt_tx_ret             ; send byte and return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_wrp:         call  dbg_brkpnt_tx                 ; receive port number (send zero)
                        ld    (#dbg_brkpnt_wrp_out+1), a    ;
                        call  dbg_brkpnt_tx                 ; receive byte (send echo)
dbg_brkpnt_wrp_out:     out   (0), a                        ; write to i/o port
                        ret                                 ; return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_rds:         call  dbg_brkpnt_tx                 ; receive stack index/relative address (send zero)
                        ld    (#dbg_brkpnt_rds_ld+2), a     ;
dbg_brkpnt_rds_ld:      ld    a, 0(ix)                      ; read byte from stack
                        jr    dbg_brkpnt_tx_ret             ; send byte and return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_brkpnt_wrs:         call  dbg_brkpnt_tx                 ; receive stack index/relative address (send zero)
                        ld    (#dbg_brkpnt_wrs_ld+2), a     ;
                        call  dbg_brkpnt_tx                 ; receive byte (send echo)
dbg_brkpnt_wrs_ld:      ld    0(ix), a                      ; write byte to stack
                        ret                                 ; return




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_leave (don't call, just jump into)
; DESCRIPTION           leave debug mode (stop i2c controller, restore configuration and registers)
; INPUT                 n/a
; OUTPUT                n/a
; WASTE                 restores all registers and returns to caller
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_leave:              xor   a                             ; disable i2c controller
                        out   (I2C_CONTROL_REG), a          ;

                        pop   af                            ; restore configuration
                        out   (I2C_ADDR_REG), a             ;
                        pop   af                            ;
                        out   (I2C_CONTROL_REG), a          ;
                        pop   af                            ;
                        out   (MFP_SELECT_REG), a           ;

                        pop   af                            ; remove I and R register
                        pop   af                            ; remove stack-ptr entry

                        pop   af                            ; restore registers
                        pop   bc                            ;
                        pop   de                            ;
                        pop   hl                            ;
                        pop   ix                            ;
                        pop   iy                            ;
                        ret                                 ; return




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_enter
; DESCRIPTION           enter debug mode (store configuration and registers, prepare i2c controller)
; INPUT                 n/a
; OUTPUT                n/a
; WASTE                 B, D, E, F, IX, IY, SP
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_enter:              ex    (sp), iy                      ; get return address and store registers
                        push  ix                            ;
                        push  hl                            ;
                        push  de                            ;
                        push  bc                            ;
                        push  af                            ;

                       ;di                                  ;   (howto store IFF1/IFF2?)

                        ld    b, a                          ; backup register a

                        ld    ix, #0                        ; remember stack-pointer
                        add   ix, sp                        ;
                        push  ix                            ; place stack-pointer on stack

                        ld    a, i                          ; store i and r register
                        ld    e, a                          ;
                        ld    a, r                          ;
                        ld    d, a                          ;
                        push  de                            ;

                        in    a, (MFP_SELECT_REG)           ; store ports
                        push  af                            ;
                        in    a, (I2C_CONTROL_REG)          ;
                        push  af                            ;
                        in    a, (I2C_ADDR_REG)             ;
                        push  af                            ;

                        xor   a                             ; disable i2c controller
                        out   (I2C_CONTROL_REG), a          ;
                        in    a, (MFP_SELECT_REG)           ; enter F1 mode
                        and   #0b00011111                   ;
                        out   (MFP_SELECT_REG), a           ;
                        ld    a, #I2C_CTRL_INIT             ; enable i2c controller in master mode
                        out   (I2C_CONTROL_REG), a          ;
                        in    a, (MFP_SELECT_REG)           ; leave F1 mode
                        or    #0b00100000                   ;
                        out   (MFP_SELECT_REG), a           ;

                        ld    a, b                          ; restore register a
                        push  iy                            ; return
                        ret                                 ;




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_api_entry
; DESCRIPTION           handle debug api functions
; INPUT                 A       = command value
;                       HL      = data/pointer
; OUTPUT                A       = result (flags do not change!)
; WASTE                 A
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_:         call  dbg_enter                     ; init debug mode (waste: B, F, IX, IY, SP)

                        ld    bc, #dbg_api_entry_ret        ; set return address
                        push  bc                            ;

                       ;ld    a, 1(ix)                      ; load command value
                        or    a                             ; command == 0?
                        jr    z, dbg_api_entry_puts         ;   yes -> put string
                        dec   a                             ; command == 1?
                        jr    z, dbg_api_entry_putch        ;   yes -> put char
                        dec   a                             ; command == 2?
                        jr    z, dbg_api_entry_puti         ;   yes -> put integer
                        dec   a                             ; command == 3?
                        jr    z, dbg_api_entry_putui        ;   yes -> put unsigned integer
                        dec   a                             ; command == 4?
                        jr    z, dbg_api_entry_getch        ;   yes -> get char
                        xor   a                             ;   no  -> unknown command, return error
                        ret                                 ;

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_ret:      ld    1(ix), a                      ; set return value
                        jr    dbg_leave                     ; leave debug mode and return

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_putch:    ld    a, #0b01011010                ; sync for putch (2)
                        jr    dbg_api_entry_put_init        ;
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_puti:     ld    a, #0b11011110                ; sync for puti (3)
                        jr    dbg_api_entry_put_init        ;
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_putui:    ld    a, #0b00111001                ; sync for putui (4)
dbg_api_entry_put_init: ld    (#dbg_api_entry_put+1), a     ; remember sync code
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_put:      ld    c, #0x00              ;^^     ; sync for last stored sync code
                        call  dbg_tx_sync                   ;
                        ret   z                             ;   error -> return error

                        ld    a, l                          ; send low byte
                        call  dbg_tx_byte                   ;
                        ret   z                             ;   error -> return
                        cp    #1                            ; success?
                        jr    nz, dbg_api_entry_put         ;   no -> retry

                        ld    a, h                          ; send high byte
                        call  dbg_tx_byte                   ;
                        ret   z                             ;   error -> return
                        cp    #1                            ; success?
                        jr    nz, dbg_api_entry_put         ;   no -> retry

                        jr    dbg_tx_triple_success         ; return success

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_puts:     ld    c, #0b10011100                ; sync for puts (1)
                        call  dbg_tx_sync                   ;
                        ret   z                             ;   error -> return
dbg_api_entry_puts_lp:  ld    a, (hl)                       ; read next character
                        or    a                             ; zero?
                        jr    z, dbg_tx_triple_success      ;   yes -> return success
                        call  dbg_tx_byte                   ; send character
                        ret   z                             ;   error -> return
                        cp    #1                            ; success?
                        jr    nz, dbg_api_entry_puts        ;   no  -> retry
                        inc   hl                            ;   yes -> increment pointer
                        jr    dbg_api_entry_puts_lp         ;          and loop

;------------------------------------------------------------------------------------------------------------------------------------------
dbg_api_entry_getch:    ld    c, #0b10111101                ; sync for getch (5)
                        call  dbg_tx_sync                   ;
                        ret   z                             ;   error -> return
dbg_api_entry_getch_lp: call  dbg_tx_zero                   ; receive keycode
                        ret   z                             ;   error -> return
                        or    a                             ; success (!=0)?
                        ret   nz                            ;   yes -> return
                        jr    dbg_api_entry_getch_lp        ; retry (wait for key)




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_tx_triple
; DESCRIPTION           transfer and receive 9 data-bits in 3 clocks
; INPUT                 DE = data to transfer (D0 = MSB, E0 = LSB)
; OUTPUT                DE = data received (D0 = MSB, E0 = LSB)
;                       zero flag set on error/timeout
; WASTE                 A, B, D, E, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_tx_triple:          ld    b, #3                         ; init loop counter
                        rr    d                             ; shift 1st data bit into carry
                        ld    d, e                          ; load transfer data
                        jr    dbg_tx_triple_start           ; enter loop

dbg_tx_triple_lp:       rl    d                             ; shift next data nibble into accu (bit0 = MSB)
dbg_tx_triple_start:    rra                                 ;
                        rl    d                             ;
                        rra                                 ;
                        rl    d                             ;
                        rra                                 ;

                        call  dbg_tx_nibble                 ; tx/rx data [8..6], [5..3], [2..0}
                        ret   z                             ;   error -> return (zero flag set)

                        rra                                 ; shift result nibble from accu (bit0 = MSB) into result register
                        rl    e                             ;
                        rra                                 ;
                        rl    e                             ;
                        rra                                 ;
                        rl    e                             ;

                        djnz  dbg_tx_triple_lp              ; loop 3 times

                        ld    d, b                          ; store MSB
                        rl    d                             ;

dbg_tx_triple_success:  or    #0xFF                         ; return success (zero flag cleared)
                        ret                                 ;


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_tx_nibble
; DESCRIPTION           transfer and receive 3 data-bits in 1 clock
; INPUT                 A[7..5] = data to transfer (A5 = MSB)
; OUTPUT                A[2..0] = data received (A0 = MSB)
;                       A[3]    = ready bit (cleared on error)
;                       zero flag set on error/timeout
; WASTE                 A, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_tx_nibble:          push  bc                            ; store registers
                        push  de                            ;

                        and   #0b11100000                   ; mask data
                        ld    d, a                          ; backup data
                        call  dbg_i2c_read                  ; read old clock (waste: A, B, F)
                        and   #0b00011000                   ; isolate clock and ready pin
                        or    d                             ; add data
                        ld    d, a                          ;
                        call  dbg_i2c_write                 ; write new data with old clock (waste: A, B, C, F)
                        ld    a, d                          ;
                        xor   #0b00010000                   ; toggle clock pin
                        call  dbg_i2c_write                 ; rewrite data with toggled clock (waste: A, B, C, F)

                        ld    c, #0                         ; init counter
dbg_tx_nibble_rdy_lp:   call  dbg_i2c_read                  ; read ready status (waste: A, B, F)
                        xor   d                             ; xor with last ready bit
                        bit   3, a                          ; test if ready bit was toggled?
                        jr    nz, dbg_tx_nibble_ret         ;   yes -> return success (zero flag cleared)
                        dec   c                             ; timeout?
                        jr    nz, dbg_tx_nibble_rdy_lp      ;   no  -> keep waiting
                                                            ;   yes -> return error (zero flag set)
dbg_tx_nibble_ret:      pop   de                            ; restore registers
                        pop   bc                            ;
                        ret                                 ; and return


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_tx_sync
; DESCRIPTION           send zero for 3 clocks, then transfer nibble X and receive data for 1 clock, repeat until host returned nibble YYY
; INPUT                 C = 0bXXX11YYY (X: sync code, Y: result)
; OUTPUT                zero flag set on error/timeout
; WASTE                 A, B, C, D, E, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_tx_sync:            ld    b, #9                         ; init retry counter
                        ld    de, #0                        ;

dbg_tx_sync_lp:         push  bc                            ; write 3 clocks of zero data
                        call  dbg_tx_triple                 ;
                        pop   bc                            ;
                        jr    z, dbg_tx_sync_continue       ;   error -> continue loop

                        ld    a, c                          ; write sync command
                        call  dbg_tx_nibble                 ;
                        xor   c                             ; verify result and ready bit
                        and   #0b00001111                   ;
                        jr    z, dbg_tx_triple_success      ;   valid -> return success

dbg_tx_sync_continue:   djnz  dbg_tx_sync_lp                ; timeout?
                        xor   a                             ;   yes -> return error (set zero flag)
                        ret                                 ;


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_tx_byte, dbg_tx_zero
; DESCRIPTION           send/receive one byte with checksum (odd parity bit)
; INPUT                 A = character to send
; OUTPUT                A = character received
;                       zero flag set on error/timeout
; WASTE                 A, B, D, E, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_tx_zero:            xor   a
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_tx_byte:            ld    e, a                          ; setup transfer register
                        call  dbg_calc_parity               ; calculate parity bit (waste: A, F)
                        ld    d, a                          ;
                        call  dbg_tx_triple                 ; transfer 9 data bits (waste: A, B, D, E, F)
                        ret   z                             ;   error -> return
                        call  dbg_calc_parity               ; calculate parity bit (waste: A, F)
                        xor   d                             ; parity valid? (return flag)
                        xor   #1                            ;
                        ld    a, e                          ; load result
                        ret                                 ;




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_calc_parity
; DESCRIPTION           calculate odd parity bit
; INPUT                 E = data (8 bits)
; OUTPUT                A = valid parity bit
; WASTE                 A, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_calc_parity:        xor   a                             ; evaluate parity bit
                        xor   e                             ;
                        ld    a, #0                         ;
                        ret   po                            ;
                        inc   a                             ;
                        ret                                 ;




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_i2c_wait
; DESCRIPTION           wait until receiption/transmission gets finished or until a timeout occurs
; INPUT                 n/a
; OUTPUT                A = status
;                       zero flag set on error/timeout
; WASTE                 A, B, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_i2c_wait:           ld    b, #0                         ; init timeout counter
dbg_i2c_wait_lp:        in    a, (I2C_STATUS_REG)           ; read status
                        bit   5, a                          ; start bit detected?
                        jr    z, dbg_i2c_wait_continue      ;   no  -> continue loop
                        bit   7, a                          ; data ready?
                        ret   nz                            ;   yes -> return success (zero flag cleared)
dbg_i2c_wait_continue:  djnz  dbg_i2c_wait_lp               ; timeout?
                        ret                                 ;   yes -> return error (zero flag set)


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_i2c_read
; DESCRIPTION           read one byte from i2c interface
; INPUT                 n/a
; OUTPUT                A = byte read
; WASTE                 A, B, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_i2c_read:           ld    a, #DBG_INTERFACE_ADDR+1      ; address i2c interface (read mode)
                        call  dbg_i2c_start                 ; send start condition

dbg_i2c_read_addr:      in    a, (I2C_DATA_REG)             ; read address

                        call  dbg_i2c_wait                  ; wait until transfer completed
                        jr    z, dbg_i2c_read               ;   timeout? -> retry
                       ;bit   3, a                          ; was data?
                       ;jr    z, dbg_i2c_read_addr          ;   no -> read lamer

                        call  dbg_i2c_stop                  ; send stop condition
                        in    a, (I2C_DATA_REG)             ; read data
                        ret                                 ;   and return


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_i2c_write
; DESCRIPTION           write one byte to i2c interface
; INPUT                 A = byte to write
; OUTPUT                n/a
; WASTE                 A, B, C, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_i2c_write:          or    #0b00001111                   ; turn input pins high in all cases
                        ld    c, a                          ; backup data

dbg_i2c_write_rep:      ld    a, #DBG_INTERFACE_ADDR        ; address i2c interface (write mode)
                        call  dbg_i2c_start                 ; send start condition

dbg_i2c_write_data:     ld    a, c                          ; write data
                        out   (I2C_DATA_REG), a             ;

                        call  dbg_i2c_wait                  ; wait until transfer completed
                        jr    z, dbg_i2c_write_rep          ;   timeout? -> retry
                       ;bit   3, a                          ; was data?
                       ;jr    z, dbg_i2c_write_data         ;   no -> rewrite

                       ;jr    dbg_i2c_stop                  ; send stop condition and return
                       ;.
                       ;.
;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_i2c_stop
; DESCRIPTION           send stop condition to i2c bus
; INPUT                 n/a
; OUTPUT                zero flag set on error/timeout
; WASTE                 A, B, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_i2c_stop:           ld    a, #I2C_CTRL_STOP             ; send stop bit
                        out   (I2C_CONTROL_REG), a          ;

                        ld    b, #0                         ; init timeout cntr
dbg_i2c_stop_lp:        in    a, (I2C_STATUS_REG)           ; read status
                        bit   6, a                          ; stop bit detected?
                        ret   nz                            ;   yes -> return success (zero flag cleared)
                        djnz  dbg_i2c_stop_lp               ; timeout?
                        ret                                 ; return error (zero flag set)


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION              dbg_i2c_start
; DESCRIPTION           send start condition to i2c bus (never returns on errors until sent)
; INPUT                 A = device address
; OUTPUT                n/a
; WASTE                 A, B, F
;------------------------------------------------------------------------------------------------------------------------------------------
dbg_i2c_start:          out   (I2C_ADDR_REG), a             ; write device address
dbg_i2c_start_repeat:   or    #0xFF                         ; clear status register
                        out   (I2C_STATUS_REG), a           ;
                        ld    a, #I2C_CTRL_START            ; send start condition
                        out   (I2C_CONTROL_REG), a          ;
                        call  dbg_i2c_wait                  ; wait until start bit was detected and first transfer finished
                        ret   nz                            ;   success -> return
                        call  dbg_i2c_stop                  ; send stop condition
                        jr    dbg_i2c_start_repeat          ; and repeat
                        nop                                 ;
                        nop                                 ;
                        ld    (hl),a                        ;
                        ld    l,c                           ;
                        ld    d,d                           ;
                        ld    h,l                           ;
