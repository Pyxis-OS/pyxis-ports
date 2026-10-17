" Loaded by the native startup adapter before optional home:// configuration.
set runtimepath=nvim_runtime://
set packpath=nvim_runtime://
set noswapfile nobackup nowritebackup
" :!, system() and :make run one Pyxis shell -c line. The shell has no 2>&1,
" so :make collects stderr, where compilers report; filters use pipes.
set shell=shell shellcmdflag=-c shellpipe=2>%s shellredir=2>%s noshelltemp
set notermguicolors
filetype plugin indent on
syntax enable
colorscheme pyxis
