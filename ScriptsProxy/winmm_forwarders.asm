option casemap:none

EXTERN g_winmmFunctions:QWORD

.code
PUBLIC fCloseDriver
fCloseDriver PROC
    jmp QWORD PTR [g_winmmFunctions + 0]
fCloseDriver ENDP

PUBLIC fDefDriverProc
fDefDriverProc PROC
    jmp QWORD PTR [g_winmmFunctions + 8]
fDefDriverProc ENDP

PUBLIC fDriverCallback
fDriverCallback PROC
    jmp QWORD PTR [g_winmmFunctions + 16]
fDriverCallback ENDP

PUBLIC fDrvGetModuleHandle
fDrvGetModuleHandle PROC
    jmp QWORD PTR [g_winmmFunctions + 24]
fDrvGetModuleHandle ENDP

PUBLIC fGetDriverModuleHandle
fGetDriverModuleHandle PROC
    jmp QWORD PTR [g_winmmFunctions + 32]
fGetDriverModuleHandle ENDP

PUBLIC fOpenDriver
fOpenDriver PROC
    jmp QWORD PTR [g_winmmFunctions + 40]
fOpenDriver ENDP

PUBLIC fPlaySound
fPlaySound PROC
    jmp QWORD PTR [g_winmmFunctions + 48]
fPlaySound ENDP

PUBLIC fPlaySoundA
fPlaySoundA PROC
    jmp QWORD PTR [g_winmmFunctions + 56]
fPlaySoundA ENDP

PUBLIC fPlaySoundW
fPlaySoundW PROC
    jmp QWORD PTR [g_winmmFunctions + 64]
fPlaySoundW ENDP

PUBLIC fSendDriverMessage
fSendDriverMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 72]
fSendDriverMessage ENDP

PUBLIC fWOWAppExit
fWOWAppExit PROC
    jmp QWORD PTR [g_winmmFunctions + 80]
fWOWAppExit ENDP

PUBLIC fauxGetDevCapsA
fauxGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 88]
fauxGetDevCapsA ENDP

PUBLIC fauxGetDevCapsW
fauxGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 96]
fauxGetDevCapsW ENDP

PUBLIC fauxGetNumDevs
fauxGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 104]
fauxGetNumDevs ENDP

PUBLIC fauxGetVolume
fauxGetVolume PROC
    jmp QWORD PTR [g_winmmFunctions + 112]
fauxGetVolume ENDP

PUBLIC fauxOutMessage
fauxOutMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 120]
fauxOutMessage ENDP

PUBLIC fauxSetVolume
fauxSetVolume PROC
    jmp QWORD PTR [g_winmmFunctions + 128]
fauxSetVolume ENDP

PUBLIC fjoyConfigChanged
fjoyConfigChanged PROC
    jmp QWORD PTR [g_winmmFunctions + 136]
fjoyConfigChanged ENDP

PUBLIC fjoyGetDevCapsA
fjoyGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 144]
fjoyGetDevCapsA ENDP

PUBLIC fjoyGetDevCapsW
fjoyGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 152]
fjoyGetDevCapsW ENDP

PUBLIC fjoyGetNumDevs
fjoyGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 160]
fjoyGetNumDevs ENDP

PUBLIC fjoyGetPos
fjoyGetPos PROC
    jmp QWORD PTR [g_winmmFunctions + 168]
fjoyGetPos ENDP

PUBLIC fjoyGetPosEx
fjoyGetPosEx PROC
    jmp QWORD PTR [g_winmmFunctions + 176]
fjoyGetPosEx ENDP

PUBLIC fjoyGetThreshold
fjoyGetThreshold PROC
    jmp QWORD PTR [g_winmmFunctions + 184]
fjoyGetThreshold ENDP

PUBLIC fjoyReleaseCapture
fjoyReleaseCapture PROC
    jmp QWORD PTR [g_winmmFunctions + 192]
fjoyReleaseCapture ENDP

PUBLIC fjoySetCapture
fjoySetCapture PROC
    jmp QWORD PTR [g_winmmFunctions + 200]
fjoySetCapture ENDP

PUBLIC fjoySetThreshold
fjoySetThreshold PROC
    jmp QWORD PTR [g_winmmFunctions + 208]
fjoySetThreshold ENDP

PUBLIC fmciDriverNotify
fmciDriverNotify PROC
    jmp QWORD PTR [g_winmmFunctions + 216]
fmciDriverNotify ENDP

PUBLIC fmciDriverYield
fmciDriverYield PROC
    jmp QWORD PTR [g_winmmFunctions + 224]
fmciDriverYield ENDP

PUBLIC fmciExecute
fmciExecute PROC
    jmp QWORD PTR [g_winmmFunctions + 232]
fmciExecute ENDP

PUBLIC fmciFreeCommandResource
fmciFreeCommandResource PROC
    jmp QWORD PTR [g_winmmFunctions + 240]
fmciFreeCommandResource ENDP

PUBLIC fmciGetCreatorTask
fmciGetCreatorTask PROC
    jmp QWORD PTR [g_winmmFunctions + 248]
fmciGetCreatorTask ENDP

PUBLIC fmciGetDeviceIDA
fmciGetDeviceIDA PROC
    jmp QWORD PTR [g_winmmFunctions + 256]
fmciGetDeviceIDA ENDP

PUBLIC fmciGetDeviceIDFromElementIDA
fmciGetDeviceIDFromElementIDA PROC
    jmp QWORD PTR [g_winmmFunctions + 264]
fmciGetDeviceIDFromElementIDA ENDP

PUBLIC fmciGetDeviceIDFromElementIDW
fmciGetDeviceIDFromElementIDW PROC
    jmp QWORD PTR [g_winmmFunctions + 272]
fmciGetDeviceIDFromElementIDW ENDP

PUBLIC fmciGetDeviceIDW
fmciGetDeviceIDW PROC
    jmp QWORD PTR [g_winmmFunctions + 280]
fmciGetDeviceIDW ENDP

PUBLIC fmciGetDriverData
fmciGetDriverData PROC
    jmp QWORD PTR [g_winmmFunctions + 288]
fmciGetDriverData ENDP

PUBLIC fmciGetErrorStringA
fmciGetErrorStringA PROC
    jmp QWORD PTR [g_winmmFunctions + 296]
fmciGetErrorStringA ENDP

PUBLIC fmciGetErrorStringW
fmciGetErrorStringW PROC
    jmp QWORD PTR [g_winmmFunctions + 304]
fmciGetErrorStringW ENDP

PUBLIC fmciGetYieldProc
fmciGetYieldProc PROC
    jmp QWORD PTR [g_winmmFunctions + 312]
fmciGetYieldProc ENDP

PUBLIC fmciLoadCommandResource
fmciLoadCommandResource PROC
    jmp QWORD PTR [g_winmmFunctions + 320]
fmciLoadCommandResource ENDP

PUBLIC fmciSendCommandA
fmciSendCommandA PROC
    jmp QWORD PTR [g_winmmFunctions + 328]
fmciSendCommandA ENDP

PUBLIC fmciSendCommandW
fmciSendCommandW PROC
    jmp QWORD PTR [g_winmmFunctions + 336]
fmciSendCommandW ENDP

PUBLIC fmciSendStringA
fmciSendStringA PROC
    jmp QWORD PTR [g_winmmFunctions + 344]
fmciSendStringA ENDP

PUBLIC fmciSendStringW
fmciSendStringW PROC
    jmp QWORD PTR [g_winmmFunctions + 352]
fmciSendStringW ENDP

PUBLIC fmciSetDriverData
fmciSetDriverData PROC
    jmp QWORD PTR [g_winmmFunctions + 360]
fmciSetDriverData ENDP

PUBLIC fmciSetYieldProc
fmciSetYieldProc PROC
    jmp QWORD PTR [g_winmmFunctions + 368]
fmciSetYieldProc ENDP

PUBLIC fmidiConnect
fmidiConnect PROC
    jmp QWORD PTR [g_winmmFunctions + 376]
fmidiConnect ENDP

PUBLIC fmidiDisconnect
fmidiDisconnect PROC
    jmp QWORD PTR [g_winmmFunctions + 384]
fmidiDisconnect ENDP

PUBLIC fmidiInAddBuffer
fmidiInAddBuffer PROC
    jmp QWORD PTR [g_winmmFunctions + 392]
fmidiInAddBuffer ENDP

PUBLIC fmidiInClose
fmidiInClose PROC
    jmp QWORD PTR [g_winmmFunctions + 400]
fmidiInClose ENDP

PUBLIC fmidiInGetDevCapsA
fmidiInGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 408]
fmidiInGetDevCapsA ENDP

PUBLIC fmidiInGetDevCapsW
fmidiInGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 416]
fmidiInGetDevCapsW ENDP

PUBLIC fmidiInGetErrorTextA
fmidiInGetErrorTextA PROC
    jmp QWORD PTR [g_winmmFunctions + 424]
fmidiInGetErrorTextA ENDP

PUBLIC fmidiInGetErrorTextW
fmidiInGetErrorTextW PROC
    jmp QWORD PTR [g_winmmFunctions + 432]
fmidiInGetErrorTextW ENDP

PUBLIC fmidiInGetID
fmidiInGetID PROC
    jmp QWORD PTR [g_winmmFunctions + 440]
fmidiInGetID ENDP

PUBLIC fmidiInGetNumDevs
fmidiInGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 448]
fmidiInGetNumDevs ENDP

PUBLIC fmidiInMessage
fmidiInMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 456]
fmidiInMessage ENDP

PUBLIC fmidiInOpen
fmidiInOpen PROC
    jmp QWORD PTR [g_winmmFunctions + 464]
fmidiInOpen ENDP

PUBLIC fmidiInPrepareHeader
fmidiInPrepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 472]
fmidiInPrepareHeader ENDP

PUBLIC fmidiInReset
fmidiInReset PROC
    jmp QWORD PTR [g_winmmFunctions + 480]
fmidiInReset ENDP

PUBLIC fmidiInStart
fmidiInStart PROC
    jmp QWORD PTR [g_winmmFunctions + 488]
fmidiInStart ENDP

PUBLIC fmidiInStop
fmidiInStop PROC
    jmp QWORD PTR [g_winmmFunctions + 496]
fmidiInStop ENDP

PUBLIC fmidiInUnprepareHeader
fmidiInUnprepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 504]
fmidiInUnprepareHeader ENDP

PUBLIC fmidiOutCacheDrumPatches
fmidiOutCacheDrumPatches PROC
    jmp QWORD PTR [g_winmmFunctions + 512]
fmidiOutCacheDrumPatches ENDP

PUBLIC fmidiOutCachePatches
fmidiOutCachePatches PROC
    jmp QWORD PTR [g_winmmFunctions + 520]
fmidiOutCachePatches ENDP

PUBLIC fmidiOutClose
fmidiOutClose PROC
    jmp QWORD PTR [g_winmmFunctions + 528]
fmidiOutClose ENDP

PUBLIC fmidiOutGetDevCapsA
fmidiOutGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 536]
fmidiOutGetDevCapsA ENDP

PUBLIC fmidiOutGetDevCapsW
fmidiOutGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 544]
fmidiOutGetDevCapsW ENDP

PUBLIC fmidiOutGetErrorTextA
fmidiOutGetErrorTextA PROC
    jmp QWORD PTR [g_winmmFunctions + 552]
fmidiOutGetErrorTextA ENDP

PUBLIC fmidiOutGetErrorTextW
fmidiOutGetErrorTextW PROC
    jmp QWORD PTR [g_winmmFunctions + 560]
fmidiOutGetErrorTextW ENDP

PUBLIC fmidiOutGetID
fmidiOutGetID PROC
    jmp QWORD PTR [g_winmmFunctions + 568]
fmidiOutGetID ENDP

PUBLIC fmidiOutGetNumDevs
fmidiOutGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 576]
fmidiOutGetNumDevs ENDP

PUBLIC fmidiOutGetVolume
fmidiOutGetVolume PROC
    jmp QWORD PTR [g_winmmFunctions + 584]
fmidiOutGetVolume ENDP

PUBLIC fmidiOutLongMsg
fmidiOutLongMsg PROC
    jmp QWORD PTR [g_winmmFunctions + 592]
fmidiOutLongMsg ENDP

PUBLIC fmidiOutMessage
fmidiOutMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 600]
fmidiOutMessage ENDP

PUBLIC fmidiOutOpen
fmidiOutOpen PROC
    jmp QWORD PTR [g_winmmFunctions + 608]
fmidiOutOpen ENDP

PUBLIC fmidiOutPrepareHeader
fmidiOutPrepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 616]
fmidiOutPrepareHeader ENDP

PUBLIC fmidiOutReset
fmidiOutReset PROC
    jmp QWORD PTR [g_winmmFunctions + 624]
fmidiOutReset ENDP

PUBLIC fmidiOutSetVolume
fmidiOutSetVolume PROC
    jmp QWORD PTR [g_winmmFunctions + 632]
fmidiOutSetVolume ENDP

PUBLIC fmidiOutShortMsg
fmidiOutShortMsg PROC
    jmp QWORD PTR [g_winmmFunctions + 640]
fmidiOutShortMsg ENDP

PUBLIC fmidiOutUnprepareHeader
fmidiOutUnprepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 648]
fmidiOutUnprepareHeader ENDP

PUBLIC fmidiStreamClose
fmidiStreamClose PROC
    jmp QWORD PTR [g_winmmFunctions + 656]
fmidiStreamClose ENDP

PUBLIC fmidiStreamOpen
fmidiStreamOpen PROC
    jmp QWORD PTR [g_winmmFunctions + 664]
fmidiStreamOpen ENDP

PUBLIC fmidiStreamOut
fmidiStreamOut PROC
    jmp QWORD PTR [g_winmmFunctions + 672]
fmidiStreamOut ENDP

PUBLIC fmidiStreamPause
fmidiStreamPause PROC
    jmp QWORD PTR [g_winmmFunctions + 680]
fmidiStreamPause ENDP

PUBLIC fmidiStreamPosition
fmidiStreamPosition PROC
    jmp QWORD PTR [g_winmmFunctions + 688]
fmidiStreamPosition ENDP

PUBLIC fmidiStreamProperty
fmidiStreamProperty PROC
    jmp QWORD PTR [g_winmmFunctions + 696]
fmidiStreamProperty ENDP

PUBLIC fmidiStreamRestart
fmidiStreamRestart PROC
    jmp QWORD PTR [g_winmmFunctions + 704]
fmidiStreamRestart ENDP

PUBLIC fmidiStreamStop
fmidiStreamStop PROC
    jmp QWORD PTR [g_winmmFunctions + 712]
fmidiStreamStop ENDP

PUBLIC fmixerClose
fmixerClose PROC
    jmp QWORD PTR [g_winmmFunctions + 720]
fmixerClose ENDP

PUBLIC fmixerGetControlDetailsA
fmixerGetControlDetailsA PROC
    jmp QWORD PTR [g_winmmFunctions + 728]
fmixerGetControlDetailsA ENDP

PUBLIC fmixerGetControlDetailsW
fmixerGetControlDetailsW PROC
    jmp QWORD PTR [g_winmmFunctions + 736]
fmixerGetControlDetailsW ENDP

PUBLIC fmixerGetDevCapsA
fmixerGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 744]
fmixerGetDevCapsA ENDP

PUBLIC fmixerGetDevCapsW
fmixerGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 752]
fmixerGetDevCapsW ENDP

PUBLIC fmixerGetID
fmixerGetID PROC
    jmp QWORD PTR [g_winmmFunctions + 760]
fmixerGetID ENDP

PUBLIC fmixerGetLineControlsA
fmixerGetLineControlsA PROC
    jmp QWORD PTR [g_winmmFunctions + 768]
fmixerGetLineControlsA ENDP

PUBLIC fmixerGetLineControlsW
fmixerGetLineControlsW PROC
    jmp QWORD PTR [g_winmmFunctions + 776]
fmixerGetLineControlsW ENDP

PUBLIC fmixerGetLineInfoA
fmixerGetLineInfoA PROC
    jmp QWORD PTR [g_winmmFunctions + 784]
fmixerGetLineInfoA ENDP

PUBLIC fmixerGetLineInfoW
fmixerGetLineInfoW PROC
    jmp QWORD PTR [g_winmmFunctions + 792]
fmixerGetLineInfoW ENDP

PUBLIC fmixerGetNumDevs
fmixerGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 800]
fmixerGetNumDevs ENDP

PUBLIC fmixerMessage
fmixerMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 808]
fmixerMessage ENDP

PUBLIC fmixerOpen
fmixerOpen PROC
    jmp QWORD PTR [g_winmmFunctions + 816]
fmixerOpen ENDP

PUBLIC fmixerSetControlDetails
fmixerSetControlDetails PROC
    jmp QWORD PTR [g_winmmFunctions + 824]
fmixerSetControlDetails ENDP

PUBLIC fmmDrvInstall
fmmDrvInstall PROC
    jmp QWORD PTR [g_winmmFunctions + 832]
fmmDrvInstall ENDP

PUBLIC fmmGetCurrentTask
fmmGetCurrentTask PROC
    jmp QWORD PTR [g_winmmFunctions + 840]
fmmGetCurrentTask ENDP

PUBLIC fmmTaskBlock
fmmTaskBlock PROC
    jmp QWORD PTR [g_winmmFunctions + 848]
fmmTaskBlock ENDP

PUBLIC fmmTaskCreate
fmmTaskCreate PROC
    jmp QWORD PTR [g_winmmFunctions + 856]
fmmTaskCreate ENDP

PUBLIC fmmTaskSignal
fmmTaskSignal PROC
    jmp QWORD PTR [g_winmmFunctions + 864]
fmmTaskSignal ENDP

PUBLIC fmmTaskYield
fmmTaskYield PROC
    jmp QWORD PTR [g_winmmFunctions + 872]
fmmTaskYield ENDP

PUBLIC fmmioAdvance
fmmioAdvance PROC
    jmp QWORD PTR [g_winmmFunctions + 880]
fmmioAdvance ENDP

PUBLIC fmmioAscend
fmmioAscend PROC
    jmp QWORD PTR [g_winmmFunctions + 888]
fmmioAscend ENDP

PUBLIC fmmioClose
fmmioClose PROC
    jmp QWORD PTR [g_winmmFunctions + 896]
fmmioClose ENDP

PUBLIC fmmioCreateChunk
fmmioCreateChunk PROC
    jmp QWORD PTR [g_winmmFunctions + 904]
fmmioCreateChunk ENDP

PUBLIC fmmioDescend
fmmioDescend PROC
    jmp QWORD PTR [g_winmmFunctions + 912]
fmmioDescend ENDP

PUBLIC fmmioFlush
fmmioFlush PROC
    jmp QWORD PTR [g_winmmFunctions + 920]
fmmioFlush ENDP

PUBLIC fmmioGetInfo
fmmioGetInfo PROC
    jmp QWORD PTR [g_winmmFunctions + 928]
fmmioGetInfo ENDP

PUBLIC fmmioInstallIOProcA
fmmioInstallIOProcA PROC
    jmp QWORD PTR [g_winmmFunctions + 936]
fmmioInstallIOProcA ENDP

PUBLIC fmmioInstallIOProcW
fmmioInstallIOProcW PROC
    jmp QWORD PTR [g_winmmFunctions + 944]
fmmioInstallIOProcW ENDP

PUBLIC fmmioOpenA
fmmioOpenA PROC
    jmp QWORD PTR [g_winmmFunctions + 952]
fmmioOpenA ENDP

PUBLIC fmmioOpenW
fmmioOpenW PROC
    jmp QWORD PTR [g_winmmFunctions + 960]
fmmioOpenW ENDP

PUBLIC fmmioRead
fmmioRead PROC
    jmp QWORD PTR [g_winmmFunctions + 968]
fmmioRead ENDP

PUBLIC fmmioRenameA
fmmioRenameA PROC
    jmp QWORD PTR [g_winmmFunctions + 976]
fmmioRenameA ENDP

PUBLIC fmmioRenameW
fmmioRenameW PROC
    jmp QWORD PTR [g_winmmFunctions + 984]
fmmioRenameW ENDP

PUBLIC fmmioSeek
fmmioSeek PROC
    jmp QWORD PTR [g_winmmFunctions + 992]
fmmioSeek ENDP

PUBLIC fmmioSendMessage
fmmioSendMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 1000]
fmmioSendMessage ENDP

PUBLIC fmmioSetBuffer
fmmioSetBuffer PROC
    jmp QWORD PTR [g_winmmFunctions + 1008]
fmmioSetBuffer ENDP

PUBLIC fmmioSetInfo
fmmioSetInfo PROC
    jmp QWORD PTR [g_winmmFunctions + 1016]
fmmioSetInfo ENDP

PUBLIC fmmioStringToFOURCCA
fmmioStringToFOURCCA PROC
    jmp QWORD PTR [g_winmmFunctions + 1024]
fmmioStringToFOURCCA ENDP

PUBLIC fmmioStringToFOURCCW
fmmioStringToFOURCCW PROC
    jmp QWORD PTR [g_winmmFunctions + 1032]
fmmioStringToFOURCCW ENDP

PUBLIC fmmioWrite
fmmioWrite PROC
    jmp QWORD PTR [g_winmmFunctions + 1040]
fmmioWrite ENDP

PUBLIC fmmsystemGetVersion
fmmsystemGetVersion PROC
    jmp QWORD PTR [g_winmmFunctions + 1048]
fmmsystemGetVersion ENDP

PUBLIC fsndPlaySoundA
fsndPlaySoundA PROC
    jmp QWORD PTR [g_winmmFunctions + 1056]
fsndPlaySoundA ENDP

PUBLIC fsndPlaySoundW
fsndPlaySoundW PROC
    jmp QWORD PTR [g_winmmFunctions + 1064]
fsndPlaySoundW ENDP

PUBLIC ftimeBeginPeriod
ftimeBeginPeriod PROC
    jmp QWORD PTR [g_winmmFunctions + 1072]
ftimeBeginPeriod ENDP

PUBLIC ftimeEndPeriod
ftimeEndPeriod PROC
    jmp QWORD PTR [g_winmmFunctions + 1080]
ftimeEndPeriod ENDP

PUBLIC ftimeGetDevCaps
ftimeGetDevCaps PROC
    jmp QWORD PTR [g_winmmFunctions + 1088]
ftimeGetDevCaps ENDP

PUBLIC ftimeGetSystemTime
ftimeGetSystemTime PROC
    jmp QWORD PTR [g_winmmFunctions + 1096]
ftimeGetSystemTime ENDP

PUBLIC ftimeGetTime
ftimeGetTime PROC
    jmp QWORD PTR [g_winmmFunctions + 1104]
ftimeGetTime ENDP

PUBLIC ftimeKillEvent
ftimeKillEvent PROC
    jmp QWORD PTR [g_winmmFunctions + 1112]
ftimeKillEvent ENDP

PUBLIC ftimeSetEvent
ftimeSetEvent PROC
    jmp QWORD PTR [g_winmmFunctions + 1120]
ftimeSetEvent ENDP

PUBLIC fwaveInAddBuffer
fwaveInAddBuffer PROC
    jmp QWORD PTR [g_winmmFunctions + 1128]
fwaveInAddBuffer ENDP

PUBLIC fwaveInClose
fwaveInClose PROC
    jmp QWORD PTR [g_winmmFunctions + 1136]
fwaveInClose ENDP

PUBLIC fwaveInGetDevCapsA
fwaveInGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 1144]
fwaveInGetDevCapsA ENDP

PUBLIC fwaveInGetDevCapsW
fwaveInGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 1152]
fwaveInGetDevCapsW ENDP

PUBLIC fwaveInGetErrorTextA
fwaveInGetErrorTextA PROC
    jmp QWORD PTR [g_winmmFunctions + 1160]
fwaveInGetErrorTextA ENDP

PUBLIC fwaveInGetErrorTextW
fwaveInGetErrorTextW PROC
    jmp QWORD PTR [g_winmmFunctions + 1168]
fwaveInGetErrorTextW ENDP

PUBLIC fwaveInGetID
fwaveInGetID PROC
    jmp QWORD PTR [g_winmmFunctions + 1176]
fwaveInGetID ENDP

PUBLIC fwaveInGetNumDevs
fwaveInGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 1184]
fwaveInGetNumDevs ENDP

PUBLIC fwaveInGetPosition
fwaveInGetPosition PROC
    jmp QWORD PTR [g_winmmFunctions + 1192]
fwaveInGetPosition ENDP

PUBLIC fwaveInMessage
fwaveInMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 1200]
fwaveInMessage ENDP

PUBLIC fwaveInOpen
fwaveInOpen PROC
    jmp QWORD PTR [g_winmmFunctions + 1208]
fwaveInOpen ENDP

PUBLIC fwaveInPrepareHeader
fwaveInPrepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 1216]
fwaveInPrepareHeader ENDP

PUBLIC fwaveInReset
fwaveInReset PROC
    jmp QWORD PTR [g_winmmFunctions + 1224]
fwaveInReset ENDP

PUBLIC fwaveInStart
fwaveInStart PROC
    jmp QWORD PTR [g_winmmFunctions + 1232]
fwaveInStart ENDP

PUBLIC fwaveInStop
fwaveInStop PROC
    jmp QWORD PTR [g_winmmFunctions + 1240]
fwaveInStop ENDP

PUBLIC fwaveInUnprepareHeader
fwaveInUnprepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 1248]
fwaveInUnprepareHeader ENDP

PUBLIC fwaveOutBreakLoop
fwaveOutBreakLoop PROC
    jmp QWORD PTR [g_winmmFunctions + 1256]
fwaveOutBreakLoop ENDP

PUBLIC fwaveOutClose
fwaveOutClose PROC
    jmp QWORD PTR [g_winmmFunctions + 1264]
fwaveOutClose ENDP

PUBLIC fwaveOutGetDevCapsA
fwaveOutGetDevCapsA PROC
    jmp QWORD PTR [g_winmmFunctions + 1272]
fwaveOutGetDevCapsA ENDP

PUBLIC fwaveOutGetDevCapsW
fwaveOutGetDevCapsW PROC
    jmp QWORD PTR [g_winmmFunctions + 1280]
fwaveOutGetDevCapsW ENDP

PUBLIC fwaveOutGetErrorTextA
fwaveOutGetErrorTextA PROC
    jmp QWORD PTR [g_winmmFunctions + 1288]
fwaveOutGetErrorTextA ENDP

PUBLIC fwaveOutGetErrorTextW
fwaveOutGetErrorTextW PROC
    jmp QWORD PTR [g_winmmFunctions + 1296]
fwaveOutGetErrorTextW ENDP

PUBLIC fwaveOutGetID
fwaveOutGetID PROC
    jmp QWORD PTR [g_winmmFunctions + 1304]
fwaveOutGetID ENDP

PUBLIC fwaveOutGetNumDevs
fwaveOutGetNumDevs PROC
    jmp QWORD PTR [g_winmmFunctions + 1312]
fwaveOutGetNumDevs ENDP

PUBLIC fwaveOutGetPitch
fwaveOutGetPitch PROC
    jmp QWORD PTR [g_winmmFunctions + 1320]
fwaveOutGetPitch ENDP

PUBLIC fwaveOutGetPlaybackRate
fwaveOutGetPlaybackRate PROC
    jmp QWORD PTR [g_winmmFunctions + 1328]
fwaveOutGetPlaybackRate ENDP

PUBLIC fwaveOutGetPosition
fwaveOutGetPosition PROC
    jmp QWORD PTR [g_winmmFunctions + 1336]
fwaveOutGetPosition ENDP

PUBLIC fwaveOutGetVolume
fwaveOutGetVolume PROC
    jmp QWORD PTR [g_winmmFunctions + 1344]
fwaveOutGetVolume ENDP

PUBLIC fwaveOutMessage
fwaveOutMessage PROC
    jmp QWORD PTR [g_winmmFunctions + 1352]
fwaveOutMessage ENDP

PUBLIC fwaveOutOpen
fwaveOutOpen PROC
    jmp QWORD PTR [g_winmmFunctions + 1360]
fwaveOutOpen ENDP

PUBLIC fwaveOutPause
fwaveOutPause PROC
    jmp QWORD PTR [g_winmmFunctions + 1368]
fwaveOutPause ENDP

PUBLIC fwaveOutPrepareHeader
fwaveOutPrepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 1376]
fwaveOutPrepareHeader ENDP

PUBLIC fwaveOutReset
fwaveOutReset PROC
    jmp QWORD PTR [g_winmmFunctions + 1384]
fwaveOutReset ENDP

PUBLIC fwaveOutRestart
fwaveOutRestart PROC
    jmp QWORD PTR [g_winmmFunctions + 1392]
fwaveOutRestart ENDP

PUBLIC fwaveOutSetPitch
fwaveOutSetPitch PROC
    jmp QWORD PTR [g_winmmFunctions + 1400]
fwaveOutSetPitch ENDP

PUBLIC fwaveOutSetPlaybackRate
fwaveOutSetPlaybackRate PROC
    jmp QWORD PTR [g_winmmFunctions + 1408]
fwaveOutSetPlaybackRate ENDP

PUBLIC fwaveOutSetVolume
fwaveOutSetVolume PROC
    jmp QWORD PTR [g_winmmFunctions + 1416]
fwaveOutSetVolume ENDP

PUBLIC fwaveOutUnprepareHeader
fwaveOutUnprepareHeader PROC
    jmp QWORD PTR [g_winmmFunctions + 1424]
fwaveOutUnprepareHeader ENDP

PUBLIC fwaveOutWrite
fwaveOutWrite PROC
    jmp QWORD PTR [g_winmmFunctions + 1432]
fwaveOutWrite ENDP

END
