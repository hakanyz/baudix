[Setup]
AppId={{8B2F3D9A-B7F1-4F9E-9D12-BaudixApp123}
AppName=Baudix
AppVersion=1.2.19
AppPublisher=hakanyz
AppPublisherURL=https://github.com/hakanyz/baudix
DefaultDirName={autopf}\Baudix
DisableProgramGroupPage=yes
OutputBaseFilename=Baudix_Setup_v1.2.19
SetupIconFile=resources\baudix_icon.ico
UninstallDisplayIcon={app}\baudix.exe
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The main executable
Source: "build\Release\baudix.exe"; DestDir: "{app}"; Flags: ignoreversion
; The Qt DLLs
Source: "build\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion
; Qt plugin directories
Source: "build\Release\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Baudix"; Filename: "{app}\baudix.exe"
Name: "{autodesktop}\Baudix"; Filename: "{app}\baudix.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\baudix.exe"; Description: "{cm:LaunchProgram,Baudix}"; Flags: nowait postinstall skipifsilent
Filename: "{app}\baudix.exe"; Flags: nowait; Check: IsSilentAndRestart

[Code]
function CmdLineParamExists(const Value: string): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
    if CompareText(ParamStr(I), Value) = 0 then
    begin
      Result := True;
      Exit;
    end;
end;

function IsSilentAndRestart: Boolean;
begin
  Result := WizardSilent() and CmdLineParamExists('/RESTART');
end;
