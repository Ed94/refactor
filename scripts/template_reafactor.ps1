[string[]] $include = '*.h', '*.hh', '*.hpp', '*.c', '*.cc', '*.cpp'
[string[]] $exclude = '*.g.*', '*.refactor'

# Change this to your root directory if needed.
$path_root = $PSScriptRoot

# Change this to your desired destination
$path_dest = $path_root

# Gather the files to be formatted.
$targetFiles     = @(Get-ChildItem -Recurse -Path $path_root -Include $include -Exclude $exclude | Select-Object -ExpandProperty FullName)
$refactoredFiles = @()

foreach ( $file in $targetFiles )
{
    $destination = Join-Path $path_dest (Split-Path $file -leaf)
    $destination = $destination.Replace( '.h', '.refactored.h' )
    $destination = $destination.Replace( '.c', '.refactored.c' )

    $refactoredFiles += $destination
}


write-host "Beginning refactor...`n"

$refactors = @(@())

$refactorParams = @(
    # "-debug",
    "-num=$($targetFiles.Count)"
    "-src=$($targetFiles)",
    "-dst=$($refactoredFiles)",
    "-spec=$($file_spec)"
)

& $refactor $refactorParams

Write-Host "`nRefactoring complete`n`n"
