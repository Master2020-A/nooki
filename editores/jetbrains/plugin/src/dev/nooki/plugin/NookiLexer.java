package dev.nooki.plugin;

import com.intellij.lexer.LexerBase;
import com.intellij.psi.tree.IElementType;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * Un lexer de Nooki, escrito a mano.
 *
 * No hace falta un árbol de sintaxis para colorear: basta con partir el texto en
 * tokens y decir de qué tipo es cada uno. Esto reconoce comentarios, textos
 * (incluidos los de tres comillas), números, palabras y operadores.
 */
public final class NookiLexer extends LexerBase {

    private CharSequence texto = "";
    private int inicio;
    private int fin;
    private int tokenInicio;
    private int tokenFin;
    private IElementType token;

    @Override
    public void start(@NotNull CharSequence buffer, int startOffset, int endOffset, int initialState) {
        texto = buffer;
        inicio = startOffset;
        fin = endOffset;
        tokenInicio = startOffset;
        tokenFin = startOffset;
        token = null;
        advance();
    }

    @Override
    public int getState() {
        return 0;
    }

    @Override
    public @Nullable IElementType getTokenType() {
        return token;
    }

    @Override
    public int getTokenStart() {
        return tokenInicio;
    }

    @Override
    public int getTokenEnd() {
        return tokenFin;
    }

    @Override
    public void advance() {
        if (tokenFin >= fin) {
            token = null;
            tokenInicio = fin;
            tokenFin = fin;
            return;
        }
        tokenInicio = tokenFin;
        token = siguiente();
    }

    @Override
    public void restore(@NotNull com.intellij.lexer.LexerPosition posicion) {
        // El lexer no guarda estado: cada token se reconoce por sí solo, así que
        // basta con volver a la posición que pide el editor.
        tokenInicio = posicion.getOffset();
        tokenFin = tokenInicio;
        token = null;
        advance();
    }

    @Override
    public @NotNull CharSequence getBufferSequence() {
        return texto;
    }

    @Override
    public int getBufferEnd() {
        return fin;
    }

    /** Reconoce el token que empieza en `tokenInicio` y deja `tokenFin` al final. */
    private IElementType siguiente() {
        final int i = tokenInicio;

        // Comentario de línea o de bloque.
        if (i + 1 < fin && texto.charAt(i) == '/' && texto.charAt(i + 1) == '/') {
            int j = i;
            while (j < fin && texto.charAt(j) != '\n') {
                ++j;
            }
            tokenFin = j;
            return NookiTokens.COMENTARIO;
        }
        if (i + 1 < fin && texto.charAt(i) == '/' && texto.charAt(i + 1) == '*') {
            int j = i + 2;
            while (j + 1 < fin && !(texto.charAt(j) == '*' && texto.charAt(j + 1) == '/')) {
                ++j;
            }
            tokenFin = Math.min(fin, j + 2);
            return NookiTokens.COMENTARIO;
        }

        // Texto de tres comillas: puede ocupar varias líneas y no escapa nada.
        if (i + 2 < fin && texto.charAt(i) == '"' && texto.charAt(i + 1) == '"'
                && texto.charAt(i + 2) == '"') {
            int j = i + 3;
            while (j + 2 < fin && !(texto.charAt(j) == '"' && texto.charAt(j + 1) == '"'
                    && texto.charAt(j + 2) == '"')) {
                ++j;
            }
            tokenFin = Math.min(fin, j + 3);
            return NookiTokens.TEXTO_MULTILINEA;
        }

        // Texto normal, con escapes.
        if (texto.charAt(i) == '"') {
            int j = i + 1;
            while (j < fin) {
                final char c = texto.charAt(j);
                if (c == '\\' && j + 1 < fin) {
                    j += 2;
                    continue;
                }
                if (c == '"') {
                    ++j;
                    break;
                }
                if (c == '\n') {
                    break;  // un texto sin cerrar termina en el salto
                }
                ++j;
            }
            tokenFin = j;
            return NookiTokens.TEXTO;
        }

        // Números: con guiones bajos, hexadecimales y decimales.
        if (Character.isDigit(texto.charAt(i))) {
            int j = i;
            if (texto.charAt(i) == '0' && i + 1 < fin
                    && (texto.charAt(i + 1) == 'x' || texto.charAt(i + 1) == 'X')) {
                j += 2;
                while (j < fin && (Character.isDigit(texto.charAt(j))
                        || Character.isLetter(texto.charAt(j)) || texto.charAt(j) == '_')) {
                    ++j;
                }
            } else {
                while (j < fin && (Character.isDigit(texto.charAt(j))
                        || texto.charAt(j) == '_')) {
                    ++j;
                }
                if (j + 1 < fin && texto.charAt(j) == '.'
                        && Character.isDigit(texto.charAt(j + 1))) {
                    ++j;
                    while (j < fin && (Character.isDigit(texto.charAt(j))
                            || texto.charAt(j) == '_')) {
                        ++j;
                    }
                }
            }
            tokenFin = j;
            return NookiTokens.NUMERO;
        }

        // Identificadores y palabras clave. Las tildes y la eñe cuentan.
        if (NookiTokens.empiezaIdentificador(texto.charAt(i))) {
            int j = i + 1;
            while (j < fin && NookiTokens.continuaIdentificador(texto.charAt(j))) {
                ++j;
            }
            tokenFin = j;
            final String palabra = texto.subSequence(i, j).toString();
            final IElementType conocido = NookiTokens.PALABRAS.get(palabra);
            return conocido != null ? conocido : NookiTokens.IDENTIFICADOR;
        }

        // Espacios y saltos de línea.
        if (Character.isWhitespace(texto.charAt(i))) {
            int j = i + 1;
            while (j < fin && Character.isWhitespace(texto.charAt(j))) {
                ++j;
            }
            tokenFin = j;
            return NookiTokens.ESPACIO;
        }

        // Operadores de dos y tres caracteres.
        final String resto = texto.subSequence(i, Math.min(fin, i + 3)).toString();
        for (String operador : new String[]{"..=", "**", "==", "!=", "<=", ">=", "&&", "||",
                "..", "->"}) {
            if (resto.startsWith(operador)) {
                tokenFin = i + operador.length();
                return NookiTokens.OPERADOR;
            }
        }

        final char c = texto.charAt(i);
        tokenFin = i + 1;
        if ("+-*/%<>=!&?*".indexOf(c) >= 0) {
            return NookiTokens.OPERADOR;
        }
        if ("{}()[],;:.".indexOf(c) >= 0) {
            return NookiTokens.PUNTUACION;
        }
        return NookiTokens.DESCONOCIDO;
    }
}
