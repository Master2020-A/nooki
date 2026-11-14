package dev.nooki.plugin;

import com.intellij.psi.tree.IElementType;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/** Los tipos de token del lenguaje y las palabras que los producen. */
public final class NookiTokens {

    private NookiTokens() {
    }

    public static final IElementType COMENTARIO =
            new IElementType("COMENTARIO", NookiLanguage.INSTANCIA);
    public static final IElementType TEXTO =
            new IElementType("TEXTO", NookiLanguage.INSTANCIA);
    public static final IElementType TEXTO_MULTILINEA =
            new IElementType("TEXTO_MULTILINEA", NookiLanguage.INSTANCIA);
    public static final IElementType NUMERO =
            new IElementType("NUMERO", NookiLanguage.INSTANCIA);
    public static final IElementType DECLARACION =
            new IElementType("DECLARACION", NookiLanguage.INSTANCIA);
    public static final IElementType CONTROL =
            new IElementType("CONTROL", NookiLanguage.INSTANCIA);
    public static final IElementType TIPO =
            new IElementType("TIPO", NookiLanguage.INSTANCIA);
    public static final IElementType GENERICO =
            new IElementType("GENERICO", NookiLanguage.INSTANCIA);
    public static final IElementType VISIBILIDAD =
            new IElementType("VISIBILIDAD", NookiLanguage.INSTANCIA);
    public static final IElementType OBJETO =
            new IElementType("OBJETO", NookiLanguage.INSTANCIA);
    public static final IElementType META =
            new IElementType("META", NookiLanguage.INSTANCIA);
    public static final IElementType LITERAL =
            new IElementType("LITERAL", NookiLanguage.INSTANCIA);
    public static final IElementType OPERADOR =
            new IElementType("OPERADOR", NookiLanguage.INSTANCIA);
    public static final IElementType PUNTUACION =
            new IElementType("PUNTUACION", NookiLanguage.INSTANCIA);
    public static final IElementType IDENTIFICADOR =
            new IElementType("IDENTIFICADOR", NookiLanguage.INSTANCIA);
    public static final IElementType ESPACIO =
            new IElementType("ESPACIO", NookiLanguage.INSTANCIA);
    public static final IElementType DESCONOCIDO =
            new IElementType("DESCONOCIDO", NookiLanguage.INSTANCIA);

    /** Palabra → tipo de token, para las que se distinguen. */
    public static final Map<String, IElementType> PALABRAS = new HashMap<>();

    static {
        for (String palabra : new String[]{
                "funcion", "variable", "constante", "clase", "estructura", "rasgo", "interfaz",
                "enumeracion", "tipo", "modulo", "importar", "exportar", "implementar",
                "externo"}) {
            PALABRAS.put(palabra, DECLARACION);
        }
        for (String palabra : new String[]{
                "si", "sino", "mientras", "para", "en", "hacer", "romper", "continuar",
                "retornar", "coincide", "caso", "defecto"}) {
            PALABRAS.put(palabra, CONTROL);
        }
        for (String palabra : new String[]{
                "entero", "flotante", "booleano", "texto", "caracter", "vacio", "lista",
                "tamano", "esperar", "nulo"}) {
            PALABRAS.put(palabra, TIPO);
        }
        for (String palabra : new String[]{"Vector", "Mapa", "Caja", "resultado"}) {
            PALABRAS.put(palabra, GENERICO);
        }
        for (String palabra : new String[]{"publico", "privado", "protegido", "interna",
                "mutable"}) {
            PALABRAS.put(palabra, VISIBILIDAD);
        }
        for (String palabra : new String[]{"nuevo", "este", "Este", "super", "hereda",
                "implementa"}) {
            PALABRAS.put(palabra, OBJETO);
        }
        for (String palabra : new String[]{"usar", "inseguro", "asm"}) {
            PALABRAS.put(palabra, META);
        }
        for (String palabra : new String[]{"verdadero", "falso"}) {
            PALABRAS.put(palabra, LITERAL);
        }
    }

    /** Palabras que no pueden ser nombres. */
    public static final Set<String> RESERVADAS = new HashSet<>(PALABRAS.keySet());

    /** ¿Es un carácter que puede empezar un identificador? */
    public static boolean empiezaIdentificador(char caracter) {
        return Character.isLetter(caracter) || caracter == '_';
    }

    /** ¿Es un carácter que puede continuar un identificador? */
    public static boolean continuaIdentificador(char caracter) {
        return Character.isLetterOrDigit(caracter) || caracter == '_';
    }
}
