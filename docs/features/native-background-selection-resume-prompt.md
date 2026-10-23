# Goal: reconstruir o núcleo de seleção de fundos em C++23

Implemente e entregue, em `/Users/victordmor/Projects/cm0102mac`, as quatro rotinas relacionadas de seleção de imagens de fundo do CM 01/02 Mac em C++23 semântico, mantendo as escolhas, os fallbacks e o consumo de RNG originais. Converse em português; código, testes, documentação técnica e novas mensagens do aplicativo ficam em inglês.

## Direção de produto e decisões atuais

O jogo já funciona em ARM64. O objetivo maior é substituir gradualmente a lógica recompilada por C++ compreensível sem mudar o jogo. A etapa de apresentação foi concluída: escala ajustável, textos e controles nativos em Retina e comandos por menus/atalhos. O usuário aprovou a aparência. Retome agora o núcleo de seleção de fundos, adiado para aquela entrega, como um único bloco coerente. O benefício é tornar esse trecho do engine legível e modificável, preservando a navegação familiar por clube, competição e telas genéricas; não prometa um ganho visual ou de desempenho.

**A orientação de testes mudou em 5 October 2026.** O usuário considerou excessivos os testes na aplicação e dispensou testes de carregamento e salvamento: ele avisará se encontrar bugs no uso. Este pedido substitui as exigências de aceitação do antigo `docs/features/native-background-selection-reconstruction-prompt.md` e de relatos anteriores:

- Não execute testes de Save/Load, ciclos de salvar/sair/reabrir/carregar, comparação de carreiras persistidas ou auditoria de serialização como gate desta entrega.
- Faça verificações focadas no código alterado. Não rode a suíte inteira, todas as famílias Python, campanhas Date/RNG, replays day/week ou rodadas repetidas de UI por rotina. Amplie somente para uma falha concreta ou dependência efetivamente afetada, com motivo explícito.
- Não peça autorização de novo para dispensar esses testes. Registre simplesmente que persistência não foi retestada por decisão do usuário, sem marcar como PASS.
- Pare de testar quando os critérios delimitados abaixo estiverem demonstrados. Priorize implementação e entrega, com evidência compacta.

Este arquivo é autocontido. O prompt antigo permanece preservado como histórico; não combine sua matriz de testes com esta.

## Ponto de partida verificado

Na preparação de 5 October 2026:

- Branch `main`, HEAD e consulta nova a `origin/main`: `7b812d530ad8331102af1277a8e20aab8775ac05`.
- Nenhuma alteração em arquivos versionados ou no índice. Dez handoffs anteriores estavam untracked em `docs/features`; este é o décimo primeiro. Preserve todos e mantenha-os fora do commit de produto.
- Não foi encontrado `AGENTS.md` aplicável nos ancestrais ou no projeto inspecionados. Reconfira instruções, branch, remoto e diff antes de trabalhar; o estado pode mudar.
- `.local/recompiled/compilation.json` tem 37 `semanticReplacements`: 34 Date e três RNG. As quatro entradas abaixo continuam geradas. Há 1.922 páginas, 1.953.157 instruções e nenhuma `missingStaticTargets`.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `~/Applications/CM 01-02 Native Prototype.app`, executável interno `Contents/MacOS/CM 01-02 Native Prototype`: ambos ARM64, SHA-256 `e6907400e9aa29896c061af4144efa99d5141fe39f119a025830f5b72bb24c41`.
- `docs/native-presentation-and-direct-commands.md` registra 44/44 CTest na entrega anterior e aceitação visual. Esses resultados são históricos para este novo trabalho. Nesta preparação não se executou teste, build, aplicativo, instalação ou implementação.

## Inspeção necessária

Leia `README.md`, a seção mais recente de `docs/native-port-status.md`, `docs/native-rng-core-reconstruction.md` e os contratos de integração em `docs/native-presentation-and-direct-commands.md`. Use os documentos como descrição técnica, sem importar campanhas antigas de aceitação.

Inspecione principalmente:

- `native/background_checks.cpp`: harness existente de seleção padrão/clube/competição, estado RNG e faults de getters nulos.
- `native/rng_core.hpp`, `native/rng_core_adapter.*`, `native/rng_core_checks.cpp`, `native/aot_runtime.*` e `native/callbacks.*`: contratos Mac, estado e padrões de integração/comparação.
- `tools/compile_recomp.py`, `tools/rng_core_audit.py`, `tools/test_rng_core_audit.py`, `tools/calendar_audit.py` e `native/CMakeLists.txt`: hooks nos destinos gerados, fingerprints, manifesto e checks reais.
- `native/native_drawing.*`, `native/ui_bridge.*` apenas nas interfaces afetadas: preserve a observação de desenho e os comandos recém-entregues. Não ocupe ou substitua um observador existente silenciosamente.
- `.local/recomp-input/instructions.sqlite`, somente leitura, e as páginas geradas correspondentes. Não refaça Ghidra/oracle; o export atual já existe. Não publique instruções, recursos ou payloads proprietários.

## Escopo e contratos

Reconstrua exatamente este núcleo, com 317 instruções indexadas, conferidas novamente nesta preparação:

| Entrada | Fim exclusivo | Instruções | Responsabilidade |
| --- | --- | ---: | --- |
| `0x107122e0` | `0x107124e0` | 128 | Escolher no catálogo/árvore e delegar à lista principal ou alternativa |
| `0x10712a60` | `0x10712c4c` | 123 | Escolher um nó da lista e retornar seu payload |
| `0x10713960` | `0x107139b4` | 21 | Selecionar fundo padrão por categoria |
| `0x107139c0` | `0x10713a74` | 45 | Selecionar por categoria e objeto/descritor, ou usar o padrão |

Fingerprints SHA-256, normalização `address/assembly/ops/flows/fallthrough` usada pelos audits existentes:

- `0x107122e0`: `1a92a426b5789bed300658ff087bfcebd05ffd229e192c3c9b97a84da5b0578b`.
- `0x10712a60`: `074912d3cb6b49880517225d7dd83dfd5c0d8a22250def5792f745fb8cf368b8`.
- `0x10713960`: `1efc413b607f6cc6c32cd45329b54d34b846e968d5f495aeca2b8e6f42bd85a7`.
- `0x107139c0`: `c206b64c4f3b65e30292264dd4a3ea84ad30b8a794315e2e659ed496a2ed7435`.

Caracterize os efeitos reais no export antes de implementar. Preserve a ABI Mac big-endian de 32 bits, registradores/flags afetados, TOC, PC/LR/CTR, stack/spills, memória, ordem de acessos, aliases relevantes e faults. Use lógica semântica com nomes claros e um adapter mínimo; não entregue uma tradução mecânica instrução por instrução nem wrappers que ainda executem os quatro corpos originais.

Pontos de contrato que não podem se perder:

1. `0x107122e0` recebe coleção em r3 e seletor em r4. A contagem de 32 bits em `+12` é reduzida a signed16. Nulo/contagem efetiva zero retorna zero. O acesso por `0x1074df60` e o payload do nó em `+4` levam à lista principal ou à lista em `+12`, conforme o byte efetivo do seletor; a segunda seleção passa por `0x10712a60`.
2. `0x10712a60` usa a contagem de 32 bits em `+8`, também reduzida a signed16, chama o accessor `0x1074dd40` e retorna o payload do nó em `+0`, ou zero no miss. Os accessors permanecem gerados.
3. Ambos usam o pool unsigned16 big-endian. O cursor avança antes da amostra; o limite é inclusivo; wrap retorna à base e renova salt pelo RNG Mac já semântico. Preserve também o consumo quando uma busca posterior falha e os caminhos que retornam antes de consumir. Os slots relativos a r2 são `-0x7b90` (ponteiro para salt), `-0x7b8c` (base), `-0x7b88` (ponteiro para limite) e `-0x7b84` (ponteiro para cursor). Não introduza gerador ou estado aleatório paralelo.
4. `0x10713960` usa o flag em `r2-0x35d4`, a tabela em `r2-0x5c3c` e a máscara original de categoria. `0x107139c0` tem sua própria extensão signed16 do índice; não unifique os cálculos cegamente. Fundos desabilitados/categoria ausente retornam zero. Objeto nulo ignora o descritor e usa a seleção padrão; objeto presente busca via `0x1074df90` e depois seleciona a lista.
5. Preserve os null guards já existentes nos call sites `0x10713aec` e `0x10713b3c`. Os wrappers de clube/competição devem continuar evitando só os getters indevidos no fallback nulo; faults independentes não devem virar sucesso.

O sampler `0x107124e0`, wrappers `0x10713a80` a `0x10713c10`, busca/comparação de árvores, carregamento/liberação de catálogos, aplicação/desenho de imagens e consumidores RNG fora das quatro entradas continuam gerados. Não amplie o escopo para ramos sem alcançabilidade comprovada. Preserve o comportamento assimétrico bissexto de Date.

Integre as quatro substituições nos **labels de destino** das páginas `0x10712` e `0x10713`, incluindo chamadas/gotos na mesma página, dispatch indireto e callbacks. A produção normal deve executar as quatro implementações sem fallback aos corpos originais. O modo original de comparação, se necessário, será privado e independente de Date/RNG. O manifesto final deve conter 41 entradas, preservando as 37 existentes.

Ficam fora: imagens HD, mudanças de fonte/layout, filtros, correções de gameplay, RNG/Date novos, multiplayer, serialização, reconstrução do Restart e melhorias gerais de menus. Preserve textos/controles nativos, escala, atalhos e comportamento já aprovado.

## Validação focada e critério de parada

Use um harness compartilhado para o bloco e os checks existentes. Compare original e semântico com o mesmo estado inicial, cobrindo cada entrada e composição, resultado escolhido, estado RNG e efeitos da ABI/memória. Inclua casos representativos de nulo/vazio, um/vários itens, principal/alternativa, enable flag, match/miss de descritor, fronteiras signed16, cursor final/wrap, TOC relocável e aliases que possam alterar a ordem observável. Os casos devem detectar erros de contrato, não apenas reproduzir a implementação.

Inclua um negativo próprio por entrada, que detecte sua divergência, e verifique os hooks gerados contra mudanças de fingerprint. Não use a implementação nova como oracle. Demonstre seleção não vazia e que as quatro entradas são alcançadas pelo dispatch de produção no harness; contagem de manifesto sozinha não prova integração. Diferencie essa evidência de execução no aplicativo.

Reutilize `original_background_selection` e `modern_rng_core_equivalence`. Rode os novos checks e os testes Python do audit/hook alterado; o audit RNG existente é relevante porque verifica a página consumidora. Se a alteração atingir uma interface compartilhada de desenho ou State, acrescente somente a regressão correspondente. UBSAN focado é útil para aritmética/adapter novo; não instrumente novamente o engine inteiro por rotina.

Comandos existentes, a partir da raiz, para orientar a execução após as alterações:

```sh
# Regenere somente quando os novos hooks/manifesto estiverem prontos.
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure \
  -R '^(original_background_selection|modern_rng_core_equivalence)$'
python3 tools/test_rng_core_audit.py
```

Registre e execute também o novo target/harness com o nome efetivamente criado. Faça um build consistente; não repita `clean-first`, testes já verdes ou geração sem mudança que os justifique. Não herde a suíte completa como requisito do antigo prompt.

Uma inspeção curta no app pode verificar fundos e ausência de faults nas telas diretamente afetadas, em runtime privado, se acrescentar evidência. Não transforme isso em exploração de carreira ou exigência de todas as rotinas terem contadores vivos antes de entregar. Para estados de clube/competição, prefira fixtures já disponíveis; não crie testes de carregamento/salvamento para obtê-los. Se faltar evidência real de uma entrada, relate precisamente esse limite, mantendo a comparação direta e a integração verificadas. Não invente coverage. A confirmação de bugs de Save/Load fica com o usuário.

Conclua a validação quando o build passar, as quatro comparações e seus negativos funcionarem, o dispatch usar as implementações novas e as regressões focadas passarem. Uma falha concreta exige corrigir e revalidar o afetado; ausência de falha não exige outra rodada.

## Proteção e entrega

Não toque nos saves pessoais, fixtures, inputs originais ou candidatos/referências históricos. Testes e diagnósticos ficam sob `.local`, com evidência compacta e sem novas cópias de carreiras. Não refaça inventários globais de centenas de arquivos a cada ajuste. Antes da instalação, faça uma referência privada do instalado atual e a conferência necessária de conteúdo/metadados dos arquivos pessoais potencialmente afetados; após instalar, confira sua preservação uma vez. Use as proteções do instalador existente.

Documente em inglês em `docs/native-background-selection-reconstruction.md` os quatro contratos, integração, dependências ainda geradas, casos/negativos, checks efetivamente executados e limites. Atualize `docs/native-port-status.md`. Declare que Save/Load não foi retestado por orientação do usuário; não o liste como pendência obrigatória nem como aprovado nesta entrega.

Após concluir a implementação e os checks focados, instale uma vez com `python3 tools/install_native.py`, conforme a autorização de entrega do projeto. Confira ARM64, hash do executável fonte/instalado, payloads originais, plist sem diagnósticos e horário efetivo da instalação. Uma cópia assinada para inspeção privada pode ter hash diferente; distinga as identidades. Não abra a carreira pessoal para validar a instalação.

Revise o diff exato, preserve alterações alheias e faça commit/push normal sob a autorização permanente para trabalho concluído e validado. Confirme o SHA remoto por consulta nova. Exclua os onze handoffs, saves, payloads, screenshots e logs privados; não mude ignore/LFS para publicá-los. Não apresente essas quatro rotinas como reconstrução de todo o engine.

A sessão que prepara este handoff apenas salva e copia o prompt. Implementação, instalação e publicação pertencem à sessão em que este pedido for executado.
