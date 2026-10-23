# Goal: reconstruir o núcleo de consultas de coleções do Mac em C++23

Implemente e entregue, em `/Users/victordmor/Projects/cm0102mac`, as cinco entradas relacionadas de indexação de listas e consulta de árvores do CM 01/02 Mac descritas abaixo. Substitua sua execução gerada por C++23 semântico em produção, preservando os resultados, a ordem das consultas e os efeitos originais. Converse em português; código, testes, documentação técnica e novas mensagens do aplicativo ficam em inglês.

## Produto e prioridade

O jogo já funciona nativamente em ARM64. O objetivo maior é substituir gradualmente o engine recompilado por C++ compreensível, mantendo o comportamento original. Date, o núcleo de estado/inicialização RNG, a apresentação Retina com escala e comandos diretos e o núcleo de seleção de backgrounds já foram entregues. A aparência foi aprovada; este goal avança na lógica do engine.

Reconstrua um único bloco funcional: acessar um nó de lista por índice, acessar um nó de árvore por posição e buscar um nó pelo comparador original. Essas consultas são dependências efetivamente usadas pelos seletores de backgrounds recém-entregues; a busca também possui callers gerados de carregamento e de outro consumidor de catálogo. Por exemplo, escolher uma imagem principal ou alternativa e buscar o background de um objeto devem continuar encontrando os mesmos nós e consumindo as mesmas amostras RNG, agora com consultas de coleções semânticas.

Este bloco é preferível a portar somente wrappers triviais ou o sampler RNG `0x107124e0`: os dois callers externos conhecidos desse sampler estão em ramos incompatíveis com a redução signed16 dos seletores, e o terceiro call é interno. Isso não prova ausência de callers dinâmicos desconhecidos, mas não oferece um caminho produtivo conhecido para priorizá-lo agora. Mantenha o sampler gerado. O benefício deste goal é legibilidade e continuidade da reconstrução; não prometa ganho visual ou de desempenho.

## Decisões atuais de validação

Em 5 October 2026 o usuário dispensou testes de carregamento e salvamento e pediu verificações proporcionais ao código alterado; ele avisará sobre bugs encontrados durante o uso. Essa decisão prevalece sobre matrizes históricas de handoffs e relatórios:

- Não execute Save/Load, ciclos de salvar/sair/reabrir/carregar, comparações de carreiras persistidas ou auditoria de serialização como gate.
- Não rode a suíte inteira, todas as famílias Python, campanhas Date/RNG, replays day/week ou UI repetida por rotina. Amplie somente por uma falha concreta ou dependência realmente afetada, explicando o motivo.
- Não peça nova autorização para dispensar esses testes. Registre persistência como não retestada por decisão do usuário, sem PASS nem pendência obrigatória.
- Pare quando o build consistente, a comparação focada, os negativos próprios, o dispatch e as regressões diretamente relacionadas estiverem demonstrados. Uma falha exige corrigir e revalidar o afetado; checks verdes não exigem outra campanha.

Este prompt é autocontido. Não combine sua aceitação com exigências antigas de Date, RNG, apresentação ou backgrounds.

## Ponto de partida verificado em 5 October 2026

- Projeto em `main`; HEAD e `origin/main`: `b2b696610f4b993f818c517d0bd6b0c81d599bd3`, `Reconstruct Mac background selection core in semantic C++23`. Remoto configurado: `git@github.com:VictorDMor/cm0102mac.git`. Reconfira branch, remoto, estado remoto, diff e instruções antes de trabalhar.
- Não havia alterações versionadas ou staged. Onze handoffs anteriores estavam untracked em `docs/features`: dois de backgrounds, sete de Date, um de apresentação e um de RNG. Preserve todos e este novo handoff; exclua-os do commit de produto. Não foi encontrado `AGENTS.md` aplicável nos ancestrais e no projeto inspecionados; reconfira.
- `.local/recompiled/compilation.json` registra 41 `semanticReplacements`: 34 Date, três RNG e quatro seletores de backgrounds. Há 1.922 páginas, 1.953.157 instruções indexadas e nenhuma `missingStaticTargets`. As cinco entradas deste goal permanecem geradas. O manifesto final esperado é 46, preservando as 41 existentes.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`, executável `Contents/MacOS/CM 01-02 Native Prototype`: ambos ARM64 e SHA-256 `c5850fc5cb5ba228fbd30d2251bab27f386e5644f9452473539097d09b42a11a`. A última instalação registrada ocorreu uma vez em 5 October, 18:13:23 UTC, 15:13 no fuso de São Paulo. Reconfira identidades; o mtime da pasta `.app` não prova sua versão.
- `docs/native-background-selection-reconstruction.md` registra build consistente, 163 comparações diretas, 18 faults, quatro negativos próprios, dispatch não vazio das quatro entradas, callbacks/transições indiretas, regressão original de backgrounds, comparação RNG existente, auditorias afetadas e UBSAN apenas do adapter/harness novos. São resultados da entrega anterior; não constituem aceitação destas cinco consultas. Nenhuma sessão do app foi aberta naquela entrega e Save/Load não foi retestado.
- Esta preparação somente inspecionou o projeto, escolheu o bloco e criou o handoff. Não implementou, executou build/testes, abriu o jogo, instalou ou publicou uma nova versão.

## Inspeção necessária

Leia `README.md`, a seção mais recente de `docs/native-port-status.md` e `docs/native-background-selection-reconstruction.md`. Consulte os contratos de integração de `docs/native-presentation-and-direct-commands.md` e `docs/native-rng-core-reconstruction.md` apenas onde a dependência exigir, sem herdar suas campanhas de aceitação.

Inspecione principalmente:

- `native/background_selection_adapter.*`, `native/background_selection_checks.cpp` e `native/background_checks.cpp`: consumidores já semânticos, fixtures de listas/árvores/descritores, equivalência e regressão de fallback.
- `native/aot_runtime.*`, `native/callbacks.*`, `native/aot_probe.cpp` e `native/CMakeLists.txt`: ABI, dispatch, transition vectors, comparações, diagnósticos e targets existentes.
- `tools/compile_recomp.py`, `tools/background_selection_audit.py`, `tools/test_background_selection_audit.py` e `tools/calendar_audit.py`: manifesto, hooks nos destinos e normalização de fingerprints.
- `.local/recomp-input/instructions.sqlite`, em conexão read-only, e páginas geradas privadas `0x1074d` e `0x1074e`; consulte callers e dependências nas páginas correspondentes somente quando necessário. Não refaça Ghidra ou o oracle existente, nem publique listagens/payloads proprietários.
- `native/native_drawing.*` somente para preservar interfaces se State/observadores forem afetados. A observação de desenho e os comandos originais já entregues devem permanecer intactos.

Use o Mac original como referência. Não transplante estruturas ou algoritmos do engine Windows/ReCM.

## Escopo fechado: cinco entradas, 607 instruções

Os intervalos e contagens foram conferidos no export local completo. A inspeção de todos os flows e fallthroughs indexados não encontrou arestas externas para o interior desses cinco corpos. Isso não classifica entradas dinâmicas desconhecidas. Reconfira os limites e referências antes de implementar; calls estáticos não equivalem a cobertura runtime.

| Entrada | Fim exclusivo | Instruções | Responsabilidade |
| --- | --- | ---: | --- |
| `0x1074dd40` | `0x1074dd94` | 21 | Acessar nó da lista pelo índice original |
| `0x1074df60` | `0x1074df8c` | 11 | Preparar a posição mutável e consultar a árvore |
| `0x1074df90` | `0x1074dfb8` | 10 | Preparar busca por descritor a partir da raiz |
| `0x1074e150` | `0x1074e5a4` | 277 | Percorrer a árvore em pré-ordem pela posição mutável |
| `0x1074e5b0` | `0x1074ea30` | 288 | Buscar na árvore usando seu comparador original |

Fingerprints SHA-256 dos registros originais normalizados por `address/assembly/ops/flows/fallthrough`, em ordem de endereço, com `json.dumps(sort_keys=True, separators=(",", ":"))` e UTF-8, como nos audits existentes:

- `0x1074dd40`: `6616dd13a4bb0987aea301ef1223f923e4ba1eb9c6ff19dd4f8be72015e23780`.
- `0x1074df60`: `2f1b87f82d1b17d062d211efef96e018ca1037a33ecbf7949cc798bdecc1fab1`.
- `0x1074df90`: `cc4f5a4343d038fa57d92058142c9b659d3d975729e068973098227a5936d5d0`.
- `0x1074e150`: `7d90da28bd1aa7a1b61e53cd33f2b9a4b25040330a8b92df582e341f3f45582c`.
- `0x1074e5b0`: `aac339e33d1c6f7e3d8ec5e88d6ce4e915cd9d00317e6f4e21a76b88785882e5`.

### Contratos a preservar

Caracterize cada entrada no export antes de implementar. Os pontos abaixo orientam o trabalho e não substituem a comparação completa da ABI.

`0x1074dd40` recebe a coleção em r3 e índice signed32 em r4. Lê primeiro o head em coleção `+0`, mesmo para índice zero ou negativo. Índice menor ou igual a zero retorna esse head. Índice positivo segue `next` em nó `+4` exatamente o número solicitado, no original em grupos de oito e depois o restante. A rotina ignora a contagem da lista; não introduza validação pela contagem. Head nulo com índice zero pode retornar zero; atravessar um nó nulo em índice positivo mantém o fault original. Preserve resíduos de registradores/CTR e a ordem dos acessos.

`0x1074df60` recebe coleção e posição, cria frame de 64 bytes, salva a posição no caller area em oldSP `+28` e passa o endereço dessa posição mutável a `0x1074e150`, com a raiz lida de coleção `+0`. Não transforme esse local em uma variável C++ sem preservar seus aliases e efeitos na memória guest.

`0x1074e150` recebe coleção, raiz e ponteiro de posição mutável. Percorre em pré-ordem: nó atual, filho em `+8`, filho em `+12`. Raiz nula retorna zero sem ler a posição. Posição zero seleciona o nó atual. Nos demais casos, o decremento de 32 bits com wrap é escrito antes de ler os filhos; uma árvore esgotada retorna zero mantendo a posição alterada. O original possui quatro níveis desenrolados antes de recursão e frames recursivos de 80 bytes. Uma travessia equivalente em resultado, mas com outra ordem de loads/stores, profundidade de frames ou efeitos de stack, não basta.

`0x1074df90` prepara a raiz da coleção e o descritor para `0x1074e5b0`. A busca com raiz nula não chama o comparador. Para nó presente, `0x1074e5b0` chama o transition vector mutável em coleção `+4`, passando a chave em nó `+0` e o descritor procurado. Resultado signed negativo segue o filho `+8`, positivo segue `+12`, zero retorna o nó. Não normalize o resultado para um bool nem presuma a ordenação convencional de uma biblioteca. O vetor é recarregado antes de cada comparação; o filho escolhido é carregado após o retorno do callback. Um comparador pode alterar a coleção/árvore e seus clobbers fazem parte do contrato. Preserve TOC e o caminho de callback existente, incluindo `0x10781320` e o slot de stack pertinente. A busca também tem quatro níveis desenrolados e recursão com frames de 80 bytes.

O contexto de backgrounds utiliza o slot `r2-0x5c38`, que aponta para um transition vector observado `0x108bd664`, com entrada `0x1074de10` e TOC `0x108bba90`. Esse wrapper chama `0x107393f0`. Esses endereços descrevem a dependência original observada, não autorizam fixar o comparador globalmente: o contrato é o vector da coleção e seu contexto mutável.

Preserve a ABI Mac de 32 bits big-endian, valores signed/unsigned, wrap, registradores e temporários, CR/XER/FPSCR quando afetados, PC/LR/CTR, TOC, backchains, spills, memória e faults com seus PCs e prefixos de efeitos. Dependências geradas devem executar pelo mecanismo de ABI existente; não restaure seus clobbers artificialmente. Não acrescente saneamento de índices, null guards ou alteração da ordem que transforme um fault original em sucesso.

Produza lógica semântica legível, com nomes e responsabilidades claros e adapter mínimo. Preserve os detalhes de compatibilidade em uma camada explícita, evitando uma cópia mecânica do corpo gerado. Não entregue wrappers que continuem executando as cinco rotinas originais.

Carregamento/liberação/inserção/remoção de coleções, comparação de strings e seu wrapper, aplicação/desenho de backgrounds e demais accessors continuam gerados. Date preserva sua assimetria bissexta; RNG e os quatro seletores permanecem semânticos e compatíveis. Imagens, fontes/layout/escala, menus, Restart, gameplay, multiplayer, serialização e otimização geral ficam fora deste goal.

## Integração em produção

Integre as cinco entradas nos labels de destino das páginas `0x1074d` e `0x1074e`, antes da primeira operação original. Cubra chamadas/gotos na mesma página, dispatch entre páginas, recursão, transition vectors e callbacks. Os helpers recursivos devem alcançar seus hooks também quando chamados pelo wrapper original. Produção normal usa as cinco implementações sem fallback aos seus corpos originais.

Mantenha um modo original privado, independente de Date, RNG e backgrounds, para comparação. Ele deve permitir comparar wrappers e helpers de forma isolada e composta, sem desabilitar todos os módulos para obter uma referência. Campos de diagnóstico ficam fora de saves/snapshots e são opcionais. Não substitua silenciosamente observadores existentes de Memory ou desenho; leituras feitas só para diagnosticar não podem introduzir faults.

Acrescente um audit de contratos e testes de hooks/manifesto para o novo bloco, usando nomes coerentes com o projeto, por exemplo `collection_lookup_adapter.*` e `collection_lookup_audit.py`. Preserve as 41 entradas atuais na mesma ordem e acrescente as cinco. Atualize a expectativa literal de 41 entradas em `tools/test_background_selection_audit.py` para preservar o bloco antigo e conferir a extensão a 46, sem relaxar seus contratos. Os call sites de backgrounds continuam chamando as mesmas entradas guest; a nova seleção ocorre nos destinos. Preserve os null guards de clube/competição em `0x10713aec` e `0x10713b3c`.

## Comparação focada e aceitação

Crie um harness compartilhado para as cinco entradas, usando o original como oracle com o mesmo estado inicial. Reutilize a infraestrutura e as fixtures de backgrounds quando apropriado. Compare arquitetura completa, toda a memória mapeada, estado de allocator/imports/callbacks e acessos ordenados, incluindo prefixos de faults. Separe contagem de instruções e diagnósticos da comparação de comportamento. Inclua expectativas independentes para nós retornados, ordem visitada, posição residual e número/ordem de comparações; não use a implementação nova como oracle.

Cubra casos representativos que detectem os riscos deste bloco:

- Lista com head nulo ou vários payloads distintos; índices negativos, zero, um e fronteiras dos grupos de oito; count inconsistente ignorado; travessia além da cadeia com fault preservado. Use mappings controlados e orçamento de execução, sem iterar bilhões de nós para simular um índice extremo.
- Árvores vazia, um nó, assimétrica e profunda o bastante para ultrapassar os níveis desenrolados; índices na raiz, em ambos os filhos e fora da árvore; posição negativa/extrema com decremento wrap e árvore pequena. Verifique a posição residual e o retorno em cada caso.
- Busca com match na raiz/ambos os lados/profundidade recursiva, miss, resultados de comparador zero/negativos/positivos não normalizados e árvore com chaves cuja ordem confirme os ramos originais.
- Comparador com TOC próprio, clobbers e mutação do vector/filhos entre comparações; callback fault; descritor não mapeado com raiz nula demonstrando que não é usado. Preserve acessos mesmo quando os ponteiros se repetem.
- TOC relocável, mappings mínimos, aliases relevantes da posição/links/collection/vector/spills e faults de dependências. Selecione aliases que possam distinguir a ordem original, sem campanha cartesiana indiscriminada.
- Wrappers originais com helpers novos, wrappers novos com helpers originais e o bloco todo semântico. Componha com os seletores de backgrounds já semânticos, com imagens não vazias, busca real de descritor e consumo RNG idêntico.

Inclua um negativo próprio por entrada, rejeitado em sua primeira fronteira observável; uma divergência do filho não substitui o negativo próprio do wrapper. Verifique reset dos diagnósticos e recusa de contratos alterados/parciais. Não introduza streams gigantes de snapshots para este bloco.

Demonstre dispatch de produção não vazio para as cinco entradas no harness, incluindo recursão dos dois helpers e pelo menos os caminhos relevantes de callback/dispatch indireto. Modo normal não deve executar os corpos originais das entradas selecionadas. Manifesto e hooks textuais sozinhos não provam execução. Relate essa evidência como integração no harness, distinguindo-a de uma sessão real do app.

Faça uma regeneração quando os hooks estiverem prontos e um build consistente. Rebuild completo é necessário se State/layout ou dependências compartilhadas mudarem; não repita `clean-first` por rotina. Registre o nome efetivo do novo target/CTest. Os dois checks existentes diretamente relacionados são `original_background_selection` e `modern_background_selection_equivalence`; ambos devem continuar passando. Rode o audit novo e `tools/test_background_selection_audit.py`, efetivamente alterado. Não repita RNG/Date só por estarem no mesmo engine; acrescente outra regressão apenas se sua implementação afetar seu contrato e explique por quê.

Comandos-base existentes, a partir da raiz do projeto:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure \
  -R '^(original_background_selection|modern_background_selection_equivalence)$'
python3 tools/test_background_selection_audit.py
```

Execute também o novo harness e seu audit com os nomes realmente criados. UBSAN focado no adapter/harness novos é apropriado para wrap, recursão e aliases; informe sua extensão exata, sem reinstrumentar todo o engine nem alegar whole-program/ASan. Reexecute somente os grupos afetados por correções ou falhas.

Uma inspeção curta e isolada do app é opcional se acrescentar evidência útil de backgrounds nas telas diretamente afetadas. Prefira fixtures para obter os estados; não abra a carreira pessoal, não use Save/Load para preparar coverage e não faça de contadores vivos por entrada um gate adicional. Se a UI não for exercitada, declare esse limite com precisão.

## Proteção, documentação e entrega

Preserve saves pessoais, inputs originais, fixtures e referências/candidatos históricos. Diagnósticos e fixtures novos ficam privados sob `.local`, compactos, sem copiar carreiras ou recriar artefatos antigos removidos por deduplicação. Não faça inventários globais repetidos a cada ajuste nem limpe a árvore `.local`. A referência `.local/background-selection-reconstruction/reference/installed.app` pertence à entrega anterior; crie uma referência privada nova do instalado atual antes de substituir a versão e preserve a antiga.

Documente em inglês em `docs/native-collection-lookup-reconstruction.md` os cinco contratos e fingerprints, integração, dependências geradas, casos/negativos, dispatch, comparação de efeitos, checks executados e limites. Atualize `docs/native-port-status.md`. Declare que persistência não foi retestada por orientação do usuário e diferencie resultados históricos, comparação direta e eventual inspeção do app.

Após concluir implementação e checks focados, instale o candidato uma vez com `python3 tools/install_native.py`, conforme a autorização de entrega já estabelecida no projeto. Use as proteções do instalador e confira conteúdo/metadados dos arquivos pessoais potencialmente afetados antes e depois, uma vez; preserve também preferência de apresentação e handoffs. Verifique ARM64, hash do executável fonte/instalado, bundled originals, plist de produção sem diagnósticos e horário efetivo da instalação. Separe fonte, candidato assinado, instalado e referência anterior; não abra carreira pessoal para validar a instalação.

Revise o diff exato, preserve alterações alheias e faça commit/push normal sob a autorização permanente para trabalho concluído e validado. Reconfirme o remoto de destino e verifique o SHA remoto após o push. Exclua os doze handoffs, saves, payloads, snapshots, screenshots e logs privados; não altere ignore/LFS para publicá-los. Não apresente essas cinco rotinas como reconstrução completa do engine ou de todo o ciclo de vida das coleções.

Conclua quando as cinco entradas executarem semântica C++23 em produção, os contratos e composições forem equivalentes, os negativos/dispatch/regressões focados passarem e documentação, instalação protegida e entrega estiverem conferidas. A sessão de preparação deste handoff somente salva e copia o prompt; implementação, instalação e publicação pertencem à sessão em que este pedido for executado.
