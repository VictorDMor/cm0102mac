# Native season reference and regression foundation

Estabeleça uma referência verificável de comportamento do CM 01/02 nativo em um ciclo completo de temporada. Este é o próximo marco proposto antes da modernização semântica dos subsistemas; não recomece a implementação dos fluxos básicos já funcionais.

## Objetivo e visão do produto

O objetivo final é um jogo completo em código moderno, compreensível e modificável, preservando a simulação, a aleatoriedade, os resultados numéricos, os saves e a experiência essencial do CM 01/02. A implementação atual recompila estaticamente as instruções originais do Mac em C++ para ARM64. Isso fornece uma referência executável, mas ainda não significa que os subsistemas foram reconstruídos em C++ com nomes, tipos e responsabilidades claros.

Entregue um ciclo de temporada documentado e uma comparação automática de comportamento em um trecho limitado do avanço de dias. Essa base deverá detectar regressões quando módulos gerados forem substituídos por implementações modernas. A temporada longa e a comparação determinística limitada têm provas diferentes: não confunda ausência de crash com fidelidade integral.

## Ponto de partida a revalidar

- Projeto: /Users/victordmor/Projects/cm0102mac.
- Estado observado em 01/10/2026: branch main, HEAD b9589e295e093cf501d0b21b02131943f4bd9379. Há alterações pendentes no runtime, gerador, interface, testes e documentação, além de arquivos novos de replay e otimizações. Preserve tudo; inspecione novamente status, diff, instruções locais e processos antes de agir.
- O fluxo principal single-player já funciona: criar/carregar carreira, navegar pelo clube, avançar dias, escalar, disputar partida completa, substituir, voltar ao clube, salvar e reabrir. Não trate a documentação histórica que dizia "not playable" como estado atual.
- Há registro local de 25 grupos CTest aprovados para o candidato de register-inline. É evidência histórica de componentes, não execução nova nem prova de carreira longa ou paridade integral.
- O replay atual em native/match_replay_main.cpp para na primeira chamada nativa. O snapshot existente cobre estado do runtime, mas não restaura todos os serviços nativos. Não o apresente como simulador completo de partida ou carreira sem interface.
- Alguns serviços menos usados e multiplayer permanecem incompletos. O multiplayer fica fora deste incremento; uma recompilação completa do jogo continuará exigindo sua avaliação em etapa posterior.

## Leia e inspecione

Consulte instruções locais, README.md, .gitignore, .gitattributes, docs/native-port-status.md, docs/aot-port.md e docs/performance-validation.md. A sequência de implementação em docs/native-port-status.md prevê preservar a referência recompilada e modernizar módulos incrementalmente com comparações de comportamento.

Inspecione native/CMakeLists.txt, native/aot_runtime.hpp, native/aot_runtime.cpp, native/platform.cpp, native/callbacks.cpp, native/match_replay.cpp, native/match_replay_main.cpp, native/day_advance_checks.cpp, native/match_arithmetic_checks.cpp, tools/compile_recomp.py e tools/prepare_validation.py. Verifique o ambiente e os insumos locais; arquivos ignorados não acompanham um clone novo.

## Escopo

1. Fixe uma versão de referência identificada por revisão e hash do executável, uma cópia privada do save ou uma nova carreira, ligas, opções e decisões de gerenciamento. Preserve o save Git LFS saves/Mac-Teste-Validado-Pos-Jogo.sav sem modificá-lo. Utilize uma instância do jogo por vez e proteja qualquer sessão ativa antes de trocar builds.
2. Complete uma temporada e alcance o início da seguinte. Registre o que foi efetivamente exercitado: calendário, conclusão das competições, atribuição de títulos e promoções/rebaixamentos aplicáveis, atualização anual dos jogadores, contratos/transferências e geração do calendário seguinte. Se um fenômeno não ocorrer no cenário escolhido, marque-o como não exercitado em vez de inventar cobertura.
3. Faça checkpoints privados antes e depois da transição de temporada. Salve pela interface original, encerre normalmente, reabra e confira data, clube, competições, resultados e estado relevante dos jogadores. Corrija falhas concretamente reproduzidas com testes pertinentes, sem alterar as regras para contorná-las.
4. Crie uma comparação repetível de um trecho delimitado do avanço de dias, começando com um dia completo e ampliando para um intervalo curto que inclua uma operação relevante da temporada. Identifique e controle no modo de validação os fatores externos realmente observados, como tempo, eventos de entrada e arquivos; preserve ordem e consumo do RNG. Não force placares nem altere a aleatoriedade para fazer as execuções concordarem. Ampliação para replay completo de carreira não é requisito deste incremento.
5. Compare o estado de jogo nos mesmos limites de execução. Mapeie e nomeie os campos necessários a um relatório útil; registre a primeira divergência. Só exclua metadados voláteis demonstrados, com justificativa explícita. Hash de arquivo ou memória isolado não substitui interpretação do estado quando houver diferenças legítimas de metadados.
6. Demonstre que a comparação passa em duas execuções equivalentes e acusa uma divergência deliberada em um fixture privado de teste. Mantenha diagnóstico e controle de entradas externos ao fluxo normal do usuário.

Não introduza novas otimizações, novas regras, novos gráficos, paralelização da simulação, reescrita geral do engine ou remoção das verificações de memória. Não substitua a experiência original por um simulador reduzido. A referência recompilada serve para detectar regressões entre versões; sem evidência adicional, não declare equivalência integral com o jogo rodando no hardware original.

## Validação e conclusão

- Temporada concluída e começo da seguinte observado no aplicativo ARM64 real, com evidência das transições exercitadas e suas limitações.
- Save/reload nos dois lados da transição preserva os campos examinados.
- Um cenário limitado do avanço de dias tem entradas registradas/controladas e comparação automática de estado, com aprovação de execuções equivalentes e detecção de divergência conhecida.
- Falhas encontradas estão corrigidas e verificadas, ou descritas como bloqueios que impedem declarar o marco concluído. Não deixe "estabilização" se tornar um objetivo sem fim.
- Testes pertinentes e suíte completa passam após as mudanças. Use o cache de build existente antes de reconfigurar; cabeçalhos compartilhados podem recompilar cerca de 1.922 unidades. A suíte existente é executada com ctest --test-dir .local/aot-build --output-on-failure.
- Atualize a documentação técnica em inglês com versão de referência, cenários, método de comparação, cobertura e limites. Separe teste de componente, comparação automática e observação visual.

Converse em português; mantenha código, documentação técnica, testes e mensagens do aplicativo em inglês. Instalação local do aplicativo validado, commit e push normais estão autorizados para trabalho concluído e validado: preserve dados, revise o diff exato e verifique o SHA remoto. Não publique saves privados, snapshots, entradas gravadas, logs, mídia proprietária ou build. Este prompt de planejamento não autoriza iniciar outra conversa ou publicar apenas a proposta como se a implementação estivesse entregue.

Ao concluir, indique o primeiro subsistema candidato a reconstrução semântica, com uma fronteira pequena e um contrato agora testável. A reconstrução desse subsistema será o incremento seguinte, não parte desta entrega.
