package dev.nooki.plugin;

import com.intellij.lang.ASTNode;
import com.intellij.lang.ParserDefinition;
import com.intellij.lang.PsiParser;
import com.intellij.lexer.Lexer;
import com.intellij.openapi.project.Project;
import com.intellij.psi.FileViewProvider;
import com.intellij.psi.PsiElement;
import com.intellij.psi.PsiFile;
import com.intellij.psi.tree.IElementType;
import com.intellij.psi.tree.IFileElementType;
import com.intellij.psi.tree.TokenSet;
import com.intellij.extapi.psi.PsiFileBase;
import com.intellij.openapi.fileTypes.FileType;
import org.jetbrains.annotations.NotNull;

/**
 * Definición del analizador.
 *
 * Para colorear no hace falta un árbol de sintaxis de verdad: el parser consume
 * los tokens y construye un árbol plano. Cuando el lenguaje crezca, esto es lo
 * que se sustituye por un parser real.
 */
public final class NookiParserDefinition implements ParserDefinition {

    public static final IFileElementType ARCHIVO =
            new IFileElementType("NOOKI_ARCHIVO", NookiLanguage.INSTANCIA);

    @Override
    public @NotNull Lexer createLexer(Project project) {
        return new NookiLexer();
    }

    @Override
    public @NotNull PsiParser createParser(Project project) {
        return (raiz, constructor) -> {
            constructor.mark().done(raiz);
            return constructor.getTreeBuilt();
        };
    }

    @Override
    public @NotNull IFileElementType getFileNodeType() {
        return ARCHIVO;
    }

    @Override
    public @NotNull TokenSet getWhitespaceTokens() {
        return TokenSet.create(NookiTokens.ESPACIO);
    }

    @Override
    public @NotNull TokenSet getCommentTokens() {
        return TokenSet.create(NookiTokens.COMENTARIO);
    }

    @Override
    public @NotNull TokenSet getStringLiteralElements() {
        return TokenSet.create(NookiTokens.TEXTO, NookiTokens.TEXTO_MULTILINEA);
    }

    @Override
    public @NotNull PsiElement createElement(ASTNode nodo) {
        throw new UnsupportedOperationException("Nooki no construye elementos de sintaxis todavía");
    }

    @Override
    public @NotNull PsiFile createFile(@NotNull FileViewProvider proveedor) {
        return new PsiFileBase(proveedor, NookiLanguage.INSTANCIA) {
            @Override
            public @NotNull FileType getFileType() {
                return NookiFileType.INSTANCIA;
            }
        };
    }
}
