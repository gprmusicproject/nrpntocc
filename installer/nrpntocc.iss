; Setup of NRPN to CC for Windows, made with Inno Setup 6.
;
;   ISCC.exe nrpntocc.iss
;
; What it does
;   - the VST3 plug-in goes to Program Files\Common Files\VST3\GPR Music Project
;   - the LICENSE and README go to Program Files\GPR Music Project\NRPN to CC
;   - NRPN to CC is listed among the installed apps of Windows and removed from there;
;     that entry is the only thing written to the registry
;
; Defines
;   /DTestRoot=<folder>   builds a setup that needs no administrator and installs under that
;                         folder instead of the real places: for trying it out
;   /DBuildDir=<folder>   where the built plug-in is
;   /DAppVersion=<ver>    overrides the version number

#define AppName      "NRPN to CC"
#define Publisher    "GPR Music Project"
#define Vst3Name     "NRPN to CC.vst3"

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\build\NrpnToCc_artefacts\Release"
#endif

#ifdef TestRoot
  #define AppDir    TestRoot + "\Program Files\GPR Music Project\NRPN to CC"
  #define Vst3Dir   TestRoot + "\Program Files\Common Files\VST3\GPR Music Project"
  #define Vst3Root  TestRoot + "\Program Files\Common Files\VST3"
  #define MenuDir   TestRoot + "\Start Menu\NRPN to CC"
#else
  #define AppDir    "{autopf}\GPR Music Project\NRPN to CC"
  #define Vst3Dir   "{commoncf}\VST3\GPR Music Project"
  #define Vst3Root  "{commoncf}\VST3"
  #define MenuDir   "{group}"
#endif

[Setup]
#ifdef TestRoot
AppId=GPRMusicProject.NrpnToCc.Test
UninstallDisplayName={#AppName} (test build)
#else
AppId=GPRMusicProject.NrpnToCc
UninstallDisplayName={#AppName}
#endif
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Publisher}
AppPublisherURL=https://github.com/gprmusicproject/nrpntocc
AppSupportURL=https://github.com/gprmusicproject/nrpntocc/issues
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#Publisher}
VersionInfoDescription={#AppName} Setup
VersionInfoProductName={#AppName}
DefaultDirName={#AppDir}
DefaultGroupName={#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
DisableWelcomePage=no
AllowNoIcons=no
LicenseFile=..\LICENSE
#ifdef TestRoot
PrivilegesRequired=lowest
#else
PrivilegesRequired=admin
#endif
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CreateUninstallRegKey=yes
UsePreviousAppDir=no
UsePreviousGroup=no
Uninstallable=yes
UninstallFilesDir={app}
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
OutputDir=..\build\installer\out
#ifdef TestRoot
OutputBaseFilename=NRPN-to-CC_{#AppVersion}_Setup_test
#else
OutputBaseFilename=NRPN-to-CC_{#AppVersion}_Setup
#endif

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"

[Messages]
en.WelcomeLabel2=This will install [name/ver]: the VST3 MIDI instrument plug-in that converts NRPN to CC.%n%nClose your DAW/host before you go on.
it.WelcomeLabel2=Verrà installato [name/ver]: il plug-in strumento VST3 che converte NRPN in CC.%n%nChiudi la tua DAW/host prima di continuare.

[CustomMessages]
en.ReadyVst3=VST3 plug-in
en.ReadyDocs=License and README
en.ReadyRemove=To remove NRPN to CC: Windows Settings, Apps, Installed apps.
en.OpenReadme=Open the README
en.MenuReadme=NRPN to CC README
en.MenuUninstall=Uninstall NRPN to CC
en.InUse=NRPN to CC is in use. Close your DAW/host, then start Setup again.%n%nFile in use:%n%1
en.InUseUninstall=NRPN to CC is in use. Close your DAW/host, then start the removal again.%n%nFile in use:%n%1
en.OldCopy=Another copy of the plug-in was found in%n%1%n%nA host must not find two copies of NRPN to CC. Remove that copy now?
it.ReadyVst3=Plug-in VST3
it.ReadyDocs=Licenza e README
it.ReadyRemove=Per rimuovere NRPN to CC: Impostazioni di Windows, App, App installate.
it.OpenReadme=Apri il README
it.MenuReadme=README di NRPN to CC
it.MenuUninstall=Disinstalla NRPN to CC
it.InUse=NRPN to CC è in uso. Chiudi la tua DAW/host, poi avvia di nuovo il Setup.%n%nFile in uso:%n%1
it.InUseUninstall=NRPN to CC è in uso. Chiudi la tua DAW/host, poi avvia di nuovo la rimozione.%n%nFile in uso:%n%1
it.OldCopy=È stata trovata un'altra copia del plug-in in%n%1%n%nUn host non deve trovare due copie di NRPN to CC. Rimuovere ora quella copia?

[Files]
Source: "{#BuildDir}\VST3\{#Vst3Name}\*"; DestDir: "{#Vst3Dir}\{#Vst3Name}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{#MenuDir}\{cm:MenuReadme}"; Filename: "{app}\README.md"
Name: "{#MenuDir}\{cm:MenuUninstall}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\README.md"; Description: "{cm:OpenReadme}"; Flags: postinstall shellexec skipifsilent unchecked nowait

[Code]
function IsLocked(const FileName: String): Boolean;
var
  Stream: TFileStream;
begin
  Result := False;
  if not FileExists(FileName) then
    Exit;
  try
    Stream := TFileStream.Create(FileName, fmOpenReadWrite or fmShareExclusive);
    Stream.Free;
  except
    Result := True;
  end;
end;

// The installed plug-in binary if it is open in a running host, or nothing.
function BinaryInUse: String;
var
  Bin: String;
begin
  Result := '';
  Bin := ExpandConstant('{#Vst3Dir}\{#Vst3Name}\Contents\x86_64-win\{#Vst3Name}');
  if IsLocked(Bin) then
    Result := Bin;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := CustomMessage('ReadyVst3') + ':' + NewLine + Space + ExpandConstant('{#Vst3Dir}\{#Vst3Name}') + NewLine + NewLine
          + CustomMessage('ReadyDocs') + ':' + NewLine + Space + ExpandConstant('{app}') + NewLine + NewLine
          + CustomMessage('ReadyRemove');
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  InUse: String;
begin
  Result := '';
  InUse := BinaryInUse;
  if InUse <> '' then
    Result := FmtMessage(CustomMessage('InUse'), [InUse]);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Old: String;
begin
  if CurStep = ssInstall then
  begin
    // a copy put by hand straight into the VST3 folder would be found next to the new one
    Old := ExpandConstant('{#Vst3Root}\{#Vst3Name}');
    if DirExists(Old) or FileExists(Old) then
      if SuppressibleMsgBox(FmtMessage(CustomMessage('OldCopy'), [Old]), mbConfirmation, MB_YESNO, IDNO) = IDYES then
        DelTree(Old, True, True, True);
  end;
end;

// ── removal ──

function InitializeUninstall: Boolean;
var
  InUse: String;
begin
  Result := True;
  InUse := BinaryInUse;
  if InUse <> '' then
  begin
    MsgBox(FmtMessage(CustomMessage('InUseUninstall'), [InUse]), mbError, MB_OK);
    Result := False;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // leave the shared VST3\GPR Music Project folder only if nothing else is in it
    RemoveDir(ExpandConstant('{#Vst3Dir}'));
    RemoveDir(ExtractFileDir(ExpandConstant('{app}')));
  end;
end;
