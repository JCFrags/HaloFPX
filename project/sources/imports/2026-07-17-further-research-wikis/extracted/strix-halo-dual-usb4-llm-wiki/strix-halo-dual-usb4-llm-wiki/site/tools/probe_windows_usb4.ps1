# Read-only Windows inventory for USB4 interdomain networking.
$ErrorActionPreference = "Continue"
Write-Output "===== timestamp ====="
Get-Date -Format o
Write-Output "===== operating system ====="
Get-ComputerInfo | Select-Object WindowsProductName, WindowsVersion, OsBuildNumber, BiosFirmwareType, BiosVersion
Write-Output "===== USB4 / Thunderbolt PnP devices ====="
Get-PnpDevice | Where-Object {
  $_.FriendlyName -match 'USB4|Thunderbolt' -or $_.InstanceId -match 'USB4|THUNDERBOLT'
} | Sort-Object Class,FriendlyName | Format-Table -AutoSize Status,Class,FriendlyName,InstanceId
Write-Output "===== network adapters ====="
Get-NetAdapter -IncludeHidden | Sort-Object Name | Format-Table -AutoSize Name,InterfaceDescription,Status,LinkSpeed,MacAddress,ifIndex
Write-Output "===== network configuration ====="
Get-NetIPConfiguration -All | Format-List InterfaceAlias,InterfaceDescription,NetProfile,IPv4Address,IPv6Address,IPv4DefaultGateway
Write-Output "===== relevant signed drivers ====="
Get-CimInstance Win32_PnPSignedDriver | Where-Object {
  $_.DeviceName -match 'USB4|Thunderbolt'
} | Select-Object DeviceName,DriverVersion,DriverDate,Manufacturer,InfName | Format-Table -AutoSize
