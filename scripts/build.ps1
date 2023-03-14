cls

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

	Start-Process meson $args_meson -NoNewWindow -Wait -WorkingDirectory $path_scripts
}

if ( $type )
{
	$args_meson = @()
	$args_meson += "configure"
	$args_meson += $path_build
	$args_meson += "--buildtype $($type)"

	Start-Process meson $args_meson -NoNewWindow -Wait -WorkingDirectory $path_scripts
}
#endregion Regular Build


#region Test Build
write-host "`n`nBuilding Test`n"

if ( -not( Test-Path $path_build ) )
{
	return;
}

$args_ninja = @()
$args_ninja += "-C"
$args_ninja += $path_build

Start-Process ninja $args_ninja -Wait -NoNewWindow -WorkingDirectory $path_root

# Refactor thirdparty libraries
& .\refactor_and_format.ps1

if ( $test -eq $false )
{
	return;
}

$path_test       = Join-Path $path_root test
$path_test_build = Join-Path $path_test build

if ( -not( Test-Path $path_test_build ) ) 
{
	$args_meson = @()
	$args_meson += "setup"
	$args_meson += $path_test_build

	Start-Process meson $args_meson -NoNewWindow -Wait -WorkingDirectory $path_scripts
}

$args_ninja = @()
$args_ninja += "-C"
$args_ninja += $path_build

Start-Process ninja $args_ninja -Wait -NoNewWindow -WorkingDirectory $path_test
#endregion Test Build