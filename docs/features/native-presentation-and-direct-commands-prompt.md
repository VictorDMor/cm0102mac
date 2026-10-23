# Goal: melhorar a apresentação e o controle direto do CM 01/02

Implemente e entregue, em `/Users/victordmor/Projects/cm0102mac`, um bloco de apresentação e controle nativos: escala ajustável da interface, apresentação correta em telas Retina, correspondência precisa entre imagem e input, e comandos principais acionáveis por nome e atalhos, sem depender do helper de coordenadas. Preserve a identidade visual original e o comportamento da simulação. Converse em português; código, documentação técnica, testes e novas mensagens/controles do aplicativo ficam em inglês.

## Decisão de produto e resultado esperado

O jogo já funciona nativamente em Apple Silicon. O usuário decidiu priorizar agora o conforto visual em monitores atuais e uma forma direta de aplicar comandos, por atalhos ou uma API, antes de continuar a reconstrução de outros trechos internos. Esta prioridade substitui o goal de quatro rotinas de seleção de backgrounds em `docs/features/native-background-selection-reconstruction-prompt.md`, que fica adiado. Não implemente os dois blocos juntos.

Entregue uma melhoria completa e delimitada de apresentação e controle. Exemplo de uso: abrir o jogo, escolher uma escala maior em um controle visível, ler e navegar pelas mesmas telas sem cortes ou cliques deslocados, acionar Continue/Save/Load pelo teclado ou por comandos identificáveis, sair e reabrir com a preferência de apresentação preservada. A automação deve conseguir executar as ações centrais sem calcular posições X/Y.

O engine continua produzindo a interface original em bitmap. Ampliar esse bitmap não cria detalhes novos nem transforma as fontes em texto vetorial. Melhore tamanho, enquadramento e amostragem; não prometa tipografia de alta resolução neste bloco. A aparência familiar do CM 01/02, seus fundos, cores, disposição das informações e interação permanecem reconhecíveis.

Um único goal deve cobrir apresentação, transformação de coordenadas, controle da preferência, comandos diretos e validação real. Uma tela estática ampliada, mockup ou atalho que apenas chama um clique em posição fixa não satisfaz o objetivo.

## Ponto de partida verificado em 4 October 2026

- Projeto em `main`, HEAD `7535805ed17f2440f9c1fc30158a0dc2e8fa6546`. Uma consulta nova a `origin/main` durante a preparação imediatamente anterior confirmou o mesmo SHA. Reconfira remoto, branch, diff e instruções aplicáveis antes de trabalhar.
- Não foi encontrado `AGENTS.md` aplicável na inspeção desta sessão. Há nove prompts untracked anteriores em `docs/features`, incluindo o de seleção de backgrounds; este novo arquivo será o décimo. Preserve todos, sem incluí-los no commit de produto. Não havia mudanças em arquivos versionados ou no índice.
- A produção tem 37 semantic replacements: 34 Date e três RNG. Mantenha esse manifesto; este goal não adiciona entradas semânticas, não reconstrói seletores e não altera RNG, calendário, partida, scheduler ou regras.
- O executável instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app/Contents/MacOS/CM 01-02 Native Prototype` foi novamente verificado com SHA-256 `1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6`. A fonte `.local/aot-build/CM 01-02 Native Prototype.app` e o instalado foram conferidos ARM64 e com esse mesmo hash durante esta sessão. Reconfira ambos antes de trocar builds.
- `docs/native-rng-core-reconstruction.md` registra 2.022 comparações diretas, 40/40 CTest, descoberta Python com 37 aprovados/quatro grupos opcionais pulados, os quatro opcionais aprovados explicitamente, UBSAN gerado/focado e dois ciclos reais Save/exit/reopen/Load. Esses resultados pertencem à entrega RNG; não validam esta nova apresentação.
- O usuário dispensou os testes prolongados restantes de Date e pediu validação proporcional e uso cuidadoso de armazenamento. Preserve essa limitação documentada sem reabrir a campanha semanal nem chamá-la de aprovada. Uma falha nova não fica dispensada por essa decisão antiga.
- A diferença de datas mantém o comportamento original assimétrico de anos bissextos. Não corrija esse contrato aqui.

Esta preparação inspecionou código e arquivos, sem abrir o jogo, executar uma nova suíte, implementar a escala, instalar ou publicar mudanças. O próximo trabalho é implementação, com os critérios abaixo.

## O que o código atual já mostra

`native/ui_bridge.mm` cria as janelas AppKit a partir de retângulos legados. `present_window` recebe o buffer pronto, cria um `CGImage` e o apresenta via `NSImage`; `CMNativeView::drawRect` desenha em `self.bounds` com interpolação desabilitada. Isso preserva os pixels de origem, mas não constitui uma preferência de escala utilizável nem prova tratamento completo de Retina.

Hoje, `queueInput`, `pointer_state`, `window_bounds` e `find_window` relacionam eventos e geometria diretamente com coordenadas da tela macOS. `native/platform.cpp` usa esses resultados em port creation, `GetMouse`, `GetGlobalMouse`, `FindWindow`, regiões e desenho. Aumentar apenas o tamanho da imagem ou da janela deixaria a apresentação e o input incoerentes.

O programa original cria a janela principal de 800×600 e também janelas de fundo, overlays e outros elementos. Não assuma que toda janela deve ser ampliada isoladamente por um fator igual ou que o tamanho do desktop é o tamanho do painel principal. Inspecione as relações reais antes de escolher a transformação.

`set_menu_bar` reconstrói a barra nativa e já acrescenta o menu `Match` e, em validação, `Validation`. O novo controle de apresentação precisa sobreviver a essas reconstruções sem interferir nos IDs/comandos de menus do jogo.

Há uma lacuna concreta para os comandos: `originalMenuAction` já enfileira `UIEvent::Kind::menu`, mas `NativePlatform::next_event` atualmente rejeita essa seleção com `Native menu selections require original MenuSelect dispatch`. `MenuKey` já resolve atalhos para IDs/itens de menu em parte do caminho existente. Não considere um item de menu visível ou um evento enfileirado como ação implementada. A ponte necessária para os comandos centrais passa a fazer parte deste goal, conforme a última decisão do usuário.

`save_window_images` salva a imagem de origem retida. Esse arquivo é útil como referência do bitmap, mas não demonstra como a escala, o enquadramento ou o backing Retina apareceram fisicamente na tela.

## Leia e inspecione primeiro

Leia `README.md`, `docs/native-port-status.md`, `docs/aot-port.md`, `docs/native-rng-core-reconstruction.md` e as partes de preservação/persistência de `docs/native-season-reference.md`. Distinga os relatos históricos do estado atual.

Inspecione principalmente:

- `native/ui_bridge.hpp`, `native/ui_bridge.mm`, `native/platform.hpp`, `native/platform.cpp`, `native/regions.*`, `native/cursor_resource.*` e `native/menu_resource.*`.
- `native/input_checks.mm`, `native/event_checks.mm`, `native/platform_checks.cpp`, `native/menu_checks.cpp`, `native/control_cursor_checks.cpp` e `native/CMakeLists.txt`.
- `native/aot_probe.cpp`, `native/callbacks.*`, `native/apple_events.*`, a inicialização do runtime privado, o bundle/plist e os caminhos de configuração do aplicativo.
- `tools/prepare_validation.py`, `tools/install_native.py` e a configuração existente de `.local/aot-build`.

Use o pipeline nativo já existente. Não crie um launcher externo, uma interface web sobreposta, um app paralelo ou um novo engine de renderização sem necessidade demonstrada. Prefira a extensão pequena da camada AppKit/Quartz atual. Consulte documentação oficial de APIs macOS se precisar confirmar detalhes; não trate hipóteses de Retina, coordenadas ou lifecycle como fatos.

Para mapear os comandos às ações do engine, inspecione os recursos originais de menu e o loop/handlers no índice privado `.local/recomp-input/instructions.sqlite`, somente para leitura. `native/menu_checks.cpp` lê os recursos do programa original e já verifica IDs, mutações e títulos. Confirme os contratos e estados permitidos; não invente endereços, IDs, atalhos existentes ou callbacks. Mantenha corpos, recursos e payloads proprietários fora do commit.

## Comportamento a entregar

### 1. Escala visível, ajustável e reversível

Adicione um menu nativo `View` com `Interface Scale`, oferecendo `100% (Original)`, `125%`, `150%`, `200%` e `Fit to Screen`. Mostre a opção ativa. Esses controles devem existir na versão instalada normal, sem flags de diagnóstico, terminal ou edição manual de configuração.

Na ausência de preferência válida, use `100% (Original)` como base compatível quando couber. Se nem o tamanho original couber no espaço útil atual, use um ajuste seguro que mantenha os controles acessíveis. `Fit to Screen` deve escolher uma escala uniforme para acomodar a interface relevante na área útil da tela, respeitando menu bar, Dock e proporções. Não distorça e não corte conteúdo para preencher a tela.

Escalas fixas que não caibam precisam de comportamento claro e seguro: desabilite a opção naquele espaço ou apresente o ajuste efetivo, sem exibir falsamente 200% quando outro valor foi aplicado. A volta a 100%/Fit deve permanecer acessível. Não deixe uma preferência previamente válida prender a interface fora da tela depois de uma mudança de monitor ou resolução.

A troca deve funcionar durante o uso normal, sem recarregar a carreira, alterar a data ou perder a tela atual. Preserve o posicionamento relativo e a ordem de janelas e elementos sobrepostos. Trate o fundo de desktop conforme seu papel real, para que ele não cubra ou desloque o conteúdo interativo ampliado.

Não é necessário implementar redimensionamento livre, fullscreen novo ou uma coleção de filtros neste goal. O controle de escala e o ajuste à tela devem, por si, entregar o resultado utilizável.

### 2. Apresentação em Retina e amostragem coerente

Diferencie pixels do bitmap original, pontos da janela AppKit e pixels físicos do backing. O fator de escala escolhido pelo usuário é independente do fator Retina; evite ampliá-lo duas vezes. Preserve a proporção, alinhamento, cores e conteúdo; não introduza blur acidental por conversões intermediárias.

Defina e documente uma política de amostragem para escalas inteiras e fracionárias, considerando o bitmap e as fontes originais. Use comparação visual real para escolher a política; não afirme que um filtro específico melhora a legibilidade antes de verificá-lo. Texto e linhas devem permanecer legíveis, sem tremulação de alinhamento, imagem esticada ou bordas cortadas ao alternar escalas. A ampliação continuará sujeita à resolução da arte original.

Responda corretamente a mudanças de backing scale e tela conectada. Considere áreas úteis diferentes, telas com origem negativa e telas com densidades distintas no desenho e nos testes de geometria. Não exija que o usuário mude as configurações de exibição do macOS para a funcionalidade funcionar.

### 3. Imagem e input devem compartilhar a mesma transformação

Mantenha a geometria lógica usada pelo jogo separada da geometria em que ela é apresentada pelo host. Faça a conversão inversa de input de forma coerente em eventos, consulta contínua do ponteiro e hit testing. Não altere buffers, estruturas ou coordenadas da simulação só para obter uma imagem maior.

Cubra mouse down/up/move, hover, clique direito, drag quando existente, scroll, wheel/trackpad, menus contextuais, foco, teclado e navegação entre janelas. `GetMouse`, `GetGlobalMouse`, `FindWindow`, bounds, regiões e os eventos Carbon devem concordar sobre o ponto lógico. Preserve semântica de limites direito/inferior, coordenadas negativas, ordem de janelas e áreas não interativas.

Cliques em margens adicionadas para enquadramento não podem ser convertidos em cliques dentro do jogo. A transição entre regiões escaladas, desktop e menu bar nativa precisa ser explícita e testável. Evite clamp de todos os cliques para a borda, pois isso acionaria botões indevidamente.

O cursor deve apontar para o alvo visível correto, com hotspot correto. Se seu tamanho for adaptado, preserve máscara/hotspot e a troca dos cursores originais. Não é necessário redesenhar a arte do cursor.

O modo de ponteiro usado pela automação (`CM_AOT_VALIDATION_EVENT_POINTER`) é separado do input normal. Ambos precisam usar a mesma definição de coordenadas lógicas, mas o sucesso do modo sintético não prova o caminho de ponteiro físico. Não faça a produção depender do menu `Validation` ou de coordenadas injetadas.

### 4. Comandos diretos para o usuário e a automação

Entregue uma única camada de comandos com identificação estável e disponibilidade conforme o contexto. Cubra pelo menos:

| Comando | Resultado esperado |
| --- | --- |
| `continue_game` | Acionar uma vez a ação original de continuar, quando disponível |
| `save_game` | Abrir o fluxo original de salvar, permitindo concluir ou cancelar pelo teclado |
| `load_game` | Abrir o fluxo original de carregar, respeitando confirmações e estado da carreira |
| `open_game_options` | Abrir as opções originais do jogo sem coordenadas |
| `back_or_cancel` | Executar voltar/cancelar somente no contexto em que essa ação existe |
| `set_interface_scale` | Selecionar uma das escalas suportadas pela apresentação |

Exponha os comandos por menus nativos acessíveis por nome e atalhos visíveis ao usuário. Use os menus existentes quando puder preservar seus contratos, adicionando comandos nativos apenas quando necessário. Considere convenções como Command-S para salvar, Command-O para carregar e Command-Return para continuar, depois de verificar conflitos e atalhos originais; documente o mapa final. Não intercepte letras comuns, edição de nomes, confirmação de diálogos ou teclas do jogo indiscriminadamente. Autorepeat não pode disparar Continue várias vezes.

Menus, atalhos e qualquer entrada de automação devem encaminhar à mesma ação original pelo loop/dispatch correto do jogo. Preserve precondições, efeitos, confirmações, ordem de callbacks e comportamento do teclado. Resolva a ponte mínima de menu/`MenuSelect` necessária, sem abrir uma reescrita de todo o sistema de menus. Não altere datas/resultados, não grave o formato de save por um caminho paralelo e não manipule arbitrariamente PC/estado para simular que a ação ocorreu. Evite executar o engine de forma reentrante a partir de um callback AppKit.

Disponibilidade é parte do contrato: durante loading, processamento ou modal incompatível, desabilite/recuse o comando de maneira clara, sem crash, espera infinita ou execução atrasada surpreendente. Um comando aceito não equivale a concluído. Identifique quando foi executado, abriu um diálogo/aguarda interação, terminou ou foi recusado, conforme o meio de acionamento. Abrir Save deve manter a escolha de nome e as confirmações originais; abrir Load não autoriza descartar trabalho silenciosamente. Não faça retry automático de Continue/Save quando o resultado estiver incerto.

Para automação, o requisito é acionar os comandos por nome ou atalho estável **sem X/Y, reconhecimento de pixels ou cliques fixos escondidos**, com disponibilidade identificável e efeito verificável. Menus AppKit acessíveis e atalhos funcionais podem satisfazê-lo; observar visualmente o resultado continua permitido e não exige converter toda a UI bitmap em elementos acessíveis neste bloco. Não é obrigatório criar servidor/API externa se isso já resolver o fluxo de ponta a ponta. Se uma API local for necessária, limite-a aos comandos nomeados, ao processo/runtime escolhido e a um transporte local com acesso restrito; permita consultar disponibilidade/status, recuse chamadas duplicadas ou incompatíveis e use o mesmo dispatcher. Não abra um serviço de rede geral nem ofereça execução arbitrária de funções ou escrita de memória.

O helper `Validation → Click at position…` pode permanecer para diagnóstico e telas ainda não cobertas, mas deixa de ser requisito para Continue, Save, Load, Game Options, Back/Cancel e escala nos contextos suportados. Registre com precisão os limites: este bloco não promete uma API semântica para todos os botões e telas do jogo.

### 5. Preferência local e isolamento

Persista a escolha de escala como configuração de apresentação do usuário, fora do conteúdo dos saves e das preferências originais do engine. A nova configuração deve ser pequena, validada e independente do seed/estado de carreira. Valores ausentes, corrompidos ou incompatíveis com a tela atual devem cair em uma apresentação utilizável; uma falha de gravação não pode corromper a carreira ou impedir sair do jogo.

Escolha uma localização/namespace coerente com o app e documente-a. Candidatos privados e testes devem ter configuração isolada: não sobrescreva a preferência do instalado ao testar. A única mudança pessoal prevista ao usar o novo recurso é a configuração de apresentação identificada; isso não autoriza modificar Game, saves, recursos ou configurações de outras aplicações.

## Limites explícitos

- Preserve layout e arte originais, regras, RNG, calendário, resultado de partidas, serialização e os 37 semantic replacements. Não faça as quatro substituições de background adiadas nem mude qual imagem é sorteada.
- Fontes vetoriais/novas, redesenho completo, layouts responsivos, pacotes de imagens em HD, novos gráficos de partida, shaders, IA para upscaling e Metal são trabalhos separados. Não os acrescente a este primeiro bloco por associação com gráficos.
- Não reescreva bridges não relacionados. O menu `View`, os comandos diretos e a ponte de menu necessária para Save/Load/Continue estão no escopo. Reparos adicionais de menus, automação de toda a UI e uma API geral do engine ficam fora.
- Não altere dados originais, resolução global do macOS, contas ou permissões do sistema. Não use emulação, Wine, CrossOver, execução somente via Rosetta ou VM.
- A preferência mais recente do usuário autoriza este bloco visual e substitui a prioridade anterior de reconstrução interna; referências antigas que suspendiam gráficos durante a construção da referência de temporada não bloqueiam este objetivo atual.

## Validação proporcional e observável

### Verificações de componente

Extraia e teste a geometria relevante de forma determinística, sem exigir monitor específico para todos os casos. Cubra 100%, escalas fracionárias, 200% e Fit; backing 1×/2×; área útil menor/maior que o painel; mudança de área útil; múltiplas origens, incluindo negativas; offsets/margens; pontos interiores e bordas; ida e volta entre espaços; janelas sobrepostas e inexistência de hit nas margens. Faça assertions sobre os resultados esperados e os contratos do jogo, não sobre uma cópia da fórmula da implementação.

Reexecute e estenda as verificações reais de input/menus/eventos necessárias. Inclua os novos controles depois de `set_menu_bar` repetido, estado marcado correto, persistência/reabertura da configuração, conteúdo inválido, recuperação de escala que deixou de caber e isolamento entre instalado/candidato/testes.

Para comandos, compare menu e atalho chegando à mesma ação original, com IDs/itens corretos mesmo após mudanças de menu. Teste habilitado/desabilitado, carregamento, diálogo ativo, cancelamento, foco em campo de texto, autorepeat e uma única execução por acionamento. Quando houver API, teste também seleção do processo/runtime, disponibilidade/status e proteção contra envio repetido; não confunda aceitação na fila com conclusão. Inclua controles que rejeitem um comando em contexto incorreto e detectem execução dupla. A chamada não pode depender da posição/tamanho da janela ou da escala. Preserve confirmações e o comportamento original de Save/Load/Continue.

Em 100%, mantenha as expectativas de input e bitmap do caminho atual, salvo mudanças de apresentação intencionais e documentadas. Para um mesmo buffer de origem, escalar não deve alterar seus bytes ou gerar consumo RNG. Verifique que resize/redraw não vira Continue Game, clique fantasma, recarga ou mudança de estado da carreira.

Durante o desenvolvimento rode checks focados. Quando o bloco estiver estável, faça um build consistente e uma execução final da suíte configurada. O grupo `native_filesystem` inclui `platform_checks.cpp`; o nome sozinho não representa todo seu conteúdo. Os grupos existentes `native_input_queue`, `validation_event_pointer`, `original_input_callbacks` e `original_control_cursor` são relevantes, além dos novos checks de apresentação. `cm-menu-checks` é um executável; reconfira seus argumentos e registro no CMake antes de usá-lo.

Comandos-base, com os novos targets/checks integrados ao CMake:

```sh
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input" \
  -DCM_DATE_SNAPSHOT="$PWD/.local/performance/season-day-01/SeasonFixture/initial.snapshot"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure
```

Os exports e código gerado existentes não precisam ser recriados para uma mudança restrita à apresentação. Não execute novamente o pipeline Ghidra/oracle nem um clean build de todo o engine sem necessidade concreta. Se tocar ferramentas Python, rode os testes afetados e a descoberta existente; não marque grupos pulados como aprovados. Use sanitização focada quando ela responder aos riscos do código novo e declare seu alcance, sem alegar instrumentação que não foi executada.

Não imponha novos replays semanais, temporada completa, regravação dos streams Date ou campanhas RNG extensas como prova desta UI. Se houver regressão observada, amplie a investigação de forma dirigida. Esses replays não substituem input e apresentação reais.

### Aceitação no candidato real

1. Antes de mudar a implementação, registre uma referência visual curta do app atual em runtime privado e a configuração da tela usada; caracterize também os caminhos originais dos comandos. Prepare um candidato final ARM64 separado, com Runtime/Runs e configuração de apresentação privados. Reconfira processo e executável; opere uma instância por vez.
2. Carregue o save de referência de trabalho indicado abaixo. Pela UI normal do candidato, alterne 100%, ao menos uma escala ampliada/fracionária que caiba, e Fit. Verifique texto, linhas, proporções, enquadramento, foco, cursor e hotspots, navegação por lista de jogadores, perfil, clube/competição e Game Options. Exercite scroll e controles próximos às bordas. Abra e feche um menu/elemento sobreposto, retornando à tela anterior sem perder o estado.
3. Valide o input normal do caminho de produção, além dos testes sintéticos. Se a automação precisar do ponteiro de validação, registre essa limitação separadamente e não declare o input físico comprovado só por esse modo. Demonstre os dois caminhos pelo meio disponível; um bloqueio de observação não deve virar PASS.
4. Faça screenshots reais da apresentação antes/depois, com a escala escolhida visível e a mesma tela comparável. Um PNG salvo por `save_window_images` mostra o bitmap de origem e não basta para provar escala/Retina. Preserve evidências compactas e informe resolução lógica, backing scale e tamanho apresentado medidos, sem inferi-los pelo nome do computador.
5. Verifique Retina ao vivo se o hardware disponível oferecer esse modo. Teste fisicamente outros monitores/densidades apenas se estiverem disponíveis; caso contrário, cubra as transformações em testes e reporte a lacuna de hardware. Não declare uma combinação 1×/2× validada fisicamente a partir de geometria sintética. Mudanças de backing/área devem permanecer robustas no código e nos testes mesmo quando não puderem ser exercitadas em hardware adicional.
6. Com o helper de coordenadas dispensado, execute por menus identificáveis/atalhos os comandos de abrir Game Options e voltar/cancelar. Abra Save e Load e prove o cancelamento sem perda de estado. Na carreira isolada, acione Continue uma vez em contexto válido e observe a resposta original; a progressão mínima necessária agora está no escopo para provar esse comando. Não imponha partida completa ou uma data artificial como resultado. Se surgir uma decisão/modal normal, registre o estado e trate-o pelo fluxo original.
7. Em escala ampliada, acione o novo comando Save, conclua o diálogo original pelo teclado, saia normalmente, confira ausência do processo e reabra o mesmo candidato. Acione o comando Load e conclua a seleção do save pelo teclado ou elementos acessíveis, sem o helper de coordenadas. Confira restauração da preferência, alinhamento dos controles e preservação de data/fase, manager/club, resultados/fixtures e perfil/contrato observados antes do Save. Salvar somente pelo caminho antigo de cliques não aceita o requisito de comando direto.
8. Prove que os mesmos comandos funcionam em 100% e em uma escala ampliada, independentemente da posição da janela; confirme que 100%/Fit continuam acessíveis após reabrir. Registre o caminho completo acionamento → ação original → resultado, distinguindo menus/teclado/API, se houver. Pare quando os critérios estiverem demonstrados; não replique campanhas longas sem falha nova.

Recrie bindings após reabrir. Evite consultar a aplicação por AX após a saída final, pois essa API pode relançá-la. Distinga app fonte, candidato assinado, processo observado e instalado. Se algum critério central — escala, legibilidade, input normal, comandos diretos ou Save/Load — falhar, corrija antes de instalar e não declare a entrega concluída com base apenas nos testes de componente.

## Saves, armazenamento e proteção

O usuário autorizou a deduplicação anterior: 705 cópias idênticas foram removidas, somando 15.232.817.366 bytes. O inventário resultante tinha 68 arquivos, correspondentes a 67 conteúdos distintos mais um backup do save de referência. O `Reading.sav` pessoal foi preservado. Alguns caminhos antigos de testes deixaram de existir intencionalmente; não recrie todas essas cópias nem use o inventário histórico de 1.491 artefatos como obrigação atual.

Use estes arquivos que foram novamente conferidos ao preparar este prompt:

- Preferido: `.local/performance/rng-core-final-01/Runtime/Files/games/RNGFinal-04SepResults.sav`, 34.872.328 bytes, SHA-256 `44dd1f372cf9f568f63b1f2e5942c5a68f357cb816b1b2c9b58941c32580e4a3`.
- Referência LFS imutável: `saves/Mac-Teste-Validado-Pos-Jogo.sav`, 17.917.514 bytes, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.

O preferido foi observado em September 4, 2002 NTE, Victor Moraes/Cruzeiro, com Brazil 5:1 Uruguay preservado no ciclo final RNG. Os demais campos observados de fixtures, perfil e contrato de Luisao estão em `docs/native-rng-core-reconstruction.md`; reconfirme-os no candidato. Não exija que um Load real restaure a trajetória CRT de startup como se fosse snapshot determinístico.

Crie cópias privadas apenas dos saves necessários e preserve os originais. `tools/prepare_validation.py` copia todo o Runtime recebido e adiciona `Perf-Reference.sav`; dê a ele uma origem mínima com as preferências necessárias para não multiplicar novamente os saves históricos. Não use hard links para saves que podem ser gravados.

Inventarie conteúdo, forks/FinderInfo, xattrs, modos, mtime e flags dos arquivos pessoais existentes em `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}` e dos saves/referências/fixtures relevantes. Preserve tudo fora das mudanças explicitamente esperadas da nova configuração de apresentação. Candidatos e testes devem ficar em suas próprias pastas e não tocar os dados pessoais.

Crie uma nova referência privada do instalado atual antes de substituir builds. A referência `.local/rng-core-reconstruction/reference/installed.app` é anterior à entrega RNG, com executável `a35a303f...`; não a sobrescreva nem a confunda com a produção atual. Reavalie espaço livre antes de gerar arquivos; havia aproximadamente 41,7 GB após a limpeza, mas o valor muda com o uso.

Os recibos `.local/save-deduplication-20261004-215858/{plan.json,removed.jsonl,receipt.json}` auxiliam a localizar cópias preservadas; este prompt já fornece o save inicial necessário e não depende deles para definir o objetivo.

Guarde relatórios compactos, screenshots úteis e uma cópia canônica dos novos saves. Limpe apenas temporários reproduzíveis criados neste goal depois de registrar evidências e identidades. Não apague saves únicos, backup de referência, original inputs, fixtures imutáveis, candidatos/referências históricos ou a árvore `.local`. Refaça inventários com base no que existe hoje.

## Entrega

Documente em inglês em `docs/native-presentation-and-direct-commands.md` os controles, comandos/atalhos e disponibilidade por contexto, como a automação os aciona sem coordenadas, dispatcher/ponte original, modelo de coordenadas, política de amostragem, defaults/recuperação, localização e isolamento da preferência, testes executados, provas visuais/input/persistência, combinação de monitores/backing realmente observada e limitações. Se houver API, documente comandos, parâmetros, estados e uso local reproduzível. Atualize `README.md` com menus/atalhos e `docs/native-port-status.md` com o resultado. Não apresente upscale de bitmap como fontes redesenhadas ou toda a interface reconstruída.

Após a aceitação, instale uma vez com `tools/install_native.py`, dentro da autorização de entrega do projeto. Verifique ARM64, hashes separados da fonte/candidato assinado/instalado/referência anterior, bundled originals, plist sem diagnósticos e inventários pessoais antes/depois. Confirme que os controles de escala e comandos existem no build normal instalado e não dependem de modo de validação; faça uma verificação curta e isolada sempre que possível. Identifique qualquer arquivo novo de preferência esperado, preservando todos os saves pessoais. Registre o horário efetivo da instalação e o hash do executável, pois o mtime da pasta `.app` pode ser preservado pela cópia.

Revise o diff exato, preserve trabalho alheio e faça commit/push normal, sem force, conforme a autorização permanente para trabalho concluído e validado. Confirme o SHA remoto com consulta nova. Exclua os dez handoffs pendentes, saves, payloads originais, screenshots privados e logs; não mude ignore/LFS para publicá-los. A sessão que apenas prepara este prompt não implementa, instala ou publica o produto.

Conclua quando a escala for controlável pela UI normal, a apresentação estiver proporcional e legível no hardware observado, cliques/cursor/menus corresponderem ao conteúdo, os comandos centrais funcionarem sem o helper de coordenadas, a preferência e Save/Load persistirem, os contratos do engine continuarem preservados e instalação/documentação/proteção/entrega estiverem verificadas. Relate limites de resolução da arte, de comandos/contextos ainda não cobertos e de hardware não testado sem transformá-los em alegações de validação.
