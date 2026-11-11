param([Parameter(Mandatory=$true)][string]$NgspicePath)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$audit=Join-Path $root 'test-results/spice'
New-Item -ItemType Directory -Force $audit | Out-Null
$ng=(Resolve-Path -LiteralPath $NgspicePath).Path
foreach($mode in @('ge','si')) {
  if($mode -eq 'ge') {$is='80n';$bf='90';$br='2';$leak='15u'} else {$is='6.7f';$bf='160';$br='3';$leak='0'}
  foreach($boost in @('1','0.5')) {
    $fraction=([Math]::Exp([Math]::Log(100)*[double]$boost)-1)/99
    $top=10000*$fraction
    $bottom=[Math]::Max(0.000001,10000*(1-$fraction))
    $prefix="$mode-$boost"
    $net=@"
Independent PNP Rangemaster netlist, $mode, Boost rotation=$boost
Vbat supply 0 -9
Vin input 0 DC 0 AC 1 SIN(0 0.3 3000)
C1 input base 5n
R1 base supply 470k
R2 base 0 68k
Q1 collector base emitter device
R3 emitter 0 3.9k
C3 emitter 0 47u
Rtop supply wiper $top
Rbottom wiper collector $bottom
C2 wiper output 10n
Rload output 0 1meg
C4 supply 0 47u
Ileak emitter collector $leak
.model device PNP(IS=$is BF=$bf BR=$br TNOM=26.827)
.temp 26.827
.options reltol=1e-7 abstol=1e-13 vntol=1e-9
.control
set wr_singlescale
set wr_vecnames
op
print v(base) v(emitter) v(collector)
ac lin 1 100 100
print vdb(output)
ac lin 1 1000 1000
print vdb(output)
ac lin 1 5000 5000
print vdb(output)
tran 5.208333333333u 0.5 0 1u
linearize v(output)
wrdata $($prefix)-spice.txt v(output)
quit
.endc
.end
"@
    Set-Content -LiteralPath (Join-Path $audit "$prefix.cir") -Value $net -Encoding ascii
    Push-Location $audit
    try { & $ng -b -o "$prefix.log" "$prefix.cir" | Out-Null } finally {Pop-Location}
    Get-Content (Join-Path $audit "$prefix.log") | Select-String 'v\(base\)|v\(emitter\)|v\(collector\)|vdb|Error|Warning'
  }
}
