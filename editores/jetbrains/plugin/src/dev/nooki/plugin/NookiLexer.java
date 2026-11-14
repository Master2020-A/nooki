package dev.nooki.plugin;

import com.intellij.lexer.LexerBase;
import com.intellij.lexer.LexerPosition;
import com.intellij.psi.tree.IElementType;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * Un lexer de Nooki, escrito a mano.
 *
 * No hace falta un árbol de sintaxis para colorear: basta con partir el texto en
 * tokens. Pero sí hace falta mirar alrededor para distinguir cosas que se
 * escriben igual: un nombre después de `funcion` es una función, después de
 * `clase` es un tipo, antes de `(` es una llamada, detrás de un punto es un
 * miembro y antes de `:` es el nombre de un campo.
 */
public final class NookiLexer extends LexerBase {

    private CharSequence texto = "";
    private int fin;
    private int tokenInicio;
    private int tokenFin;
    private IElementType token;

    @Override
    public void start(@NotNull CharSequence buffer, int startOffset, int endOffset, int initialState) {
        texto = buffer;
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
    public void restore(@NotNull LexerPosition posicion) {
        // El lexer no guarda estado entre tokens: todo lo que necesita saber lo
        // mira en el propio texto, así que basta con volver a la posición.
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

    private IElementType siguiente() {
        final int i = tokenInicio;

        // Comentarios.
        if (i + 1 < fin && texto.charAt(i) == '/' && texto.charAt(i + 1) == '/') {
            tokenFin = hastaElFinalDeLinea(i);
            return NookiTokens.COMENTARIO;
        }
        if (i + 1 < fin && texto.charAt(i) == '/' && texto.charAt(i + 1) == '*') {
            int j = i + 2;
            while (j + 1 < fin && !(texto.charAt(j) == '*' && texto.charAt(j + 1) == '/')) {
                ++j;
            }
            tokenFin = Math.min(fin, j + 2);
            return NookiTokens.COMENTARIO_BLOQUE;
        }

        // Texto de tres comillas: varias líneas y sin escapes.
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

        // Texto normal.
        if (texto.charAt(i) == '"') {
            tokenFin = finalDeTexto(i);
            return NookiTokens.TEXTO;
        }

        // Números.
        if (Character.isDigit(texto.charAt(i))) {
            tokenFin = finalDeNumero(i);
            return NookiTokens.NUMERO;
        }

        // Identificadores y palabras.
        if (NookiTokens.empiezaIdentificador(texto.charAt(i))) {
            int j = i + 1;
            while (j < fin && NookiTokens.continuaIdentificador(texto.charAt(j))) {
                ++j;
            }
            tokenFin = j;
            return clasificarPalabra(texto.subSequence(i, j).toString(), i);
        }

        // Espacios.
        if (Character.isWhitespace(texto.charAt(i))) {
            int j = i + 1;
            while (j < fin && Character.isWhitespace(texto.charAt(j))) {
                ++j;
            }
            tokenFin = j;
            return NookiTokens.ESPACIO;
        }

        // `&`, `&mutable` y el `?` de propagación. Antes hay que descartar `&&`,
        // que es un operador y no dos referencias.
        if (texto.charAt(i) == '&') {
            if (i + 1 < fin && texto.charAt(i + 1) == '&') {
                tokenFin = i + 2;
                return NookiTokens.OPERADOR;
            }
            final int j = siguienteNoEspacio(i + 1);
            if (j >= 0 && j + 7 <= fin && texto.subSequence(j, j + 7).toString().equals("mutable")) {
                tokenFin = j + 7;
            } else {
                tokenFin = i + 1;
            }
            return NookiTokens.REFERENCIA;
        }
        if (texto.charAt(i) == '?') {
            tokenFin = i + 1;
            return NookiTokens.REFERENCIA;
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
        if ("+-*/%<>=!".indexOf(c) >= 0) {
            return NookiTokens.OPERADOR;
        }
        if (c == '.') {
            return NookiTokens.PUNTO;
        }
        if (c == ',') {
            return NookiTokens.COMA;
        }
        if ("{}()[];:".indexOf(c) >= 0) {
            return NookiTokens.PUNTUACION;
        }
        return NookiTokens.DESCONOCIDO;
    }

    /** Decide el tipo de una palabra según lo que tenga alrededor. */
    private IElementType clasificarPalabra(String palabra, int inicioPalabra) {
        final IElementType conocido = NookiTokens.PALABRAS.get(palabra);
        if (conocido != null) {
            return conocido;
        }

        // El nombre que sigue a `funcion` es una función.
        final String anterior = palabraAnterior(inicioPalabra);
        if ("funcion".equals(anterior)) {
            return NookiTokens.NOMBRE_FUNCION;
        }
        if (anterior != null && NookiTokens.abreNombreDeTipo(anterior)) {
            return NookiTokens.NOMBRE_TIPO;
        }

        final int siguiente = siguienteNoEspacio(tokenFin);
        final boolean conParentesis = siguiente >= 0 && texto.charAt(siguiente) == '(';

        // Detrás de un punto es un miembro; si lleva paréntesis, un método.
        if (caracterAnterior(inicioPalabra) == '.') {
            return conParentesis ? NookiTokens.METODO : NookiTokens.CAMPO;
        }

        // Antes de `(` es una llamada; antes de `:` es el nombre de un campo.
        if (conParentesis) {
            return NookiTokens.LLAMADA;
        }
        if (siguiente >= 0 && texto.charAt(siguiente) == ':') {
            return NookiTokens.ETIQUETA;
        }

        // Un nombre con mayúscula inicial se lee como tipo.
        if (Character.isUpperCase(palabra.charAt(0))) {
            return NookiTokens.NOMBRE_TIPO;
        }
        return NookiTokens.IDENTIFICADOR;
    }

    private int hastaElFinalDeLinea(int i) {
        int j = i;
        while (j < fin && texto.charAt(j) != '\n') {
            ++j;
        }
        return j;
    }

    private int finalDeTexto(int i) {
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
                break;
            }
            ++j;
        }
        return j;
    }

    private int finalDeNumero(int i) {
        int j = i;
        if (i + 1 < fin && texto.charAt(i) == '0'
                && (texto.charAt(i + 1) == 'x' || texto.charAt(i + 1) == 'X')) {
            j += 2;
            while (j < fin && (Character.isLetterOrDigit(texto.charAt(j))
                    || texto.charAt(j) == '_')) {
                ++j;
            }
            return j;
        }
        while (j < fin && (Character.isDigit(texto.charAt(j)) || texto.charAt(j) == '_')) {
            ++j;
        }
        if (j + 1 < fin && texto.charAt(j) == '.'
                && Character.isDigit(texto.charAt(j + 1))) {
            ++j;
            while (j < fin && (Character.isDigit(texto.charAt(j)) || texto.charAt(j) == '_')) {
                ++j;
            }
        }
        return j;
    }

    /** El primer carácter no blanco a partir de `i`, o -1. */
    private int siguienteNoEspacio(int i) {
        int j = i;
        while (j < fin && Character.isWhitespace(texto.charAt(j))) {
            ++j;
        }
        return j < fin ? j : -1;
    }

    /** El carácter no blanco anterior a `i`, o 0. */
    private char caracterAnterior(int i) {
        int j = i - 1;
        while (j >= 0 && Character.isWhitespace(texto.charAt(j))) {
            --j;
        }
        return j >= 0 ? texto.charAt(j) : 0;
    }

    /** La palabra anterior a `i`, si la hay. */
    private String palabraAnterior(int i) {
        int j = i - 1;
        while (j >= 0 && Character.isWhitespace(texto.charAt(j))) {
            --j;
        }
        if (j < 0 || !NookiTokens.continuaIdentificador(texto.charAt(j))) {
            return null;
        }
        int inicio = j;
        while (inicio > 0 && NookiTokens.continuaIdentificador(texto.charAt(inicio - 1))) {
            --inicio;
        }
        return texto.subSequence(inicio, j + 1).toString();
    }
}
