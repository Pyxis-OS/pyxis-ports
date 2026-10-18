" Loaded by the native startup adapter before optional home:// configuration.
set runtimepath=nvim_runtime://
set packpath=nvim_runtime://
set noswapfile nobackup nowritebackup
" Collect both streams through the shell's single ordered byte sink.
set shell=shell shellcmdflag=-c shellpipe=>%s\ 2>&1 shellredir=>%s\ 2>&1 noshelltemp
set notermguicolors
filetype plugin indent on
syntax enable
colorscheme pyxis
