;=============================================================================
;ATJ2085 register definitions
;=============================================================================
;author: wiRe (mailto:wiRe@s1mp3.de)
;source: www.s1mp3.de
;last change: 08/12/2006
;=============================================================================


;-----------------------------------------------------------------------------
;MISC
;-----------------------------------------------------------------------------
MCU_CLKCTRL_REG1     = 0x00
MCU_CLKCTRL_REG2     = 0x70    ;bit 3
DMA_CLKCTRL_REG      = 0x70    ;bit 2-0
B1_MEMMAP_REG        = 0x70    ;bit 5-4 (def=11b)
B2_MEMMAP_REG        = 0x70    ;bit 5-4 (def=11b)
ZRAM2_MEMMAP_REG     = 0x70    ;bit 5-4 (def=11b)
EM_PAGE_LO_REG       = 0x01
EM_PAGE_HI_REG       = 0x02
EM_PAGE_INC          = 0x03    ;write only
EM_PAGE_DEC          = 0x03    ;write only
MCU_A15CTRL_REG      = 0x04
MROM_PAGE_REG        = 0x05
SRAM_PAGE_REG        = 0x05
URAM_PAGE_REG	     = 0x05


;-----------------------------------------------------------------------------
;DMA1 CNTRL
;-----------------------------------------------------------------------------
DMA1_SRCADDR0_REG    = 0x06
DMA1_SRCADDR1_REG    = 0x07
DMA1_SRCADDR2_REG    = 0x08
DMA1_SRCADDR3_REG    = 0x09
DMA1_SRCADDR4_REG    = 0x0a
DMA1_IPMSRC_REG      = 0x0a
DMA1_IDMSRC_REG      = 0x0a
DMA1_ZRAM2SRC_REG    = 0x0a

DMA1_DSTADDR0_REG    = 0x0b
DMA1_DSTADDR1_REG    = 0x0c
DMA1_DSTADDR2_REG    = 0x0d
DMA1_DSTADDR3_REG    = 0x0e
DMA1_DSTADDR4_REG    = 0x0f
DMA1_IPMDST_REG      = 0x0f
DMA1_IDMDST_REG      = 0x0f
DMA1_ZRAM2DST_REG    = 0x0f

DMA1_CNTR_LO_REG     = 0x10
DMA1_CNTR_HI_REG     = 0x11
DMA1_MODE_REG        = 0x12
DMA1_COMMAND_REG     = 0x13


;-----------------------------------------------------------------------------
;DMA2 CNTRL
;-----------------------------------------------------------------------------
DMA2_SRCADDR0_REG    = 0x14
DMA2_SRCADDR1_REG    = 0x15
DMA2_SRCADDR2_REG    = 0x16
DMA2_SRCADDR3_REG    = 0x17
DMA2_SRCADDR4_REG    = 0x18
DMA2_IPMSRC_REG      = 0x18
DMA2_IDMSRC_REG      = 0x18
DMA2_ZRAM2SRC_REG    = 0x18

DMA2_DSTADDR0_REG    = 0x19
DMA2_DSTADDR1_REG    = 0x1a
DMA2_DSTADDR2_REG    = 0x1b
DMA2_DSTADDR3_REG    = 0x1c
DMA2_DSTADDR4_REG    = 0x1d
DMA2_IPMDST_REG      = 0x1d
DMA2_IDMDST_REG      = 0x1d
DMA2_ZRAM2DST_REG    = 0x1d

DMA2_CNTR_LO_REG     = 0x1e
DMA2_CNTR_HI_REG     = 0x1f
DMA2_MODE_REG        = 0x20
DMA2_COMMAND_REG     = 0x21


;-----------------------------------------------------------------------------
;CTC1
;-----------------------------------------------------------------------------
CTC1_PRESCALE_REG    = 0x22
CTC1_PERIOD_LO_REG   = 0x23
CTC1_PERIOD_HI_REG   = 0x24


;-----------------------------------------------------------------------------
;IRQ STATUS
;-----------------------------------------------------------------------------
IRQ_STATUS_REG       = 0x25    ;write 1 to any bit will clear tis one


;-----------------------------------------------------------------------------
;MASTER INTERRUPT CONTROL
;-----------------------------------------------------------------------------
MINT_STATUS_REG      = 0x26
MINT_ENABLE_REG      = 0x27


;-----------------------------------------------------------------------------
;DSP CONTROL
;-----------------------------------------------------------------------------
DSP_CONTROL_REG      = 0x2e    ;def=01
DSP_STATUS_REG       = 0x2e    ;def=01
DSP_BOOTMODE_REG     = 0x2f    ;def=73
DSP_HIP_REG0         = 0x30
DSP_HIP_REG1         = 0x31
DSP_HIP_REG2         = 0x32
DSP_HIP_REG3         = 0x33
DSP_HIP_REG4         = 0x34
DSP_HIP_REG5         = 0x35
DSP_HIP_REG6         = 0x36
DSP_HIP_REG7         = 0x37


;-----------------------------------------------------------------------------
;USB INTERFACE
;-----------------------------------------------------------------------------
USB_RESCTRL_REG      = 0x3e    ;def=04
USB_STATUS_REG       = 0x50
USB_CONTROL_REG      = 0x50
USB_DEVADDR_REG      = 0x51
USB_INT_STATUS_REG   = 0x52
USB_EP_STATUS_REG    = 0x53
USB_FRAME_HI_REG     = 0x54    ;read only
USB_FRAME_LO_REG     = 0x55    ;read only
USB_RESERVED56_REG   = 0x56
USB_SETUP_STATUS_REG = 0x57
USB_SETUP_DATA0_REG  = 0x58    ;read only
USB_SETUP_DATA1_REG  = 0x59    ;read only
USB_SETUP_DATA2_REG  = 0x5a    ;read only
USB_SETUP_DATA3_REG  = 0x5b    ;read only
USB_SETUP_DATA4_REG  = 0x5c    ;read only
USB_SETUP_DATA5_REG  = 0x5d    ;read only
USB_SETUP_DATA6_REG  = 0x5e    ;read only
USB_SETUP_DATA7_REG  = 0x5f    ;read only
USB_EPI_REG          = 0x60
USB_EPI_MODE_REG     = 0x61    ;def=02
USB_EPI_ADDR_HI_REG  = 0x62
USB_EPI_ADDR_LO_REG  = 0x63
USB_EPI_CNTR_HI_REG  = 0x64
USB_EPI_CNTR_LO_REG  = 0x65
USB_EPI_MPS_REG      = 0x66    ;bit 5-0
USB_EPI_DTOGGLE_REG  = 0x66    ;bit 7,6
USB_RESERVED67_REG   = 0x67
USB_EPI_EPSBR_REG    = 0x68    ;read only
USB_EPI_EPSNS_REG    = 0x69
USB_EPI_EPSST_REG    = 0x6a
USB_RESERVED76_REG   = 0x76


;-----------------------------------------------------------------------------
;(SAMSUNG) NAND FLASH SMC
;-----------------------------------------------------------------------------
NAND_ENABLE_REG      = 0x28
SNAN_CEMODE_REG      = 0x29
SNAN_CMD_REG         = 0x2a
SNAN_CA_REG          = 0x2b    ;write only
SNAN_RA_REG          = 0x2c    ;write only
SNAN_BA_REG1         = 0x2d
SNAN_BA_REG2         = 0xec
NAND_ECCCTRL_REG     = 0xcc
NAND_ECC_REG0        = 0xcd
NAND_ECC_REG1        = 0xce    ;def=10
NAND_ECC_REG2        = 0xcf


;-----------------------------------------------------------------------------
;I2C INTERFACE
;-----------------------------------------------------------------------------
I2C_CONTROL_REG      = 0x7a
I2C_STATUS_REG       = 0x7b    ;read only
I2C_ADDR_REG         = 0x71
I2C_DATA_REG         = 0x7c


;-----------------------------------------------------------------------------
;KEY SCAN INTERFACE
;-----------------------------------------------------------------------------
KEYSCAN_DATA_REG     = 0xc0
KEYSCAN_CTRL_REG     = 0xc1


;-----------------------------------------------------------------------------
;GPIO AND MULTI FUNCTION CONFIGURATION
;-----------------------------------------------------------------------------
MFP_SELECT_REG       = 0xee    ;bit 7-5
GPIO_DATAOUT_REG     = 0xee    ;bit 4-0 (def=00001b)
GPIO_CONFIG_REG      = 0xef    ;bit clr
KEYIO_CONFIG_REG     = 0xef    ;bit set
GPIO_BO_ENABLE_REG   = 0xf0
GPIO_BI_ENABLE_REG   = 0xf1
GPIO_B_DATA_REG      = 0xf2
GPIO_C_ENABLE_REG    = 0xf3    ;def=07
GPIO_C_DATA_REG      = 0xf4    ;def=03
GPIO_G_ENABLE_REG    = 0xfe
GPIO_G_DATA_REG      = 0xff


;-----------------------------------------------------------------------------
;RTC/LOSC/WATCHDOG
;-----------------------------------------------------------------------------
RTC_CONTROL_REG      = 0x43    ;def=04
RTC_IRQSTATUS_REG    = 0x44
RTC_TIME_LO_REG      = 0x45    ;read only
RTC_TIME_MI_REG      = 0x46    ;read only
RTC_TIME_HI_REG      = 0x47    ;read only
RTC_ALARM_LO_REG     = 0x48    ;def=22
RTC_ALARM_MI_REG     = 0x49    ;def=22
RTC_ALARM_HI_REG     = 0x4a    ;def=22

LOSC_DIV_LO_REG      = 0x4b    ;def=22
LOSC_DIV_MI_REG      = 0x4c    ;def=22
LOSC_DIV_HI_REG      = 0x4d    ;def=22

WATCHDOG_REG         = 0x4e    ;def=22


;-----------------------------------------------------------------------------
;HOSC/PLL
;-----------------------------------------------------------------------------
HFC_CONTROL_REG      = 0x40    ;def=04
PLL_PERFORMANCE_REG  = 0x41    ;def=55
PLL_CONTROL_REG      = 0x42


;-----------------------------------------------------------------------------
;PMU/DC-DC
;-----------------------------------------------------------------------------
VOLTAGE_CONTROL_REG  = 0x3f
DCDC_CONTROL_REG     = 0x4f    ;def=0ec
BATTERY_MON_REG      = 0xd0    ;def=0101b
POWER_CONTROL_REG    = 0xdf    ;def=28


;-----------------------------------------------------------------------------
;ADC/DAC/HEADPHONE
;-----------------------------------------------------------------------------
DAC_CONTROL_REG1     = 0x80    ;def=03
DAC_RATECTRL_REG     = 0x81
DAC_PCMOUT_LO_REG    = 0x88    ;write only
DAC_PCMOUT_MI_REG    = 0x89    ;write only
DAC_PCMOUT_HI_REG    = 0x8a    ;write only
FIFO_STATUS_REG      = 0x8b    ;def=15
DAC_CONTROL_REG2     = 0x91    ;def=07


;-----------------------------------------------------------------------------
;DAC ANALOG
;-----------------------------------------------------------------------------
DACA_VOLCTRL_REG     = 0x9e    ;def=40
DACA_BLKCTRL_REG     = 0xdd


;-----------------------------------------------------------------------------
;ADC
;-----------------------------------------------------------------------------
ADC_DATA_REG         = 0x9a    ;read only
SARADC_CONTROL_REG   = 0xd1    ;def=04
ADC_PERFORMANCE_REG  = 0xd3    ;def=03
ADC_MODULEPERF_REG   = 0x9f    ;def=5d
ADC_CONTROL_REG1     = 0xd4    ;def=05
ADC_CONTROL_REG2     = 0xd5
ADC_GAINCTRL_REG     = 0xd6    ;def=55
ADC_FIFOCTRL_REG     = 0xd7
ADC_BUSCTRL_REG      = 0x9b
LRADC_DATA_REG       = 0xd8    ;read only
SARADC_IRQCTRL_REG   = 0xde    ;write (def=80)
SARADC_IRQSTAT_REG   = 0xde    ;read (def=80)


;=============================================================================
;EOF
;=============================================================================
