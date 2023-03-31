[string] $type = $null
[string] $test = $false

foreach ( $arg in $args )
{
	if ( $arg -eq "test" )
	{
		$test = $true
	}
	else 
	{
		$type = $arg
	}
}


#region Regular Build

write-host "Building project`n"

$path_root    = git rev-parse --show-toplevel
$path_build   = Join-Path $path_root build
$path_scripts = Join-Path $path_root scripts


if ( -not( Test-Path $path_build ) ) 
{
	$args_meson = @()
	$args_meson += "setup"
	$args_meson += $path_build

	# Start-Process meson $args_meson -NoNewWindow -Wait -WorkingDirectory $path_scripts
	Push-Location $path_scripts
	& meson $args_meson
	Pop-Location
}

if ( $type )
{
	$args_meson = @()
	$args_meson += "configure"
	$args_meson += $path_build
	$args_meson += "--buildtype $($type)"

	# Start-Process meson $args_meson -NoNewWindow -Wait -WorkingDirectory $path_scripts
	Push-Location $path_scripts
	meson $args_meson
	Pop-Location
}

$args_ninja = @()
$args_ninja += "-C"
$args_ninja += $path_build

Push-Location $path_root
ninja $args_ninja
Pop-Location
#endregion Regular Build


if ( $test -eq $true )
{
	#region Test Build
	write-host "`n`nBuilding Test`n"

	# Refactor thirdparty libraries
	Invoke-Expression "& $(Join-Path $PSScriptRoot 'refactor_and_format.ps1') $args" 


	$path_test       = Join-Path $path_root test
	$path_test_build = Join-Path $path_test build

	if ( -not( Test-Path $path_test_build ) ) 
	{
		$args_meson = @()
		$args_meson += "setup"
		$args_meson += $path_test_build

		Push-Location $path_test
		& meson $args_meson
		Pop-Location
	}

	$args_ninja = @()
	$args_ninja += "-C"
	$args_ninja += $path_test_build

	Push-Location $path_root
	ninja $args_ninja
	Pop-Location
	#endregion Test Build
}
