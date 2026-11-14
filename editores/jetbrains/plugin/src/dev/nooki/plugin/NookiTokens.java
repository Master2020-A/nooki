package dev.nooki.plugin;

import com.intellij.psi.tree.IElementType;
import java.util.HashMap;
import java.util.Map;

/**
 * Los tipos de token del lenguaje y las palabras que los producen.
 *
 * Hay más categorías de las que parecen necesarias porque cada una puede llevar
 * un color distinto: no es lo mismo declarar una función que una variable.
 */
public final class NookiTokens {

    private NookiTokens() {
    }

    // Texto y comentarios.
    public static final IElementType COMENTARIO = nuevo("COMENTARIO");
    public static final IElementType COMENTARIO_BLOQUE = nuevo("COMENTARIO_BLOQUE");
    public static final IElementType TEXTO = nuevo("TEXTO");
    public static final IElementType TEXTO_MULTILINEA = nuevo("TEXTO_MULTILINEA");
    public static final IElementType ESCAPE = nuevo("ESCAPE");
    public static final IElementType NUMERO = nuevo("NUMERO");

    // Palabras del lenguaje, cada grupo con su color.
    public static final IElementType PALABRA_FUNCION = nuevo("PALABRA_FUNCION");
    public static final IElementType PALABRA_VARIABLE = nuevo("PALABRA_VARIABLE");
    public static final IElementType PALABRA_TIPO = nuevo("PALABRA_TIPO");
    public static final IElementType PALABRA_CONTROL = nuevo("PALABRA_CONTROL");
    public static final IElementType PALABRA_PATRON = nuevo("PALABRA_PATRON");
    public static final IElementType PALABRA_MODULO = nuevo("PALABRA_MODULO");
    public static final IElementType PALABRA_OBJETO = nuevo("PALABRA_OBJETO");
    public static final IElementType PALABRA_META = nuevo("PALABRA_META");
    public static final IElementType MODIFICADOR = nuevo("MODIFICADOR");

    // Nombres.
    public static final IElementType NOMBRE_FUNCION = nuevo("NOMBRE_FUNCION");
    public static final IElementType NOMBRE_TIPO = nuevo("NOMBRE_TIPO");
    public static final IElementType TIPO_PRIMITIVO = nuevo("TIPO_PRIMITIVO");
    public static final IElementType TIPO_GENERICO = nuevo("TIPO_GENERICO");
    public static final IElementType LITERAL = nuevo("LITERAL");
    public static final IElementType LLAMADA = nuevo("LLAMADA");
    public static final IElementType METODO = nuevo("METODO");
    public static final IElementType CAMPO = nuevo("CAMPO");
    public static final IElementType ETIQUETA = nuevo("ETIQUETA");
    public static final IElementType IDENTIFICADOR = nuevo("IDENTIFICADOR");

    // Símbolos.
    public static final IElementType OPERADOR = nuevo("OPERADOR");
    public static final IElementType REFERENCIA = nuevo("REFERENCIA");
    public static final IElementType PUNTUACION = nuevo("PUNTUACION");
    public static final IElementType PUNTO = nuevo("PUNTO");
    public static final IElementType COMA = nuevo("COMA");
    public static final IElementType ESPACIO = nuevo("ESPACIO");
    public static final IElementType DESCONOCIDO = nuevo("DESCONOCIDO");

    private static IElementType nuevo(String nombre) {
        return new IElementType(nombre, NookiLanguage.INSTANCIA);
    }

    /** Palabra → tipo de token. */
    public static final Map<String, IElementType> PALABRAS = new HashMap<>();

    private static void anadir(IElementType tipo, String... palabras) {
        for (String palabra : palabras) {
            PALABRAS.put(palabra, tipo);
        }
    }

    static {
        anadir(PALABRA_FUNCION, "funcion");
        anadir(PALABRA_VARIABLE, "variable", "constante");
        anadir(PALABRA_TIPO, "clase", "estructura", "rasgo", "interfaz", "enumeracion", "tipo");
        anadir(PALABRA_CONTROL, "si", "sino", "mientras", "para", "en", "hacer", "romper",
                "continuar", "retornar");
        anadir(PALABRA_PATRON, "coincide", "caso", "defecto");
        anadir(PALABRA_MODULO, "modulo", "usar", "importar", "exportar", "externo",
                "implementar");
        anadir(PALABRA_OBJETO, "nuevo", "este", "Este", "super", "hereda", "implementa");
        anadir(PALABRA_META, "inseguro", "asm");
        anadir(MODIFICADOR, "publico", "privado", "protegido", "interna", "mutable");
        anadir(TIPO_PRIMITIVO, "entero", "flotante", "booleano", "texto", "caracter", "vacio",
                "lista", "tamano", "esperar");
        anadir(TIPO_GENERICO, "Vector", "Mapa", "Caja", "resultado");
        anadir(LITERAL, "verdadero", "falso", "nulo");
    }

    /** Las palabras que abren una declaración con nombre detrás. */
    public static boolean abreNombreDeTipo(String palabra) {
        return switch (palabra) {
            case "clase", "estructura", "rasgo", "interfaz", "enumeracion" -> true;
            default -> false;
        };
    }

    public static boolean empiezaIdentificador(char caracter) {
        return Character.isLetter(caracter) || caracter == '_';
    }

    public static boolean continuaIdentificador(char caracter) {
        return Character.isLetterOrDigit(caracter) || caracter == '_';
    }
}
