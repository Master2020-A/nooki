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
 * La página de colores: permite ver y cambiar cada color de Nooki desde
 * Settings → Editor → Color Scheme → Nooki.
 */
public final class NookiColorPage implements ColorSettingsPage {

    private static final AttributesDescriptor[] ATRIBUTOS = new AttributesDescriptor[]{
            new AttributesDescriptor("Comentario de línea", NookiSyntaxHighlighter.COMENTARIO),
            new AttributesDescriptor("Comentario de bloque", NookiSyntaxHighlighter.COMENTARIO_BLOQUE),
            new AttributesDescriptor("Texto", NookiSyntaxHighlighter.TEXTO),
            new AttributesDescriptor("Número", NookiSyntaxHighlighter.NUMERO),
            new AttributesDescriptor("Función (declaración)", NookiSyntaxHighlighter.FUNCION),
            new AttributesDescriptor("Variable y constante", NookiSyntaxHighlighter.VARIABLE),
            new AttributesDescriptor("Tipo declarado", NookiSyntaxHighlighter.TIPO_DECLARADO),
            new AttributesDescriptor("Control de flujo", NookiSyntaxHighlighter.CONTROL),
            new AttributesDescriptor("Patrones", NookiSyntaxHighlighter.PATRON),
            new AttributesDescriptor("Módulos", NookiSyntaxHighlighter.MODULO),
            new AttributesDescriptor("Inseguro y asm", NookiSyntaxHighlighter.META),
            new AttributesDescriptor("Modificador de acceso", NookiSyntaxHighlighter.MODIFICADOR),
            new AttributesDescriptor("Objeto", NookiSyntaxHighlighter.OBJETO),
            new AttributesDescriptor("Tipo primitivo", NookiSyntaxHighlighter.TIPO_PRIMITIVO),
            new AttributesDescriptor("Tipo genérico", NookiSyntaxHighlighter.TIPO_GENERICO),
            new AttributesDescriptor("Literal", NookiSyntaxHighlighter.LITERAL),
            new AttributesDescriptor("Llamada a función", NookiSyntaxHighlighter.LLAMADA),
            new AttributesDescriptor("Método", NookiSyntaxHighlighter.METODO),
            new AttributesDescriptor("Campo", NookiSyntaxHighlighter.CAMPO),
            new AttributesDescriptor("Nombre de campo en un literal", NookiSyntaxHighlighter.ETIQUETA),
            new AttributesDescriptor("Identificador", NookiSyntaxHighlighter.IDENTIFICADOR),
            new AttributesDescriptor("Operador", NookiSyntaxHighlighter.OPERADOR),
            new AttributesDescriptor("Referencia y propagación", NookiSyntaxHighlighter.REFERENCIA),
            new AttributesDescriptor("Puntuación", NookiSyntaxHighlighter.LLAVE),
            new AttributesDescriptor("Punto", NookiSyntaxHighlighter.PUNTO),
            new AttributesDescriptor("Coma", NookiSyntaxHighlighter.COMA),
    };

    private static final String EJEMPLO = """
            // Un poco de todo: cada cosa de un color
            usar matematicas
            usar pln

            estructura punto {
                publico x: entero
                publico y: entero
            }

            rasgo Describible {
                funcion describir() -> texto
            }

            clase figura implementa Describible {
                publico nombre: texto

                publico funcion describir() -> texto {
                    retornar "figura " + este.nombre
                }
            }

            funcion distancia(a: punto, b: punto) -> entero {
                variable dx = a.x - b.x
                variable dy = a.y - b.y
                constante cero = 0
                imprimir(matematicas.cuadrado(dx) + matematicas.cuadrado(dy))
                retornar cero
            }

            funcion principal() -> entero {
                variable p = punto { x: 1, y: 2 }
                variable q = punto { x: 4, y: 6 }
                variable nombre = "Nooki"
                variable largo = 1_000

                si (largo > 0 && verdadero) {
                    mientras (largo > 990) {
                        largo = largo - 1
                    }
                }

                coincide (largo) {
                    caso 0 => imprimir("cero")
                    caso _ => imprimir("otro")
                }

                variable caja = Caja<entero>.nuevo(0)
                variable referencia: &mutable Caja<entero> = &mutable caja
                variable texto = \"\"\"línea uno
                línea dos\"\"\"

                imprimir(distancia(p, q))
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
