[string[]] $include = '*.h', '*.hh', '*.hpp', '*.c', '*.cc', '*.cpp'
[string[]] $exclude = '*.g.*', '*.refactor'


$path_root       = git rev-parse --show-toplevel
$path_build      = Join-Path $path_root build
$path_project    = Join-Path $path_root project
$path_test       = Join-Path $path_root test
$path_thirdparty = Join-Path $path_root thirdparty

$file_spec = Join-Path $path_test zpl.refactor
$refactor  = Join-Path $path_build refactor.exe

# Gather the files to be formatted.
$targetFiles     = @(Get-ChildItem -Recurse -Path $path_thirdparty -Include $include -Exclude $exclude | Select-Object -ExpandProperty FullName)
$refactoredFiles = @()

foreach ( $file in $targetFiles )
{
    $destination = Join-Path $path_test (Split-Path $file -leaf)
    $destination = $destination.Replace( '.h', '.refactored.h' )
    $destination = $destination.Replace( '.c', '.refactored.c' )

    $refactoredFiles += $destination
}


write-host "Beginning thirdpary refactor...`n"

$refactors = @(@())

if ( $false ){
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
}
else {
    $refactorParams = @(
        # "-debug",
        "-num=$($targetFiles.Count)"
        "-src=$($targetFiles)",
        "-dst=$($refactoredFiles)",
        "-spec=$($file_spec)"
    )

    Start-Process $refactor $refactorParams -NoNewWindow -PassThru -Wait
}

foreach ( $process in $refactors )
{
    if ( $process )
    {
        $process.WaitForExit()
    }
}

Write-Host "`nRefactoring complete`n`n"

write-host "Beginning project refactor...`n"

# Gather the files to be formatted.
$targetFiles  = @(Get-ChildItem -Recurse -Path $path_project -Include $include -Exclude $exclude | Select-Object -ExpandProperty FullName)
$refactoredFiles = @()

$file_spec = Join-Path $path_test project.refactor

write-host "FILE SPEC:" $file_spec

foreach ( $file in $targetFiles )
{
    $destination = Join-Path $path_test (Split-Path $file -leaf)
    $destination = $destination.Replace( '.hpp', '.refactored.hpp' )
    $destination = $destination.Replace( '.cpp', '.refactored.cpp' )

    $refactoredFiles += $destination
}

$refactorParams = @(
    # "-debug",
    "-num=$($targetFiles.Count)"
    "-src=$($targetFiles)",
    "-dst=$($refactoredFiles)",
    "-spec=$($file_spec)"
)

Start-Process $refactor $refactorParams -NoNewWindow -PassThru -Wait

write-host "`nRefactoring complete`n`n"


# Can't format zpl library... (It hangs clang format)
if ( $false ) {
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
