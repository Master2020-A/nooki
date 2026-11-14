package dev.nooki.plugin;

import com.intellij.openapi.fileTypes.LanguageFileType;
import javax.swing.Icon;
import org.jetbrains.annotations.NotNull;

/** Los archivos de Nooki terminan en `.oki`. */
public final class NookiFileType extends LanguageFileType {

    public static final NookiFileType INSTANCIA = new NookiFileType();

    private NookiFileType() {
        super(NookiLanguage.INSTANCIA);
    }

    @Override
    public @NotNull String getName() {
        return "Nooki";
    }

    @Override
    public @NotNull String getDescription() {
        return "Archivo del lenguaje Nooki";
    }

    @Override
    public @NotNull String getDefaultExtension() {
        return "oki";
    }

    @Override
    public Icon getIcon() {
        return null;
    }
}
