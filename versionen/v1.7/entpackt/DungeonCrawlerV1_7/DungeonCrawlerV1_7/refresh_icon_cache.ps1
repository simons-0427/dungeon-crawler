param([Parameter(Mandatory=$true)][string]$Executable)

$target = (Resolve-Path -LiteralPath $Executable -ErrorAction Stop).Path

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class DungeonIconCache {
    [DllImport("shell32.dll", CharSet=CharSet.Unicode)]
    public static extern void SHChangeNotify(uint eventId, uint flags, IntPtr item1, IntPtr item2);
}
'@

$pathPointer = [Runtime.InteropServices.Marshal]::StringToHGlobalUni($target)
try {
    # SHCNE_UPDATEITEM with SHCNF_PATHW and SHCNF_FLUSH.
    [DungeonIconCache]::SHChangeNotify(0x00002000, 0x00001005, $pathPointer, [IntPtr]::Zero)
} finally {
    [Runtime.InteropServices.Marshal]::FreeHGlobal($pathPointer)
}

# Ask Explorer to discard cached executable icon associations as well.
[DungeonIconCache]::SHChangeNotify(0x08000000, 0x00001000, [IntPtr]::Zero, [IntPtr]::Zero)
