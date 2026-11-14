package dev.nooki.plugin;

import com.intellij.openapi.editor.DefaultLanguageHighlighterColors;
import com.intellij.openapi.editor.colors.TextAttributesKey;
import com.intellij.openapi.fileTypes.SyntaxHighlighter;
import com.intellij.openapi.fileTypes.SyntaxHighlighterBase;
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.vfs.VirtualFile;
import com.intellij.lexer.Lexer;
import com.intellij.psi.tree.IElementType;
import org.jetbrains.annotations.NotNull;

/** Los colores de Nooki, tomados del esquema del editor. */
public final class NookiSyntaxHighlighter extends SyntaxHighlighterBase {

    public static final TextAttributesKey COMENTARIO = clave(
            "NOOKI_COMENTARIO", DefaultLanguageHighlighterColors.LINE_COMMENT);
    public static final TextAttributesKey TEXTO = clave(
            "NOOKI_TEXTO", DefaultLanguageHighlighterColors.STRING);
    public static final TextAttributesKey NUMERO = clave(
            "NOOKI_NUMERO", DefaultLanguageHighlighterColors.NUMBER);
    public static final TextAttributesKey DECLARACION = clave(
            "NOOKI_DECLARACION", DefaultLanguageHighlighterColors.KEYWORD);
    public static final TextAttributesKey CONTROL = clave(
            "NOOKI_CONTROL", DefaultLanguageHighlighterColors.KEYWORD);
    public static final TextAttributesKey TIPO = clave(
            "NOOKI_TIPO", DefaultLanguageHighlighterColors.KEYWORD);
    public static final TextAttributesKey GENERICO = clave(
            "NOOKI_GENERICO", DefaultLanguageHighlighterColors.CLASS_NAME);
    public static final TextAttributesKey VISIBILIDAD = clave(
            "NOOKI_VISIBILIDAD", DefaultLanguageHighlighterColors.KEYWORD);
    public static final TextAttributesKey OBJETO = clave(
            "NOOKI_OBJETO", DefaultLanguageHighlighterColors.KEYWORD);
    public static final TextAttributesKey META = clave(
            "NOOKI_META", DefaultLanguageHighlighterColors.KEYWORD);
    public static final TextAttributesKey LITERAL = clave(
            "NOOKI_LITERAL", DefaultLanguageHighlighterColors.CONSTANT);
    public static final TextAttributesKey OPERADOR = clave(
            "NOOKI_OPERADOR", DefaultLanguageHighlighterColors.OPERATION_SIGN);
    public static final TextAttributesKey PUNTUACION = clave(
            "NOOKI_PUNTUACION", DefaultLanguageHighlighterColors.BRACES);
    public static final TextAttributesKey IDENTIFICADOR = clave(
            "NOOKI_IDENTIFICADOR", DefaultLanguageHighlighterColors.IDENTIFIER);

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
            return new TextAttributesKey[]{COMENTARIO};
        }
        if (token == NookiTokens.TEXTO || token == NookiTokens.TEXTO_MULTILINEA) {
            return new TextAttributesKey[]{TEXTO};
        }
        if (token == NookiTokens.NUMERO) {
            return new TextAttributesKey[]{NUMERO};
        }
        if (token == NookiTokens.DECLARACION) {
            return new TextAttributesKey[]{DECLARACION};
        }
        if (token == NookiTokens.CONTROL) {
            return new TextAttributesKey[]{CONTROL};
        }
        if (token == NookiTokens.TIPO) {
            return new TextAttributesKey[]{TIPO};
        }
        if (token == NookiTokens.GENERICO) {
            return new TextAttributesKey[]{GENERICO};
        }
        if (token == NookiTokens.VISIBILIDAD) {
            return new TextAttributesKey[]{VISIBILIDAD};
        }
        if (token == NookiTokens.OBJETO) {
            return new TextAttributesKey[]{OBJETO};
        }
        if (token == NookiTokens.META) {
            return new TextAttributesKey[]{META};
        }
        if (token == NookiTokens.LITERAL) {
            return new TextAttributesKey[]{LITERAL};
        }
        if (token == NookiTokens.OPERADOR) {
            return new TextAttributesKey[]{OPERADOR};
        }
        if (token == NookiTokens.PUNTUACION) {
            return new TextAttributesKey[]{PUNTUACION};
        }
        if (token == NookiTokens.IDENTIFICADOR) {
            return new TextAttributesKey[]{IDENTIFICADOR};
        }
        return TextAttributesKey.EMPTY_ARRAY;
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
