[Setup]
AppId={{8B2F3D9A-B7F1-4F9E-9D12-BaudixApp123}
AppName=Baudix
AppVersion=1.0.0
AppPublisher=hakanyz
DefaultDirName={autopf}\Baudix
DefaultGroupName=Baudix
AllowNoIcons=yes
OutputDir=Output
OutputBaseFilename=Baudix_Setup_v1.0.0
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "build\Release\baudix.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Baudix"; Filename: "{app}\baudix.exe"
Name: "{autodesktop}\Baudix"; Filename: "{app}\baudix.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\baudix.exe"; Description: "{cm:LaunchProgram,Baudix}"; Flags: nowait postinstall skipifsilent
