package dev.nooki.plugin;

import com.intellij.lang.Language;

/** El lenguaje Nooki: solo necesita un identificador. */
public final class NookiLanguage extends Language {

    public static final NookiLanguage INSTANCIA = new NookiLanguage();

    private NookiLanguage() {
        super("Nooki");
    }
}
