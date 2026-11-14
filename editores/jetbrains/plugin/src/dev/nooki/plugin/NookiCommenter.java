package dev.nooki.plugin;

import com.intellij.lang.Commenter;
import org.jetbrains.annotations.Nullable;

/**
 * Comentar y descomentar con Ctrl+/. Nooki solo tiene comentarios de línea
 * (`//`): el de bloque existe en la gramática, pero el compilador no lo reconoce.
 */
public final class NookiCommenter implements Commenter {

    @Override
    public @Nullable String getLineCommentPrefix() {
        return "// ";
    }

    @Override
    public @Nullable String getBlockCommentPrefix() {
        return null;
    }

    @Override
    public @Nullable String getBlockCommentSuffix() {
        return null;
    }

    @Override
    public @Nullable String getCommentedBlockCommentPrefix() {
        return null;
    }

    @Override
    public @Nullable String getCommentedBlockCommentSuffix() {
        return null;
    }
}
