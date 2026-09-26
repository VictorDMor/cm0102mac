/goal Otimize drasticamente os tempos de carregamento e processamento do Championship Manager 01/02 nativo para Apple Silicon e modernize progressivamente sua implementação em C++, preservando o comportamento do jogo original.

Trabalhe no repositório /Users/victordmor/Projects/cm0102mac, remoto origin git@github.com:VictorDMor/cm0102mac.git. Implemente as melhorias e valide o aplicativo real; não pare em um diagnóstico, plano, benchmark isolado ou alteração da versão da linguagem.

## Objetivo e prioridades

O jogo já funciona nativamente, mas os tempos de carregamento e simulação são insatisfatórios para um Mac de 2026. A prioridade é uma melhora expressiva e demonstrável na experiência real: abrir o aplicativo, carregar uma partida salva, criar uma nova carreira, avançar dias e processar partidas. Modernize primeiro as rotinas que mais custam tempo, substituindo código gerado de baixo nível por C++ direto, legível, seguro e eficiente onde isso trouxer benefício comprovado.

Mantenha a interface atual de 800×600 como referência. Aumento de resolução, redesenho das telas, novos gráficos e mudanças de layout ficam para um goal separado. Otimizar a renderização existente está no escopo se o perfil demonstrar que ela é um gargalo, preservando a apresentação e a interação atuais.

Após obter a linha de base, estabeleça e registre metas numéricas ambiciosas e justificadas para os cenários prioritários. Não invente tempos atuais nem prometa fatores de aceleração antes das medições. Um ganho pequeno em uma operação isolada não satisfaz o objetivo de reduzir drasticamente o tempo percebido pelo usuário.

## Estado inicial conhecido — confirme no ambiente

- O aplicativo é ARM64 nativo, sem emulador, Wine, CrossOver, Rosetta ou máquina virtual. Preserve esse requisito.
- A implementação atual recompila antecipadamente o executável PowerPC original do Mac: Ghidra/P-code → C++ gerado → ARM64, com serviços nativos de macOS. A reconstrução Windows/ReCM serve como referência; seu motor não está integrado. Não confunda esse material com código-fonte original pronto para compilar.
- O projeto já usa C++23. A modernização solicitada é estrutural e semântica: nomes e tipos claros, responsabilidades separadas, gerenciamento de recursos e estruturas de dados adequados. Adote recursos modernos efetivamente suportados pelo compilador disponível; não trate uma troca para um número de padrão mais recente como modernização concluída.
- O usuário confirmou o ciclo completo: criar/carregar a carreira do Cruzeiro, abrir plantel, jogador e competição, avançar datas, concluir uma partida, voltar ao clube, salvar, sair e carregar novamente com o resultado contra a Anapolina preservado.
- O save de referência está versionado via Git LFS em saves/Mac-Teste-Validado-Pos-Jogo.sav: 17.917.514 bytes e SHA-256 82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb, conferidos contra o objeto LFS. Em um clone novo, execute git lfs pull se necessário e confirme que o arquivo contém o save completo, não apenas o ponteiro LFS. Preserve essa referência e teste com cópias. A gravação instalada fica em /Users/victordmor/Library/Application Support/CM 01-02 Native/Runtime/Files/games/Mac-Teste-Validado-Pos-Jogo.sav; há também uma cópia local em .local/save-backups/post-match-82c504f46ce5.sav.
- Aplicativo instalado: /Users/victordmor/Applications/CM 01-02 Native Prototype.app. Dados e arquivos privados ficam em /Users/victordmor/Library/Application Support/CM 01-02 Native. A instalação já funciona sem o CDR montado, preservando resource forks, preferências e saves.
- A última execução completa dos testes passou: 18 grupos, zero falhas, 30,63 segundos. Isso não é uma medição de desempenho do jogo.
- A última alteração extraiu a montagem da exceção de Memory::check para uma função separada, mantendo limites e diagnósticos. Foi testada e instalada, mas ainda não teve validação de gameplay nem medição de ganho no jogo inteiro. O processo que estava aberto continuava usando a versão anterior: revalide qual binário está executando.
- Multiplayer e alguns serviços menos usados permanecem incompletos. Não apresente o aplicativo como uma versão integralmente validada. Implementar multiplayer não é o foco deste novo objetivo.
- O estado de implementação inspecionado em 26/09/2026 é a branch main no commit c75d79505caea14529237af41ef1cd2820a64ad2 (Track post-match save with Git LFS), precedido por 55d8e76 (Initialize native CM 01/02 port repository). Antes da edição deste prompt, o checkout estava limpo e o SHA de main no origin correspondia ao local. A revisão deste prompt é posterior a essa base; confira novamente HEAD, branch, diff e remoto ao começar. Preserve qualquer alteração que tenha surgido desde então.
- .local/, reports/generated/, mídias de instalação e saves fora de saves/ estão ignorados. Um clone novo não contém automaticamente o ambiente de compilação, os dados originais nem os relatórios locais. No mesmo Mac, confirme a existência desses arquivos antes de reusar o ambiente; em outro checkout, siga a documentação e as ferramentas do projeto para preparar os insumos disponíveis. Não inclua esses artefatos no Git apenas para fazer o build funcionar.

## Leia primeiro

Consulte as instruções locais aplicáveis e, a partir da raiz do projeto:

- README.md, .gitignore e .gitattributes
- saves/Mac-Teste-Validado-Pos-Jogo.sav, com verificação do conteúdo Git LFS
- docs/native-port-status.md
- docs/aot-port.md, especialmente o histórico de correções e as medições de desempenho
- reports/generated/aot-status.json, se disponível localmente; não é versionado
- native/CMakeLists.txt
- native/aot_runtime.hpp e native/aot_runtime.cpp
- tools/compile_recomp.py
- native/platform.cpp, native/filesystem_bridge.cpp, native/callbacks.cpp e native/sound_bridge.cpp
- Os testes das rotinas que decidir alterar, incluindo native/memory_checks.cpp, native/day_advance_checks.cpp, native/match_arithmetic_checks.cpp e native/sound_checks.cpp.

Use esses arquivos como ponto de partida; inspecione as implementações e o estado atual antes de confiar nos relatórios. Alguns documentos contêm etapas históricas com validações que eram pendentes naquela época; diferencie essas passagens das confirmações posteriores. O prompt é autossuficiente e não exige acesso à conversa anterior.

## Estratégia de implementação

1. Construa uma linha de base reproduzível para os cenários reais. Separe abertura do aplicativo, carregamento de save, criação da base, avanço de dias e processamento da partida. Diferencie tempo de CPU, disco, renderização e esperas deliberadas da interface. Use o mesmo save, configuração e condições comparáveis, com repetições e indicação da variabilidade. Evite que a instrumentação distorça o resultado.
2. Colete perfis representativos e priorize os maiores custos. Investigue infraestrutura da recompilação, verificação de memória, controle de instruções, chamadas aos serviços nativos, pesquisas, alocações, cópias e acesso aos arquivos conforme a evidência apontar. Não reescreva tudo de uma vez.
3. Substitua progressivamente as rotinas caras por módulos C++ modernos, mantendo comparações com a implementação original. Reutilize as referências e os testes existentes, mas confira os layouts reais do Mac; não aplique estruturas Windows diretamente aos dados Mac.
4. Preserve ferramentas de diagnóstico úteis. Separe instrumentação de desenvolvimento da execução normal quando demonstravelmente apropriado; não remova verificações de segurança nem silencie falhas para aparentar velocidade.
5. Considere paralelismo somente onde a independência das tarefas estiver demonstrada. Preserve a ordem da simulação, números aleatórios, resultados numéricos, callbacks, persistência e interação com a interface. Não use flags que relaxem operações de ponto flutuante indiscriminadamente.
6. Meça novamente após cada alteração relevante e descarte mudanças que não tragam benefício ou introduzam regressões. Não deixe o trabalho terminar apenas na construção do benchmark.

Já houve ganho de aproximadamente 4,7 vezes em uma consulta isolada ao sistema de arquivos; isso não prova aceleração equivalente no carregamento inteiro. Uma amostra anterior durante a partida teve 2.223 amostras da execução principal, incluindo 408 em Memory::check e 134 em State::step. São pistas históricas, não um diagnóstico definitivo da próxima execução. As pequenas otimizações mais recentes também não estabeleceram ganho global.

## Fidelidade e proteção do progresso

Preserve a simulação, os formatos de dados e de save, os recursos originais, o funcionamento sem CD e os caminhos já validados. Para comparações determinísticas, controle o estado inicial e os fatores de aleatoriedade/tempo relevantes; diferenças legítimas de metadados não devem ser confundidas com diferenças no jogo.

Não altere artificialmente resultados, reduza a quantidade de ligas/jogadores, pule simulação, esconda processamento nem substitua a experiência por um protótipo menor para atingir metas. Melhorias devem executar o mesmo trabalho de forma mais eficiente.

Não reinicie uma partida ativa sem proteger o progresso. As ferramentas de clique anteriormente tinham discrepância entre as coordenadas enviadas e o ponteiro consultado pelo jogo; revalide essa limitação. Para interação visual, use as ferramentas de controle de interface disponíveis. Se precisar de cliques físicos, peça uma sequência curta e concreta ao usuário e continue o trabalho independente. Não altere o comportamento original do mouse apenas para acomodar a automação.

## Compilação e validação

Use o Command Line Tools disponível, sem depender de uma instalação licenciada do aplicativo Xcode:

    DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake -S native -B .local/aot-build -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
    DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake --build .local/aot-build -j6
    ctest --test-dir .local/aot-build --output-on-failure

Inspecione as demais opções existentes no cache antes de reconfigurar. Regere o código quando alterar o gerador; não mantenha correções apenas em arquivos gerados que serão sobrescritos. Mudanças em cabeçalhos compartilhados podem recompilar cerca de 1.922 unidades, portanto planeje as iterações.

Depois dos testes pertinentes, valide no jogo real o carregamento, avanço, partida, retorno ao clube e gravação/reabertura, além das telas afetadas. Confirme a arquitetura ARM64 e que a instalação corresponde ao build testado. A instalação local pode ser atualizada usando tools/install_native.py com os dados já instalados, sem remontar o CD; preserve a sessão ativa e informe quando uma melhoria depender da próxima abertura.

## Critérios de conclusão e entrega

- Ganhos expressivos, reproduzíveis e documentados nos cenários reais prioritários, avaliados contra a linha de base e as metas definidas após medi-la. Não declare o objetivo concluído apenas por microbenchmarks ou testes verdes.
- Rotinas relevantes modernizadas em C++ com comportamento comparado ao original, responsabilidades claras e testes que protejam os contratos afetados.
- Nenhuma regressão conhecida nos caminhos anteriormente validados, incluindo salvar e carregar após uma partida.
- Aplicativo nativo testado e entregue localmente; distinção explícita entre build, instalação, processo em execução e validação feita pelo usuário.
- Documentação atualizada com medições antes/depois, cenários, condições, variabilidade, resultados de testes, mudanças aplicadas, gargalos restantes e limites da validação. Registre também tentativas descartadas que evitem retrabalho.

## Versionamento e entrega no repositório

Trabalhe sobre o estado atual confirmado, preservando alterações alheias. Mantenha commits coerentes de melhorias concluídas e validadas. Há autorização para fazer commit e push desse trabalho sem pedir confirmação a cada entrega: revise o diff exato, confira a branch e o remoto de destino, faça push normal e verifique que o SHA remoto corresponde ao commit entregue. Não use force push nem reescreva o histórico existente.

Preserve .gitignore e .gitattributes, incluindo a regra LFS do save de referência. Não versione outros saves pessoais, logs de execução, credenciais, dados privados, imagens do CD ou diretórios locais de compilação. O save de referência já foi incluído deliberadamente pelo usuário; mantenha-o imutável e crie arquivos de trabalho separados para testes. Publique somente evidências técnicas adequadas ao repositório, como metodologia, resultados agregados e testes reproduzíveis.

Ao concluir, informe o commit entregue, os ganhos medidos no aplicativo real, quais trechos foram modernizados, as validações executadas e as limitações restantes. Não apresente um push bem-sucedido como prova de funcionamento ou desempenho.

Converse comigo em português. Mantenha código, documentação técnica, testes e mensagens do aplicativo em inglês. Trabalhe com autonomia nas decisões rotineiras, preserve os arquivos privados e não solicite novas permissões para ações já abrangidas pelo objetivo.
