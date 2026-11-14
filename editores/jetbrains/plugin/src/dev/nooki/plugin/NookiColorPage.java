package dev.nooki.plugin;

import com.intellij.openapi.editor.colors.TextAttributesKey;
import com.intellij.openapi.fileTypes.SyntaxHighlighter;
import com.intellij.openapi.options.colors.AttributesDescriptor;
import com.intellij.openapi.options.colors.ColorDescriptor;
import com.intellij.openapi.options.colors.ColorSettingsPage;
import java.util.Map;
import javax.swing.Icon;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * La página de colores del esquema: permite ver y cambiar los colores de Nooki
 * desde Settings → Editor → Color Scheme → Nooki.
 */
public final class NookiColorPage implements ColorSettingsPage {

    private static final AttributesDescriptor[] ATRIBUTOS = new AttributesDescriptor[]{
            new AttributesDescriptor("Comentario", NookiSyntaxHighlighter.COMENTARIO),
            new AttributesDescriptor("Texto", NookiSyntaxHighlighter.TEXTO),
            new AttributesDescriptor("Número", NookiSyntaxHighlighter.NUMERO),
            new AttributesDescriptor("Declaración", NookiSyntaxHighlighter.DECLARACION),
            new AttributesDescriptor("Control de flujo", NookiSyntaxHighlighter.CONTROL),
            new AttributesDescriptor("Tipo", NookiSyntaxHighlighter.TIPO),
            new AttributesDescriptor("Genérico", NookiSyntaxHighlighter.GENERICO),
            new AttributesDescriptor("Visibilidad", NookiSyntaxHighlighter.VISIBILIDAD),
            new AttributesDescriptor("Objeto", NookiSyntaxHighlighter.OBJETO),
            new AttributesDescriptor("Meta", NookiSyntaxHighlighter.META),
            new AttributesDescriptor("Literal", NookiSyntaxHighlighter.LITERAL),
            new AttributesDescriptor("Operador", NookiSyntaxHighlighter.OPERADOR),
            new AttributesDescriptor("Puntuación", NookiSyntaxHighlighter.PUNTUACION),
            new AttributesDescriptor("Identificador", NookiSyntaxHighlighter.IDENTIFICADOR),
    };

    private static final String EJEMPLO = """
            // Un programa de Nooki
            usar matematicas

            estructura punto {
                publico x: entero
                publico y: entero
            }

            funcion principal() -> entero {
                variable saludo = "hola"
                variable n = 42
                constante pi = 3.1416

                si (n > 0 && verdadero) {
                    imprimir(saludo)
                }
                retornar 0
            }
            """;

    @Override
    public @NotNull String getDisplayName() {
        return "Nooki";
    }

    @Override
    public @Nullable Icon getIcon() {
        return null;
    }

    @Override
    public @NotNull AttributesDescriptor[] getAttributeDescriptors() {
        return ATRIBUTOS;
    }

    @Override
    public ColorDescriptor @NotNull [] getColorDescriptors() {
        return ColorDescriptor.EMPTY_ARRAY;
    }

    @Override
    public @NotNull SyntaxHighlighter getHighlighter() {
        return new NookiSyntaxHighlighter();
    }

    @Override
    public @NotNull String getDemoText() {
        return EJEMPLO;
    }

    @Override
    public @Nullable Map<String, TextAttributesKey> getAdditionalHighlightingTagToDescriptorMap() {
        return null;
    }
}
