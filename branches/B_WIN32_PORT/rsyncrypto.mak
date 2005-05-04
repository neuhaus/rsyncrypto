# Microsoft Developer Studio Generated NMAKE File, Based on rsyncrypto.dsp
!IF "$(CFG)" == ""
CFG=rsyncrypto - Win32 Debug
!MESSAGE No configuration specified. Defaulting to rsyncrypto - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "rsyncrypto - Win32 Release" && "$(CFG)" != "rsyncrypto - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rsyncrypto.mak" CFG="rsyncrypto - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rsyncrypto - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "rsyncrypto - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rsyncrypto - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\rsyncrypto.exe"


CLEAN :
	-@erase "$(INTDIR)\aes_crypt.obj"
	-@erase "$(INTDIR)\blocksizes.obj"
	-@erase "$(INTDIR)\crypt_key.obj"
	-@erase "$(INTDIR)\crypto.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\rsyncrypto.pch"
	-@erase "$(INTDIR)\rsyncrypto.res"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\rsyncrypto.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\rsyncrypto.pch" /Yu"rsyncrypto.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x40d /fo"$(INTDIR)\rsyncrypto.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\rsyncrypto.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\rsyncrypto.pdb" /machine:I386 /out:"$(OUTDIR)\rsyncrypto.exe" 
LINK32_OBJS= \
	"$(INTDIR)\blocksizes.obj" \
	"$(INTDIR)\crypt_key.obj" \
	"$(INTDIR)\crypto.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\aes_crypt.obj" \
	"$(INTDIR)\rsyncrypto.res" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\rsyncrypto.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "rsyncrypto - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\rsyncrypto.exe"


CLEAN :
	-@erase "$(INTDIR)\aes_crypt.obj"
	-@erase "$(INTDIR)\blocksizes.obj"
	-@erase "$(INTDIR)\crypt_key.obj"
	-@erase "$(INTDIR)\crypto.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\rsyncrypto.pch"
	-@erase "$(INTDIR)\rsyncrypto.res"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\rsyncrypto.exe"
	-@erase "$(OUTDIR)\rsyncrypto.ilk"
	-@erase "$(OUTDIR)\rsyncrypto.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\rsyncrypto.pch" /Yu"rsyncrypto.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
RSC_PROJ=/l 0x40d /fo"$(INTDIR)\rsyncrypto.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\rsyncrypto.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\rsyncrypto.pdb" /debug /machine:I386 /out:"$(OUTDIR)\rsyncrypto.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\blocksizes.obj" \
	"$(INTDIR)\crypt_key.obj" \
	"$(INTDIR)\crypto.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\aes_crypt.obj" \
	"$(INTDIR)\rsyncrypto.res" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\rsyncrypto.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("rsyncrypto.dep")
!INCLUDE "rsyncrypto.dep"
!ELSE 
!MESSAGE Warning: cannot find "rsyncrypto.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "rsyncrypto - Win32 Release" || "$(CFG)" == "rsyncrypto - Win32 Debug"
SOURCE=.\aes_crypt.cpp

"$(INTDIR)\aes_crypt.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\rsyncrypto.pch"


SOURCE=.\blocksizes.cpp

"$(INTDIR)\blocksizes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\rsyncrypto.pch"


SOURCE=.\crypt_key.cpp

"$(INTDIR)\crypt_key.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\rsyncrypto.pch"


SOURCE=.\crypto.cpp

"$(INTDIR)\crypto.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\rsyncrypto.pch"


SOURCE=.\file.cpp

"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\rsyncrypto.pch"


SOURCE=.\main.cpp

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\rsyncrypto.pch"


SOURCE=.\win32\rsyncrypto.rc

!IF  "$(CFG)" == "rsyncrypto - Win32 Release"


"$(INTDIR)\rsyncrypto.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x40d /fo"$(INTDIR)\rsyncrypto.res" /i "win32" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "rsyncrypto - Win32 Debug"


"$(INTDIR)\rsyncrypto.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x40d /fo"$(INTDIR)\rsyncrypto.res" /i "win32" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\win32\stdafx.cpp

!IF  "$(CFG)" == "rsyncrypto - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\rsyncrypto.pch" /Yc"rsyncrypto.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\rsyncrypto.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "rsyncrypto - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\rsyncrypto.pch" /Yc"rsyncrypto.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\rsyncrypto.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

