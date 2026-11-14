package dev.nooki.plugin;

import com.intellij.lang.BracePair;
import com.intellij.lang.PairedBraceMatcher;
import com.intellij.psi.PsiFile;
import com.intellij.psi.tree.IElementType;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/** Empareja llaves, paréntesis y corchetes. */
public final class NookiBraceMatcher implements PairedBraceMatcher {

    private static final BracePair[] PARES = new BracePair[]{
            new BracePair(NookiTokens.PUNTUACION, NookiTokens.PUNTUACION, true),
    };

    @Override
    public BracePair @NotNull [] getPairs() {
        return PARES;
    }

    @Override
    public boolean isPairedBracesAllowedBeforeType(@NotNull IElementType tipo,
                                                   @Nullable IElementType siguiente) {
        return true;
    }

    @Override
    public int getCodeConstructStart(PsiFile archivo, int apertura) {
        return apertura;
    }
}
