" 
" Put this plugin under ~/.vim/plugin/
"
function! LoadpyCscope()
  let db = findfile("pycscope.out", ".;")
  if (!empty(db))
    let path = strpart(db, 0, match(db, "/pycscope.out$"))
    set nocscopeverbose " suppress 'duplicate connection' error
    exe "cs add " . db . " " . path
    set cscopeverbose
  endif
endfunction
au BufEnter /* call LoadpyCscope()
