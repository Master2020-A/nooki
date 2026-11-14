package dev.nooki.plugin;

import com.intellij.lexer.Lexer;
import com.intellij.psi.tree.IElementType;

/**
 * Comprobación del lexer fuera del IDE.
 *
 * Imprime los tokens de un fragmento de Nooki. Sirve para verificar que el
 * resaltado acertará sin tener que abrir el IDE:
 *
 *   java -cp "Nooki-1.0.0.jar;<jars del IDE>" dev.nooki.plugin.DepurarLexer
 */
public final class DepurarLexer {

    private static final String EJEMPLO = """
            // Un comentario con tilde: así
            usar matematicas

            estructura punto {
                publico x: entero
                publico nombre: texto
            }

            funcion principal() -> entero {
                variable saludo = "hola, mundo"
                variable largo = \"\"\"primera
                segunda\"\"\"
                variable n = 1_000
                variable pi = 3.1416
                constante cero = 0x1F
                si (n > 0 && verdadero) {
                    imprimir(saludo)
                }
                retornar 0
            }
            """;

    public static void main(String[] argumentos) {
        final Lexer lexer = new NookiLexer();
        lexer.start(EJEMPLO, 0, EJEMPLO.length(), 0);

        int cuantos = 0;
        int errores = 0;
        while (lexer.getTokenType() != null) {
            final IElementType tipo = lexer.getTokenType();
            final String texto = lexer.getTokenText();
            if (tipo == NookiTokens.DESCONOCIDO) {
                ++errores;
                System.out.println("DESCONOCIDO: [" + texto + "]");
            }
            if (tipo != NookiTokens.ESPACIO) {
                ++cuantos;
                System.out.printf("%-16s %s%n", tipo.toString(), texto.replace("\n", "\\n"));
            }
            lexer.advance();
        }

        System.out.println();
        System.out.println("tokens: " + cuantos + " · desconocidos: " + errores);

        // Lo que el usuario notó: «funcion» y «variable» salían del mismo color.
        final IElementType deFuncion = NookiTokens.PALABRAS.get("funcion");
        final IElementType deVariable = NookiTokens.PALABRAS.get("variable");
        if (deFuncion == deVariable) {
            System.out.println("FALLO: funcion y variable comparten tipo de token");
            System.exit(1);
        }
        System.out.println("funcion -> " + deFuncion + " · variable -> " + deVariable);

        if (errores > 0) {
            System.exit(1);
        }
    }
}
