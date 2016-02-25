" Vim syntax file
" Language:     YANG
" Remark:       RFC 6020 http://tools.ietf.org/html/rfc6020
" Version:      1
" Last Change:  2011 Sep 28
" Maintainer:   Matt Parker <mparker@computer.org>
"------------------------------------------------------------------

if version < 600
    syntax clear
elseif exists("b:current_syntax")
    finish
endif

" yang has keywords with a '-' in them
setlocal iskeyword+=-

" keywords are case-sensitive
syn case match

" enable block folding
syn region yangBlock start="{" end="}" fold transparent


" built-in types (section 4.2.4)
syn keyword yangType decimal64 int8 int16 int32 int64 uint8 uint16 uint32 uint64
syn keyword yangType string boolean enumeration bits binary leafref identityref empty instance-identifier

syn match yangIdentifier /\c\<\h\+[A-Za-z0-9_-]*\>/

" identifiers must not begin with 'xml'. this rule must be defined after the previous yangIdentifier to work properly.
syn match yangBadIdentifier /\c\<xml\(\h\+[A-Za-z0-9_-]\)*\>/

" statement keywords
syn keyword yangStatement anyxml argument augment base belongs-to bit case choice
syn keyword yangStatement config contact container default description enum error-app-tag error-message
syn keyword yangStatement extension deviation deviate feature fraction-digits grouping identity
syn keyword yangStatement import include input key leaf leaf-list length
syn keyword yangStatement list mandatory max-elements min-elements module must namespace
syn keyword yangStatement notification ordered-by organization output path pattern position
syn keyword yangStatement prefix presence range reference refine require-instance revision
syn keyword yangStatement revision-date rpc status submodule type typedef unique
syn keyword yangStatement units uses value when yang-version yin-element 

" other keywords
syn keyword yangOther add current delete deprecated max min not-supported
syn keyword yangOther obsolete replace system unbounded user

" boolean constants (separated from the 'other keywords' for vim syntax purpose)
syn keyword yangBoolean true false

" if-feature (separated from 'statement keywords' for vim syntax purposes)
syn keyword yangConditional if-feature

" comments
syn region yangComment start=/\/\*/ end=/\*\//
syn region yangComment start="//" end="$" 

" strings
syn region yangString start=+"+ skip=+\\\\\|\\"+ end=+"+
syn region yangString start=+'+ skip=+\\\\\|\\'+ end=+'+

" dates
syn match yangDateArg /"\=\<\d\{4}-\d\{2}-\d\{2}\>"\=/

" length-arg TODO: this needs to also include the pipe and individual numbers (i.e. fixed length)
syn match yangLengthArg /"\(\d\+\|min\)\s*\.\.\s*\(\d\+\|max\)"/

" numbers
syn match yangNumber /\<[+-]\=\d\+\>/
syn match yangNumber    "\<0x\x\+\>"


"-------------------------------------
" and now for the highlighting

" things with one-to-one mapping
hi def link yangBadIdentifier Error
hi def link yangIdentifier Identifier
hi def link yangString String
hi def link yangComment Comment
hi def link yangNumber Number
hi def link yangBoolean Boolean
hi def link yangConditional Conditional
hi def link yangType Type

" arbitrary mappings
hi def link yangKeyword Type
hi def link yangStatement Type
hi def link yangModule Type
hi def link yangDateArg Conditional
hi def link yangLengthArg Conditional

let b:current_syntax = "yang"


" Vim indent file
" Language: C++
" Maintainer:   Konstantin Lepa <konstantin.lepa@gmail.com>
" Last Change:  2010 May 20
" License: MIT
" Version: 1.1.0
"
" Changes {{{
" 1.1.0 2011-01-17
"   Refactored source code.
"   Some fixes.
"
" 1.0.1 2010-05-20
"   Added some changes. Thanks to Eric Rannaud <eric.rannaud@gmail.com>
"
"}}}

if exists("b:did_indent")
    finish
endif
let b:did_indent = 1


function! GoogleCppIndent()
    let l:cline_num = line('.')

    let l:orig_indent = cindent(l:cline_num)

    if l:orig_indent == 0 | return 0 | endif

    let l:pline_num = prevnonblank(l:cline_num - 1)
    let l:pline = getline(l:pline_num)
    if l:pline =~# '^\s*template' | return l:pline_indent | endif

    " TODO: I don't know to correct it:
    " namespace test {
    " void
    " ....<-- invalid cindent pos
    "
    " void test() {
    " }
    "
    " void
    " <-- cindent pos
    if l:orig_indent != &shiftwidth | return l:orig_indent | endif

    let l:in_comment = 0
    let l:pline_num = prevnonblank(l:cline_num - 1)
    while l:pline_num > -1
        let l:pline = getline(l:pline_num)
        let l:pline_indent = indent(l:pline_num)

        if l:in_comment == 0 && l:pline =~ '^.\{-}\(/\*.\{-}\)\@<!\*/'
            let l:in_comment = 1
        elseif l:in_comment == 1
            if l:pline =~ '/\*\(.\{-}\*/\)\@!'
                let l:in_comment = 0
            endif
        elseif l:pline_indent == 0
            if l:pline !~# '\(#define\)\|\(^\s*//\)\|\(^\s*{\)'
                if l:pline =~# '^\s*namespace.*'
                    return 0
                else
                    return l:orig_indent
                endif
            elseif l:pline =~# '\\$'
                return l:orig_indent
            endif
        else
            return l:orig_indent
        endif

        let l:pline_num = prevnonblank(l:pline_num - 1)
    endwhile

    return l:orig_indent
endfunction

setlocal shiftwidth=2
setlocal tabstop=2
setlocal softtabstop=2
setlocal expandtab
setlocal textwidth=80
setlocal wrap

setlocal cindent
setlocal cinoptions=h1,l1,g1,t0,i4,+4,(0,w1,W4

setlocal indentexpr=GoogleCppIndent()

let b:undo_indent = "setl sw< ts< sts< et< tw< wrap< cin< cino< inde<"


