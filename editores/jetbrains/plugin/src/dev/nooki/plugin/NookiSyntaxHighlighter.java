package dev.nooki.plugin;

import com.intellij.lexer.Lexer;
import com.intellij.openapi.editor.DefaultLanguageHighlighterColors;
import com.intellij.openapi.editor.colors.TextAttributesKey;
import com.intellij.openapi.fileTypes.SyntaxHighlighter;
import com.intellij.openapi.fileTypes.SyntaxHighlighterBase;
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.vfs.VirtualFile;
import com.intellij.psi.tree.IElementType;
import org.jetbrains.annotations.NotNull;

/**
 * Los colores de Nooki.
 *
 * Cada construcción toma la categoría del esquema que le corresponde, para que
 * no se vean todas del mismo color: una función declarada no se parece a una
 * variable, ni un tipo a una llamada.
 */
public final class NookiSyntaxHighlighter extends SyntaxHighlighterBase {

    public static final TextAttributesKey COMENTARIO = clave(
            "NOOKI_COMENTARIO", DefaultLanguageHighlighterColors.LINE_COMMENT);
    public static final TextAttributesKey COMENTARIO_BLOQUE = clave(
            "NOOKI_COMENTARIO_BLOQUE", DefaultLanguageHighlighterColors.BLOCK_COMMENT);
    public static final TextAttributesKey TEXTO = clave(
            "NOOKI_TEXTO", DefaultLanguageHighlighterColors.STRING);
    public static final TextAttributesKey ESCAPE = clave(
            "NOOKI_ESCAPE", DefaultLanguageHighlighterColors.VALID_STRING_ESCAPE);
    public static final TextAttributesKey NUMERO = clave(
            "NOOKI_NUMERO", DefaultLanguageHighlighterColors.NUMBER);

    /** La palabra `funcion` y el nombre que la sigue. */
    public static final TextAttributesKey FUNCION = clave(
            "NOOKI_FUNCION", DefaultLanguageHighlighterColors.FUNCTION_DECLARATION);
    /** `variable` y `constante`. */
    public static final TextAttributesKey VARIABLE = clave(
            "NOOKI_VARIABLE", DefaultLanguageHighlighterColors.KEYWORD);
    /** `clase`, `estructura`, `rasgo`, `interfaz`, `enumeracion`, `tipo` y su nombre. */
    public static final TextAttributesKey TIPO_DECLARADO = clave(
            "NOOKI_TIPO_DECLARADO", DefaultLanguageHighlighterColors.CLASS_NAME);
    /** `si`, `mientras`, `para`, `retornar`, `romper`... */
    public static final TextAttributesKey CONTROL = clave(
            "NOOKI_CONTROL", DefaultLanguageHighlighterColors.KEYWORD);
    /** `coincide`, `caso`, `defecto`. */
    public static final TextAttributesKey PATRON = clave(
            "NOOKI_PATRON", DefaultLanguageHighlighterColors.KEYWORD);
    /** `usar`, `modulo`, `importar`, `externo`. */
    public static final TextAttributesKey MODULO = clave(
            "NOOKI_MODULO", DefaultLanguageHighlighterColors.METADATA);
    /** `inseguro`, `asm`. */
    public static final TextAttributesKey META = clave(
            "NOOKI_META", DefaultLanguageHighlighterColors.KEYWORD);
    /** `publico`, `privado`, `interna`, `mutable`. */
    public static final TextAttributesKey MODIFICADOR = clave(
            "NOOKI_MODIFICADOR", DefaultLanguageHighlighterColors.METADATA);
    /** `nuevo`, `este`, `super`, `hereda`, `implementa`. */
    public static final TextAttributesKey OBJETO = clave(
            "NOOKI_OBJETO", DefaultLanguageHighlighterColors.KEYWORD);
    /** `entero`, `texto`, `booleano`... */
    public static final TextAttributesKey TIPO_PRIMITIVO = clave(
            "NOOKI_TIPO_PRIMITIVO", DefaultLanguageHighlighterColors.PREDEFINED_SYMBOL);
    /** `Vector`, `Mapa`, `Caja`, `resultado`. */
    public static final TextAttributesKey TIPO_GENERICO = clave(
            "NOOKI_TIPO_GENERICO", DefaultLanguageHighlighterColors.CLASS_REFERENCE);
    /** `verdadero`, `falso`, `nulo`. */
    public static final TextAttributesKey LITERAL = clave(
            "NOOKI_LITERAL", DefaultLanguageHighlighterColors.CONSTANT);
    /** Una llamada `nombre(...)`. */
    public static final TextAttributesKey LLAMADA = clave(
            "NOOKI_LLAMADA", DefaultLanguageHighlighterColors.FUNCTION_CALL);
    /** `.metodo(...)` */
    public static final TextAttributesKey METODO = clave(
            "NOOKI_METODO", DefaultLanguageHighlighterColors.INSTANCE_METHOD);
    /** `.campo` */
    public static final TextAttributesKey CAMPO = clave(
            "NOOKI_CAMPO", DefaultLanguageHighlighterColors.INSTANCE_FIELD);
    /** El nombre de un campo en un literal: `punto { x: 1 }`. */
    public static final TextAttributesKey ETIQUETA = clave(
            "NOOKI_ETIQUETA", DefaultLanguageHighlighterColors.LABEL);
    public static final TextAttributesKey IDENTIFICADOR = clave(
            "NOOKI_IDENTIFICADOR", DefaultLanguageHighlighterColors.IDENTIFIER);
    public static final TextAttributesKey OPERADOR = clave(
            "NOOKI_OPERADOR", DefaultLanguageHighlighterColors.OPERATION_SIGN);
    /** `&`, `&mutable`, `?`. */
    public static final TextAttributesKey REFERENCIA = clave(
            "NOOKI_REFERENCIA", DefaultLanguageHighlighterColors.OPERATION_SIGN);
    public static final TextAttributesKey LLAVE = clave(
            "NOOKI_LLAVE", DefaultLanguageHighlighterColors.BRACES);
    public static final TextAttributesKey PARENTESIS = clave(
            "NOOKI_PARENTESIS", DefaultLanguageHighlighterColors.PARENTHESES);
    public static final TextAttributesKey CORCHETE = clave(
            "NOOKI_CORCHETE", DefaultLanguageHighlighterColors.BRACKETS);
    public static final TextAttributesKey PUNTO = clave(
            "NOOKI_PUNTO", DefaultLanguageHighlighterColors.DOT);
    public static final TextAttributesKey COMA = clave(
            "NOOKI_COMA", DefaultLanguageHighlighterColors.COMMA);

    private static TextAttributesKey clave(String nombre, TextAttributesKey base) {
        return TextAttributesKey.createTextAttributesKey(nombre, base);
    }


    @Override
    public @NotNull Lexer getHighlightingLexer() {
        return new NookiLexer();
    }

    @Override
    public TextAttributesKey @NotNull [] getTokenHighlights(IElementType token) {
        if (token == NookiTokens.COMENTARIO) {
            return paquete(COMENTARIO);
        }
        if (token == NookiTokens.COMENTARIO_BLOQUE) {
            return paquete(COMENTARIO_BLOQUE);
        }
        if (token == NookiTokens.TEXTO || token == NookiTokens.TEXTO_MULTILINEA) {
            return paquete(TEXTO);
        }
        if (token == NookiTokens.NUMERO) {
            return paquete(NUMERO);
        }
        if (token == NookiTokens.PALABRA_FUNCION || token == NookiTokens.NOMBRE_FUNCION) {
            return paquete(FUNCION);
        }
        if (token == NookiTokens.PALABRA_VARIABLE) {
            return paquete(VARIABLE);
        }
        if (token == NookiTokens.PALABRA_TIPO || token == NookiTokens.NOMBRE_TIPO) {
            return paquete(TIPO_DECLARADO);
        }
        if (token == NookiTokens.PALABRA_CONTROL) {
            return paquete(CONTROL);
        }
        if (token == NookiTokens.PALABRA_PATRON) {
            return paquete(PATRON);
        }
        if (token == NookiTokens.PALABRA_MODULO) {
            return paquete(MODULO);
        }
        if (token == NookiTokens.PALABRA_META) {
            return paquete(META);
        }
        if (token == NookiTokens.MODIFICADOR) {
            return paquete(MODIFICADOR);
        }
        if (token == NookiTokens.PALABRA_OBJETO) {
            return paquete(OBJETO);
        }
        if (token == NookiTokens.TIPO_PRIMITIVO) {
            return paquete(TIPO_PRIMITIVO);
        }
        if (token == NookiTokens.TIPO_GENERICO) {
            return paquete(TIPO_GENERICO);
        }
        if (token == NookiTokens.LITERAL) {
            return paquete(LITERAL);
        }
        if (token == NookiTokens.LLAMADA) {
            return paquete(LLAMADA);
        }
        if (token == NookiTokens.METODO) {
            return paquete(METODO);
        }
        if (token == NookiTokens.CAMPO) {
            return paquete(CAMPO);
        }
        if (token == NookiTokens.ETIQUETA) {
            return paquete(ETIQUETA);
        }
        if (token == NookiTokens.IDENTIFICADOR) {
            return paquete(IDENTIFICADOR);
        }
        if (token == NookiTokens.OPERADOR) {
            return paquete(OPERADOR);
        }
        if (token == NookiTokens.REFERENCIA) {
            return paquete(REFERENCIA);
        }
        if (token == NookiTokens.PUNTO) {
            return paquete(PUNTO);
        }
        if (token == NookiTokens.COMA) {
            return paquete(COMA);
        }
        if (token == NookiTokens.PUNTUACION) {
            return paquete(LLAVE);
        }
        return TextAttributesKey.EMPTY_ARRAY;
    }

    private static TextAttributesKey[] paquete(TextAttributesKey clave) {
        return new TextAttributesKey[]{clave};
    }

    /** La fábrica que el IDE usa al abrir un archivo. */
    public static final class Fabrica extends SyntaxHighlighterFactory {
        @Override
        public @NotNull SyntaxHighlighter getSyntaxHighlighter(Project project,
                                                              VirtualFile archivo) {
            return new NookiSyntaxHighlighter();
        }
    }
}
