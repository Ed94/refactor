[string[]] $include = '*.h', '*.hh', '*.hpp', '*.c', '*.cc', '*.cpp'
[string[]] $exclude = '*.g.*', '*.refactor', 'bloat.refactored.hpp', 'refactor.refactored.cpp'


$path_root       = git rev-parse --show-toplevel
$path_build      = Join-Path $path_root build
$path_test       = Join-Path $path_root test
$path_thirdparty = Join-Path $path_root thirdparty

$file_spec = Join-Path $path_test zpl.refactor
$refactor  = Join-Path $path_build refactor.exe

# Gather the files to be formatted.
$targetFiles = @(Get-ChildItem -Recurse -Path $path_thirdparty -Include $include -Exclude $exclude | Select-Object -ExpandProperty FullName)


write-host "Beginning refactor...`n"

$refactors = @(@())

# TODO: Change this to support refactoring the other files in the project directory.
# It needs two runs, one for the regular files, one for the zpl header.
foreach ( $file in $targetFiles )
{
    $destination = Join-Path $path_test (Split-Path $file -leaf)
    $destination = $destination.Replace( '.h', '.refactored.h' )
    
    $refactorParams = @(
        "-src=$($file)",
        "-dst=$($destination)"
        "-spec=$($file_spec)"
    )

    $refactors += (Start-Process $refactor $refactorParams -NoNewWindow -PassThru)
}

foreach ( $process in $refactors )
{
    if ( $process )
    {
        $process.WaitForExit()
    }
}

Write-Host "`nRefactoring complete`n`n"


# Can't format zpl library... (It hangs clang format)
if ( $false )
{
Write-Host "Beginning format...`n"

# Format the files.
$formatParams = @(
    '-i'          # In-place
    '-style=file' # Search for a .clang-format file in the parent directory of the source file.
    '-verbose'
)

$targetFiles = @(Get-ChildItem -Recurse -Path $path_test -Include $include -Exclude $exclude | Select-Object -ExpandProperty FullName)

clang-format $formatParams $targetFiles

Write-Host "`nFormatting complete"
}
