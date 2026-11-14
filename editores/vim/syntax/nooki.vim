" Resaltado de sintaxis para Nooki (.oki).
" Instalación: copiar en ~/.vim/syntax/ (o en un plugin) y añadir
"   au BufRead,BufNewFile *.oki set filetype=nooki
" al vimrc, o usar el ftdetect incluido.

if exists("b:current_syntax")
  finish
endif

syntax case match

" Comentarios
syntax match nookiComentario "//.*$" contains=@Spell
highlight default link nookiComentario Comment

" Textos y caracteres
syntax region nookiTexto start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=nookiEscape
syntax region nookiCaracter start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=nookiEscape
syntax match nookiEscape "\\\\." contained
highlight default link nookiTexto String
highlight default link nookiCaracter String
highlight default link nookiEscape SpecialChar

" Números
syntax match nookiNumero "\<[0-9][0-9_]*\>"
syntax match nookiFlotante "\<[0-9][0-9_]*\.[0-9][0-9_]*\>"
highlight default link nookiNumero Number
highlight default link nookiFlotante Float

" Palabras reservadas
syntax keyword nookiDeclaracion variable constante funcion clase estructura rasgo interfaz enumeracion tipo modulo importar exportar externo implementar
syntax keyword nookiModificador publico privado protegido interna mutable
syntax keyword nookiControl si sino mientras para en hacer romper continuar retornar coincide caso defecto
syntax keyword nookiModulo usar externo implementa hereda inseguro asm como
syntax keyword nookiTipo entero flotante booleano texto caracter vacio lista tamano esperar nulo
syntax keyword nookiObjeto nuevo este Este super Vector Mapa Caja resultado
syntax keyword nookiBooleano verdadero falso
syntax keyword nookiReservada asm inseguro es obtener paso establecer

highlight default link nookiDeclaracion Statement
highlight default link nookiModificador StorageClass
highlight default link nookiControl Conditional
highlight default link nookiModulo Include
highlight default link nookiTipo Type
highlight default link nookiObjeto Identifier
highlight default link nookiBooleano Boolean
highlight default link nookiReservada Keyword

" Nombres de funciones y tipos definidos
syntax match nookiFuncion "\<funcion\s\+\zs[A-Za-zÁÉÍÓÚÜÑáéíóúüñ_][A-Za-zÁÉÍÓÚÜÑáéíóúüñ0-9_]*"
syntax match nookiTipoDefinido "\<\(clase\|estructura\|rasgo\|interfaz\|enumeracion\)\s\+\zs[A-Za-zÁÉÍÓÚÜÑáéíóúüñ_][A-Za-zÁÉÍÓÚÜÑáéíóúüñ0-9_]*"
syntax match nookiLlamada "\<[A-Za-zÁÉÍÓÚÜÑáéíóúüñ_][A-Za-zÁÉÍÓÚÜÑáéíóúüñ0-9_]*\ze\s*("
syntax match nookiModuloRef "\<[a-záéíóúüñ_][a-záéíóúüñ0-9_]*\ze\."
highlight default link nookiFuncion Function
highlight default link nookiTipoDefinido Type
highlight default link nookiLlamada Function
highlight default link nookiModuloRef Include

" Operadores
syntax match nookiOperador "==\|!=\|<=\|>=\|&&\|||\|\.\.=\|->\|?\|\+\|-\|\*\|/\|%\|=\|<\|>\|&\|!"
highlight default link nookiOperador Operator

let b:current_syntax = "nooki"
