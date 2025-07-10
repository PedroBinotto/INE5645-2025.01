## Por fazer

- [x] Permitir operação de escrita remota (não implementada ainda);
- [x] Realizar invalidação de cache e notificação de atualização de bloco;
- [ ] Manter a log-stack de operações locais ⇒ descartar operações feitas em estado de conflito (feito com cache mas timestamp posterior à invalidação de cache, por exemplo);
- [ ] Reavaliar quais funções podem receber os parâmetros por `const &` para evitar cópias desnecessárias;

