# Run from an extracted clean source ZIP. Creates only whiteyt223/corncob3d.
# Requires Git and GitHub CLI. Does not deploy the game or change existing repos.
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
foreach ($program in @('git','gh')) {
    if (-not (Get-Command $program -ErrorAction SilentlyContinue)) {
        throw "Install $program first, then run this script again. Git: https://git-scm.com/downloads ; GitHub CLI: https://cli.github.com/"
    }
}
if (Test-Path '.git') { throw 'This folder already has Git history. Use a fresh extraction of the source ZIP.' }
& gh auth status
if ($LASTEXITCODE -ne 0) { throw 'Sign in first with: gh auth login' }
$account = & gh api user --jq .login
if ($LASTEXITCODE -ne 0 -or $account.Trim() -ne 'whiteyt223') { throw 'Sign in to GitHub as whiteyt223 first.' }
& gh repo view whiteyt223/corncob3d --json nameWithOwner 2>$null
if ($LASTEXITCODE -eq 0) { throw 'whiteyt223/corncob3d already exists. This script will not overwrite it.' }
foreach ($required in @('README.md','LICENSE.md','core','web','tools','docs/PUBLIC_INPUTS.json')) {
    if (-not (Test-Path $required)) { throw "Missing source item: $required" }
}
& git init --initial-branch=main
if ($LASTEXITCODE -ne 0) { throw 'Git initialization failed.' }
& git add -- .
if ($LASTEXITCODE -ne 0) { throw 'Git staging failed.' }
& git -c user.name=whiteyt223 -c user.email=215434012+whiteyt223@users.noreply.github.com commit -m 'Publish current Corncob 3D browser source with build and licensing notes'
if ($LASTEXITCODE -ne 0) { throw 'Git commit failed.' }
& gh repo create whiteyt223/corncob3d --public --source . --remote origin --push --description 'Source-derived Corncob 3D browser port: C++/WebAssembly, JavaScript, Shareware 3.42 and Other Worlds' --homepage https://corncob3d.com
if ($LASTEXITCODE -ne 0) { throw 'GitHub publication did not finish. Inspect the error and repository before retrying. Your local commit is preserved.' }
Write-Host 'Published: https://github.com/whiteyt223/corncob3d'
Write-Host 'The old private repository and Cloudflare deployment were not changed.'
