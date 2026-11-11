# Licencia de Nooki

Nooki se distribuye bajo **doble licencia** (decisión [ADR-012](docs/adr/ADR-012-licencia-y-gobernanza.md)):

```
SPDX-License-Identifier: MIT OR Apache-2.0
```

Puedes elegir, para cada uso, la licencia que prefieras:

- **MIT** ([texto completo](LICENCIA-MIT.txt)): permisiva y simple.
- **Apache-2.0** ([texto completo](LICENCIA-APACHE.txt)): permisiva con concesión explícita de
  patentes, preferida por muchas empresas.

## Excepción de runtime

El **código generado** por el compilador Nooki (ejecutables, bibliotecas y archivos objeto) y las
bibliotecas que ese código enlace **no** quedan cubiertos por estas licencias ni obligan a publicar
su código fuente. Esta excepción es la misma que aplican LLVM y Rust, y es imprescindible para que
Nooki pueda usarse en software propietario.

## Código de terceros

Las dependencias de terceros conservan sus propias licencias. Se declaran en
[`TERCEROS.md`](TERCEROS.md) cuando se incorporen (LLVM llegará en el Sprint 5).

---

## Texto de la licencia MIT

Copyright (c) 2026 Autor de Nooki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## Pendiente

El texto completo de **Apache-2.0** se incorporará como `LICENSE-APACHE` antes de la primera
publicación pública del proyecto (Sprint 94). Registrado como asunto **A-06** en
[DECISIONES.md](DECISIONES.md).
