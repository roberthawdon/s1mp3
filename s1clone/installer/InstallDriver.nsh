;==================================================================================================
;
; U p d a t e   D r i v e r
;
; Written by Kuba Ober
; Copyright (c) 2004 Kuba Ober
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.
;
;--------------------------------------------------------------------------------------------------
;
; U S A G E
;
; Push "c:\program files\yoursoftware\driver"
;  -- the directory of the .inf file
; Push "c:\program files\yoursoftware\driver\driver.inf"
;  -- the filepath of the .inf file (directory + filename)
; Call InstallDriver
;
; Your driver (minimally the .inf and .sys files) should already by installed
; by your NSIS script.
;
; Typically, you would put the driver either in $INSTDIR or $INSTDIR\Driver
; It's up to you, of course.
;
; The driver (i.e. .inf, .sys and related files) must be present for the
; lifetime of your application, you shouldn't remove them after calling
; this function!
;
; You DON'T want to put the driver in any of system directories. Windows
; will do it when the device is first plugged in.
;
;==================================================================================================


;BOOL SetupCopyOEMInf(PSTR, PSTR, DWORD, DWORD, PSTR, DWORD, PDWORD, PSTR);
!define sysSetupCopyOEMInf "setupapi::SetupCopyOEMInf(t, t, i, i, i, i, *i, t) i"
!define SPOST_NONE 0
!define SPOST_PATH 1
!define SPOST_URL 2
!define SP_COPY_DELETESOURCE 0x1
!define SP_COPY_REPLACEONLY 0x2
!define SP_COPY_NOOVERWRITE 0x8
!define SP_COPY_OEMINF_CATALOG_ONLY 0x40000


;--------------------------------------------------------------------------------------------------
Function InstallDriver

  Pop $R1 ; INFPATH
  Pop $R2 ; INFDIR

  DetailPrint "Pre-Installing driver..."

  System::Get '${sysSetupCopyOEMInf}'
  Pop $0
  StrCmp $0 'error' lbl_nopreinst

  ; INFPATH, INFDIR, SPOST_PATH, "", 0, 0, 0, 0
  System::Call '${sysSetupCopyOEMInf}?e (R1, R2, ${SPOST_PATH}, 0, 0, 0, 0, 0) .r0'
  Pop $1  ;get last error
  IntCmp $0 1 lbl_done  ;success?

  DetailPrint 'Driver pre-installation has failed. ($1)'
  Goto lbl_done

lbl_nopreinst:
  DetailPrint "Windows version doesn't support driver pre-installation."

lbl_done:
FunctionEnd
