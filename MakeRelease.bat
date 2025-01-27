rmdir /S /Q ReleasePkg

mkdir ReleasePkg

mkdir ReleasePkg\Aerofoil
mkdir ReleasePkg\Aerofoil\Packaged
mkdir ReleasePkg\Aerofoil\Resources
mkdir ReleasePkg\Aerofoil\Tools

copy /Y x64\Release\Aerofoil.exe ReleasePkg\Aerofoil
copy /Y x64\Release\GpAudioDriver_XAudio2.dll ReleasePkg\Aerofoil
copy /Y x64\Release\xaudio2_9redist.dll ReleasePkg\Aerofoil
copy /Y x64\Release\GpDisplayDriver_D3D11.dll ReleasePkg\Aerofoil
copy /Y x64\Release\GpInputDriver_XInput.dll ReleasePkg\Aerofoil
copy /Y x64\Release\FreeType.dll ReleasePkg\Aerofoil
copy /Y x64\Release\GpApp.dll ReleasePkg\Aerofoil

copy /Y x64\Release\flattenmov.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\bin2gp.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\hqx2bin.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\hqx2gp.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\MakeTimestamp.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\FTagData.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\gpr2gpa.exe ReleasePkg\Aerofoil\Tools
copy /Y x64\Release\unpacktool.exe ReleasePkg\Aerofoil\Tools

mkdir ReleasePkg\PDBs

copy /Y x64\Release\Aerofoil.pdb ReleasePkg\PDBs
copy /Y x64\Release\GpAudioDriver_XAudio2.pdb ReleasePkg\PDBs
copy /Y x64\Release\GpDisplayDriver_D3D11.pdb ReleasePkg\PDBs
copy /Y x64\Release\GpFontHandler_FreeType2.pdb ReleasePkg\PDBs
copy /Y x64\Release\GpInputDriver_XInput.pdb ReleasePkg\PDBs
copy /Y x64\Release\FreeType.pdb ReleasePkg\PDBs
copy /Y x64\Release\GpApp.pdb ReleasePkg\PDBs

copy /Y x64\Release\flattenmov.pdb ReleasePkg\PDBs
copy /Y x64\Release\bin2gp.pdb ReleasePkg\PDBs
copy /Y x64\Release\hqx2bin.pdb ReleasePkg\PDBs
copy /Y x64\Release\hqx2gp.pdb ReleasePkg\PDBs
copy /Y x64\Release\MakeTimestamp.pdb ReleasePkg\PDBs
copy /Y x64\Release\FTagData.pdb ReleasePkg\PDBs
copy /Y x64\Release\gpr2gpa.pdb ReleasePkg\PDBs
copy /Y x64\Release\unpacktool.pdb ReleasePkg\PDBs

xcopy /I /E /Y /K Packaged ReleasePkg\Aerofoil\Packaged
xcopy /I /E /Y /K Resources ReleasePkg\Aerofoil\Resources
xcopy /I /E /Y /K Documentation ReleasePkg\Aerofoil\Documentation

attrib +R Packaged\Houses\*

rmdir /S /Q InstallerPackages

mkdir InstallerPackages
mkdir InstallerPackages\DefaultHouses
mkdir InstallerPackages\HousePack1
mkdir InstallerPackages\LooseDocumentation

copy /Y Packaged\Houses\* InstallerPackages\HousePack1\
del /Q InstallerPackages\HousePack1\Slumberland.*
del /Q "InstallerPackages\HousePack1\Demo House.*"
copy /Y Packaged\Houses\Slumberland.* InstallerPackages\DefaultHouses
copy /Y "Packaged\Houses\Demo House.*" InstallerPackages\DefaultHouses

attrib +R InstallerPackages\DefaultHouses\*
attrib +R InstallerPackages\HousePack1\*

copy /Y Documentation\* InstallerPackages\LooseDocumentation\
del /Q InstallerPackages\LooseDocumentation\readme.txt

pause
