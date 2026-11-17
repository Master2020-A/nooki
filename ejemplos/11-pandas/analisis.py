"""Análisis con pandas, llamado desde un programa de Nooki.

Recibe el CSV de entrada y el de salida por la línea de comandos, agrupa por
ciudad y deja el resumen. Este archivo es Python normal: se puede probar solo.

    python analisis.py ventas.csv resumen.csv
"""

from __future__ import annotations

import sys

try:
    import pandas as pd
except ImportError:  # pragma: no cover - solo si falta pandas
    print("falta pandas: instálelo con «pip install pandas»", file=sys.stderr)
    raise SystemExit(2) from None


def main() -> int:
    if len(sys.argv) != 3:
        print("uso: analisis.py <entrada.csv> <salida.csv>", file=sys.stderr)
        return 64

    entrada, salida = sys.argv[1], sys.argv[2]

    datos = pd.read_csv(entrada, sep=";")
    datos["importe"] = datos["unidades"] * datos["precio"]

    resumen = (
        datos.groupby("ciudad", as_index=False)
        .agg(unidades=("unidades", "sum"), importe=("importe", "sum"))
        .sort_values("importe", ascending=False)
    )
    resumen.to_csv(salida, sep=";", index=False)

    print(f"{len(datos)} filas analizadas, {len(resumen)} ciudades")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
