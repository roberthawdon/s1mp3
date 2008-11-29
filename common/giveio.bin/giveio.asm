;******************************************************************************************************************************************
;*
;* giveio-bin v3.1 (05/09/2008)
;*
;* file:      giveio.asm
;* creation:  08/11/2006
;* target:    Z80 (s1mp3 player core)
;* compiler:  http://sdcc.sourceforge.net/
;* author:    wiRe _at_ gmx _dot_ net
;*
;* copyright (c)2005-2007 wiRe.
;* this file is licensed under the GNU GPL.
;* no warranty of any kind is explicitly or implicitly stated.
;*
;******************************************************************************************************************************************
;*
;* DESCRIPTION
;*
;* gets uploaded to memory using the ADFU interface of the original firmware
;* then it takes over the system and gives direct access to io ports and memory
;* by sending read and write packets over usb
;*
;* this code lives from 0x3400 to 0x37FF.
;* since usb could only transfer to B1/B2 (ZRAM2: 0x4000..0x4FFF, 4kb)
;* all rx/tx usb data will be placed inside this area and is limited to 4kb.
;*
;* giveio simulates the used protocol of the ADFU device and wraps commands inside CBW's.
;* the status gets returned as a CSW. CBW and CSW get stored at the beginning of the ZRAM2.
;* v9 devices with large BROMs get supported since v3.0.
;*
;* since version v3.1, giveio supports fast dma1 transfers to copy memory.
;*
;******************************************************************************************************************************************
                .include "../../inc/s1reg.inc"
                .include "../../inc/s1sys.inc"


;------------------------------------------------------------------------------------------------------------------------------------------
START_ADDR			= 0x3400
STACK_ADDR          = START_ADDR + 0x400 - 8

CBW_LENGTH          = 31                                    ;cbw structure
CBW_ADDR            = ZRAM2_ADDR
CBW_SIGNATURE       = CBW_ADDR+0x00
CBW_TAG             = CBW_ADDR+0x04
CBW_TX_DATA_LENGTH  = CBW_ADDR+0x08
CBW_FLAGS           = CBW_ADDR+0x0C
CBW_CB              = CBW_ADDR+0x0F

CSW_LENGTH          = 13                                    ;csw structure
CSW_ADDR            = ZRAM2_ADDR
CSW_SIGNATURE       = CSW_ADDR+0x00
CSW_TAG             = CSW_ADDR+0x04
CSW_RESIDUE         = CSW_ADDR+0x08
CSW_STATUS          = CSW_ADDR+0x0C

REL_DATA_ADDR       = 0x0020                                ;address for data packages relative to the ZRAM2 address
DATA_ADDR           = ZRAM2_ADDR + REL_DATA_ADDR            ;address for data packages
MAX_DATA_LENGTH     = ZRAM2_SIZE - REL_DATA_ADDR            ;maximum allowed length of data


;------------------------------------------------------------------------------------------------------------------------------------------
                .area CODE (ABS)
                .org  START_ADDR
                jp    main




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION      usb_rx
; DESCRIPTION   receives <hl> bytes from host into ZRAM2 (address is 0x4000, if mapped to cpu)
; INPUT         hl = start address
;               bc = bytes to transfer
;------------------------------------------------------------------------------------------------------------------------------------------
usb_rx:         ld    a, #0xF7                              ;bank in zram2 (b1/2)
                out   (URAM_PAGE_REG), a
;------------------------------------------------------------------------------------------------------------------------------------------
                ld    a, (v9flag)                           ;v9 device?
                or    a                                     ;
                jr    nz, usb_rx_v9                         ;  yes -> exec v9 usb code
;------------------------------------------------------------------------------------------------------------------------------------------
                in    a, (ZRAM2_MEMMAP_REG)                 ;let usb use b1/b2 memory
                and   #0xCF
                out   (ZRAM2_MEMMAP_REG),a

                ld    a, #3                                 ;select ep1-out
                out   (USB_EPI_REG), a
                ld    a, #63                                ;set maximum packet size (= 64)
                out   (USB_EPI_MPS_REG), a
                xor   a                                     ;nak
                out   (USB_EPI_MODE_REG), a
                ld    a, h                                  ;set start address in b1
                out   (USB_EPI_ADDR_HI_REG), a
                ld    a, l
                out   (USB_EPI_ADDR_LO_REG), a
                ld    a, b                                  ;set byte counter (high, low)
                out   (USB_EPI_CNTR_HI_REG), a
                ld    a, c
                out   (USB_EPI_CNTR_LO_REG), a

usb_rx_wh:      in    a, (USB_EPI_EPSBR_REG)                ;poll buffer ready status, wait for EP1_OUT_BR -> 0
                and   a, #8
                jr    z, usb_rx_wh
usb_rx_wl:      in    a, (USB_EPI_EPSBR_REG)
                and   a, #8
                jr    nz, usb_rx_wl

                in    a, (ZRAM2_MEMMAP_REG)                 ;let cpu use b1/b2
                or    #0x30
                out   (ZRAM2_MEMMAP_REG),a

                ret                                         ;return
;------------------------------------------------------------------------------------------------------------------------------------------
usb_rx_v9:      ld    a, #1                                 ;select ep1-out
                out   (0x58), a
                ld    a, #0x28
                out   (0x5A), a
                ld    a, #0xFF
                out   (0x5C), a

                ld    a, c                                  ;set byte counter (low, high)
                out   (0x63), a
                ld    a, b
                out   (0x64), a
                xor   a                                     ;???
                out   (0x65), a
                ld    a, l                                  ;set start address in b1
                out   (0x89), a
                ld    a, h
                out   (0x8A), a
                xor   a                                     ;start usb transfer (rx)
                out   (0x51), a
                inc   a
                out   (0x51), a

                push  bc                                    ;wait for tx finish
                ld    b, #3
                djnz  .
usb_rx_v9_lp:   in    a, (0x51)
                bit   0, a
                jr    nz, usb_rx_v9_lp
                ld    b, #10
                djnz  .
                pop   bc

			          ld	  a, #1
			          out	  (0x58), a
			          ld	  a, #0x20
			          out	  (0x5A), a

                ret


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION      usb_tx
; DESCRIPTION   transfers <hl> bytes from ZRAM2 to host
; INPUT         hl = start address
;               bc = bytes to transfer
;------------------------------------------------------------------------------------------------------------------------------------------
usb_tx:         ld    a, #0xF7                              ;bank in zram2 (b1/2)
                out   (URAM_PAGE_REG), a
;------------------------------------------------------------------------------------------------------------------------------------------
                ld    a, (v9flag)                           ;v9 device?
                or    a                                     ;
                jr    nz, usb_tx_v9                         ;  yes -> exec v9 usb code
;------------------------------------------------------------------------------------------------------------------------------------------
                in    a, (ZRAM2_MEMMAP_REG)                 ;let usb use b1/b2 memory
                and   #0xCF
                out   (ZRAM2_MEMMAP_REG),a

                ld    a, #4                                 ;select ep2-in
                out   (USB_EPI_REG), a
                ld    a, #63                                ;set maximum packet size (= 64)
                out   (USB_EPI_MPS_REG), a
                xor   a                                     ;nak
                out   (USB_EPI_MODE_REG), a
                ld    a, h                                  ;set start address in b1
                out   (USB_EPI_ADDR_HI_REG), a
                ld    a, l
                out   (USB_EPI_ADDR_LO_REG), a
                ld    a, b                                  ;set byte counter (high, low)
                out   (USB_EPI_CNTR_HI_REG), a
                ld    a, c
                out   (USB_EPI_CNTR_LO_REG), a

usb_tx_wh:      in    a, (USB_EPI_EPSBR_REG)                ;poll buffer ready status, wait for EP2_IN_BR -> 0
                and   a, #16
                jr    z, usb_tx_wh
usb_tx_wl:      in    a, (USB_EPI_EPSBR_REG)
                and   a, #16
                jr    nz, usb_tx_wl

                in    a, (ZRAM2_MEMMAP_REG)                 ;let cpu use b1/b2
                or    #0x30
                out   (ZRAM2_MEMMAP_REG),a

                ret                                         ;return
;------------------------------------------------------------------------------------------------------------------------------------------
usb_tx_v9:      ld    a, #2                                 ;select ep2-in
                out   (0x58), a
                ld    a, #4 ;0x24
                out   (0x5A), a
               ;ld    a, #0xFF
               ;out   (0x5C), a

                ld    a, c                                  ;set byte counter (low, high)
                out   (0x63), a
                ld    a, b
                out   (0x64), a
                xor   a                                     ;???
                out   (0x65), a
                ld    a, l                                  ;set start address in b1
                out   (0x89), a
                ld    a, h
                out   (0x8A), a
                ld    a, #2                                 ;start usb transfer (tx)
                out   (0x51), a
                inc   a
                out   (0x51), a

                push  bc                                    ;wait for tx finish
                ld    b, #3
                djnz  .
                pop   bc
usb_tx_v9_l1:   in    a, (0x51)
                bit   0, a
                jr    nz, usb_tx_v9_l1
usb_tx_v9_l2:   in    a, (0x5B)
                bit   1, a
                jr    z, usb_tx_v9_l2

			          ld	  a, #2
			          out	  (0x58), a
			          ld	  a, #0x20
			          out	  (0x5A), a

                ret




;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION      v9detect
; DESCRIPTION   detect if this is a v9 device
; OUTPUT        a != 0 on v9 devices
;------------------------------------------------------------------------------------------------------------------------------------------
v9detect:       xor   a
                out   (EM_PAGE_LO_REG), a
                out   (EM_PAGE_HI_REG), a

                ld    de, #0x8000
                ld    hl, #0xA000
                ld    b, #32
v9detect_lp:    ld    a, (de)
                sub   a, (hl)
                ret   nz
                djnz  v9detect_lp

                ret




;==========================================================================================================================================
; FUNCTION      main
;==========================================================================================================================================
main:           di                                          ;permanently disable interrupts and kill stupid watchdog
                im    1
                xor   a
                out   (MINT_ENABLE_REG), a
                out   (WATCHDOG_REG), a
                out   (0xF0), a                             ;turn off oled/colored display
               ;out   (0xF2), a

                ld    sp, #STACK_ADDR                       ;init our own stack

                call  v9detect                              ;detect v9 devices
                ld    (v9flag), a
                or    a
                jr    z, main_lp

                ld    a, #0x09                              ;setup dma clock source on v9 devices
                out   (MCU_A15CTRL_REG), a

;------------------------------------------------------------------------------------------------------------------------------------------

main_lp:                                                    ;our main loop, we never will return now

                ld    hl, #cmd_ret                          ;push return address
                push  hl

;------------------------------------------------------------------------------------------------------------------------------------------

cmd_rx_cbw:     ld    hl, #CBW_ADDR-ZRAM2_ADDR              ;receive cbw packet
                ld    bc, #CBW_LENGTH
                call  usb_rx

                ld    hl, #CBW_SIGNATURE                    ;verify cbw signature
                ld    a, (hl)
                cp    a, #0x55
                jr    nz, cmd_rx_cbw
                inc   hl
                ld    a, (hl)
                cp    a, #0x53
                jr    nz, cmd_rx_cbw
                inc   hl
                ld    a, (hl)
                cp    a, #0x42
                jr    nz, cmd_rx_cbw
                inc   hl
                ld    a, (hl)
                cp    a, #0x43
                jr    nz, cmd_rx_cbw

;------------------------------------------------------------------------------------------------------------------------------------------

                ld    bc, (CBW_TX_DATA_LENGTH)              ;read data transfer length
                ld    a, b                                  ;data available?
                or    a, c
                jr    z, cmd_no_data

                ld    a, (CBW_FLAGS)                        ;read cbw flags
                and   a, #0x80                              ;receive data?
                jr    nz, cmd_data_in

                ld    hl, #REL_DATA_ADDR                    ;receive cmd data
                call  usb_rx

               ;jr    cmd_data_out

;------------------------------------------------------------------------------------------------------------------------------------------

cmd_data_out:   ld    a, (CBW_CB)                           ;execute commands with host->device data

                cp    #'d;'                                 ;dma command?
                jp    z, cmd_dma_out                        ;  yes -> execute command
                cp    #'m;'                                 ;mem command?
                jp    z, cmd_mem_out                        ;  yes -> execute command
                cp    #'X;'                                 ;execute command?
                jp    z, cmd_exec_out                       ;  yes -> execute command
                jp    cmd_nop                               ;no command otherwise

;------------------------------------------------------------------------------------------------------------------------------------------

cmd_no_data:    ld    a, (CBW_CB)                           ;execute commands with no data

                cp    #'p;'                                 ;port command?
                jp    z, cmd_port_out                       ;  yes -> execute command
                cp    #'m;'                                 ;mem command?
                jp    z, cmd_mem_outb                       ;  yes -> execute command
                cp    #'X;'                                 ;execute command?
                jp    z, cmd_exec                           ;  yes -> execute command
                cp    #'R;'                                 ;reset command?
                jp    z, cmd_reset                          ;  yes -> reset device
                jp    cmd_nop                               ;no command otherwise

;------------------------------------------------------------------------------------------------------------------------------------------

cmd_data_in:    ld    a, (CBW_CB)                           ;execute commands with device->host data

                cp    #'d;'                                 ;dma command?
                jp    z, cmd_dma_in                         ;  yes -> execute command
                cp    #'m;'                                 ;mem command?
                jp    z, cmd_mem_in                         ;  yes -> execute command
                cp    #'p;'                                 ;port command?
                jp    z, cmd_port_in                        ;  yes -> execute command
                cp    #'i;'                                 ;info command?
                jp    z, cmd_info                           ;  yes -> execute command
                jp    cmd_nop                               ;no command otherwise

;------------------------------------------------------------------------------------------------------------------------------------------

               ;pop   hl                                    ;remove return address

;------------------------------------------------------------------------------------------------------------------------------------------

cmd_ret:        ld    bc, (CBW_TX_DATA_LENGTH)              ;read data transfer length
                ld    a, b                                  ;data available?
                or    a, c
                jr    z, cmd_tx_csw

                ld    a, (CBW_FLAGS)                        ;read cbw flags
                and   a, #0x80                              ;send data?
                jr    z, cmd_tx_csw

                ld    hl, #REL_DATA_ADDR                    ;transfer cmd data
                call  usb_tx

cmd_tx_csw:     ld    a, #0x53                              ;set csw signature
                ld    (CSW_SIGNATURE+3), a
                xor   a                                     ;set csw status (good status)
                ld    (CSW_STATUS), a
                ld    b, a                                  ;clear csw data residue
                ld    c, a
                ld    (CSW_RESIDUE+0), bc
                ld    (CSW_RESIDUE+2), bc

                ld    hl, #CSW_ADDR-ZRAM2_ADDR              ;transfer csw packet to host
                ld    bc, #CSW_LENGTH
                call  usb_tx

                jp    main_lp                               ;infinite loop




;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_nop
; DESCRIPTION   no operation
; INPUT         n/a
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_nop:        ret


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_info
; DESCRIPTION   read information, enables detection of giveio
; INPUT         bc = data size
; OUTPUT        DATA[0] = version code
;               DATA[1] = module name string
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_info:       ld    hl, #cmd_info_data                    ;copy data to zram2
                ld    de, #DATA_ADDR
                ld    bc, #CMD_INFO_LENGTH
                ldir
                ret

cmd_info_data:  .db   0x31
v9flag:         .db   0x00                                  ;gets set to one on v9 devices
                .asciz "s1giveio"
CMD_INFO_LENGTH = . - cmd_info_data


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_port_in
; DESCRIPTION   read port
; INPUT         CB[1] = port value
; OUTPUT        DATA[0] = result
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_port_in:    ld    a, (CBW_CB+1)                         ;read port value
                ld    c, a
                in    a, (c)                                ;read port
                ld    (DATA_ADDR), a                        ;write result
                ret


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_port_out
; DESCRIPTION   write port
; INPUT         CB[1] = port value
;               CB[2] = data
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_port_out:   ld    a, (CBW_CB+1)                         ;read port value
                ld    c, a
                ld    a, (CBW_CB+2)                         ;read data
                out   (c), a                                ;write port
                ret


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_mem_in
; DESCRIPTION   read <bc> bytes from memory address
; INPUT         bc = data size
;               CB[2]:CB[1] = address
; OUTPUT        DATA[0..bc-1]
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_mem_in:     ld    hl, (CBW_CB+1)                        ;set source address
                ld    de, #DATA_ADDR                        ;set target address
                ldir                                        ;copy data
                ret


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_mem_out
; DESCRIPTION   write <bc> bytes to memory address
; INPUT         bc = data size
;               CB[2]:CB[1] = address
;               DATA[0..bc-1]
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_mem_out:    ld    hl, #DATA_ADDR                        ;set source address
                ld    de, (CBW_CB+1)                        ;set target address
                ldir                                        ;copy data
                ret


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_mem_outb
; DESCRIPTION   write one byte byte to memory address
; INPUT         CB[2]:CBW[1] = address
;               CB[3] = data
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_mem_outb:   ld    hl, (CBW_CB+1)                        ;set target address
                ld    a, (CBW_CB+3)                         ;get data
                ld    (hl), a                               ;write data
                ret


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_exec_out
; DESCRIPTION   upload code and call cmd_exec
; INPUT         bc = data size
;               CB[2]:CB[1] = address
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_exec_out:   call  cmd_mem_out
               ;jr    cmd_exec


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_exec
; DESCRIPTION   call address and wait for return
; INPUT         CB[2]:CBW[1] = address
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_exec:       ld    de, (CBW_CB+1)                        ;put address on call stack
                push  de
                ret                                         ;go...


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_reset
; DESCRIPTION   reset device
; INPUT         n/a
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_reset:      ld    a, #0x88                              ;reenable watchdog
                out   (WATCHDOG_REG), a
                jr    .                                     ;wait for reset (max. 176 ms)


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_dma_in
; DESCRIPTION   read <bc> bytes from memory address using fast dma transfer
; INPUT         bc = data size
;               CB[4]:CB[1] = dma source address
;               CB[5] = dma int. mem sel
;               CB[6] = dma mode
; OUTPUT        DATA[0..bc-1]
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_dma_in:     ld    hl, #CBW_CB+1                         ;set dma source address from CB[4]:CB[1]
                ld    a, (hl)
                inc   hl
                out   (DMA1_SRCADDR0_REG), a
                ld    a, (hl)
                inc   hl
                out   (DMA1_SRCADDR1_REG), a
                ld    a, (hl)
                inc   hl
                out   (DMA1_SRCADDR2_REG), a
                ld    a, (hl)
                inc   hl
                out   (DMA1_SRCADDR3_REG), a

                ld    a, (hl)                               ;set dma int. mem sel from CB[5]
                inc   hl
                out   (DMA1_SRCADDR4_REG), a

                ld    a, #<REL_DATA_ADDR                    ;set dma target address to REL_DATA_ADDR@ZRAM2
                out   (DMA1_DSTADDR0_REG), a
                ld    a, #>REL_DATA_ADDR
                out   (DMA1_DSTADDR1_REG), a
                xor   a
                out   (DMA1_DSTADDR2_REG), a
                ld    a, #0x40
                out   (DMA1_DSTADDR3_REG), a
                ld    a, #0x07
                out   (DMA1_DSTADDR4_REG), a

                jr    dma_tx


;------------------------------------------------------------------------------------------------------------------------------------------
; COMMAND       cmd_dma_out
; DESCRIPTION   write <bc> bytes to memory address using fast dma transfer
; INPUT         bc = data size
;               CB[4]:CB[1] = dma destination address
;               CB[5] = dma int. mem sel
;               CB[6] = dma mode
;               DATA[0..bc-1]
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
cmd_dma_out:    ld    a, #<REL_DATA_ADDR                    ;set dma source address to REL_DATA_ADDR@ZRAM2
                out   (DMA1_SRCADDR0_REG), a
                ld    a, #>REL_DATA_ADDR
                out   (DMA1_SRCADDR1_REG), a
                xor   a
                out   (DMA1_SRCADDR2_REG), a
                ld    a, #0x40
                out   (DMA1_SRCADDR3_REG), a
                ld    a, #0x07
                out   (DMA1_SRCADDR4_REG), a

                ld    hl, #CBW_CB+1                         ;set dma target address from CB[4]:CB[1]
                ld    a, (hl)
                inc   hl
                out   (DMA1_DSTADDR0_REG), a
                ld    a, (hl)
                inc   hl
                out   (DMA1_DSTADDR1_REG), a
                ld    a, (hl)
                inc   hl
                out   (DMA1_DSTADDR2_REG), a
                ld    a, (hl)
                inc   hl
                out   (DMA1_DSTADDR3_REG), a
                
                ld    a, (hl)                               ;set dma int. mem sel from CB[5]
                inc   hl
                out   (DMA1_DSTADDR4_REG), a

               ;jr    dma_tx


;------------------------------------------------------------------------------------------------------------------------------------------
; FUNCTION      dma_tx
; DESCRIPTION   start fast dma transfer and wait until process finished
; INPUT         hl -> dma mode (1 byte)
;               bc = data size
; OUTPUT        n/a
;------------------------------------------------------------------------------------------------------------------------------------------
dma_tx:         ld    a, (hl)                               ;set dma mode
                out   (DMA1_MODE_REG), a

                dec   bc                                    ;set dma byte count (size-1 must be odd)
                ld    a, c
                or    #1
                out   (DMA1_CNTR_LO_REG), a
                ld    a, b
                out   (DMA1_CNTR_HI_REG), a

                ld    a, #1                                 ;start dma memory transfer
                out   (DMA1_COMMAND_REG), a

dma_tx_lp:      in    a, (DMA1_COMMAND_REG)                 ;wait until dma transfer finished
                bit   0, a
                jr    nz, dma_tx_lp

                ret


;******************************************************************************************************************************************
; align size of code
;******************************************************************************************************************************************
                .org   STACK_ADDR
                .db    0
                .ascii "(c)wiRe"
