# LED Cube 4×4×4 — ESP32-C3 + BLE

Cubo de 64 LEDs controlado por ESP32-C3 SuperMini, dois TPIC6B595 em cascata e quatro camadas com PMOS acionados por NPN. Firmware Arduino com comandos BLE e cinco efeitos sequenciais.

## Firmware

Abra `led_cube_ble/led_cube_ble.ino` na Arduino IDE com o pacote de placas **esp32 da Espressif Systems** instalado. Selecione a placa ESP32-C3 correspondente (ESP32C3 Dev Module quando aplicável) e a porta serial. O código utiliza a biblioteca BLE incluída no pacote ESP32, além de `<atomic>`.

Esta versão preserva o comportamento do código de animações fornecido na conversa. Ainda não foi compilada ou validada em hardware nesta entrega; nenhuma versão específica do pacote ESP32 foi fixada.

## Pinagem

| Função | GPIO |
|---|---:|
| TPIC SER IN / Data | 4 |
| TPIC SRCK / Clock | 5 |
| TPIC RCK / Latch | 6 |
| Layer 1 / base do NPN | 7 |
| Layer 2 / base do NPN | 8 |
| Layer 3 / base do NPN | 9 |
| Layer 4 / base do NPN | 10 |

Nível alto no GPIO da camada liga o NPN e, por consequência, o PMOS. Um bit 1 enviado ao TPIC ativa a saída de corrente correspondente. A ordem física das colunas depende da montagem.

## Uso com nRF Connect

1. Ligue a placa e conecte a `Cubo_LED_4x4x4`.
2. Abra o serviço `6e400001-b5a3-f393-e0a9-e50e24dcca9e`.
3. Escreva texto UTF-8 na característica `6e400002-b5a3-f393-e0a9-e50e24dcca9e`.

| Comando | Ação |
|---|---|
| `ON` ou `1` | Inicia/reinicia a sequência |
| `OFF` ou `0` | Apaga o cubo |

Ao ligar a alimentação, o cubo começa apagado. Desconectar o aplicativo não interrompe uma animação em andamento; o dispositivo volta a anunciar para permitir reconexão. Os comandos não exigem autenticação BLE.

## Efeitos

1. Camadas subindo e descendo.
2. Colunas verticais em sequência.
3. Oito pontos cintilando.
4. Preenchimento e esvaziamento por camadas.
5. Três flashes e uma pausa.

A sequência se repete até `OFF`. A atualização usa `millis()` e a multiplexação usa `micros()`, com período nominal de 2 ms por camada e intervalo de apagamento de 30 µs. No máximo uma camada é comandada por vez. Tempos e durações podem ser ajustados nas constantes e na função `updateAnimation()`.

## Observações do hardware

- Alimentação do cubo: 5 V, com GND comum ao ESP. A capacidade da fonte não determina a capacidade das trilhas da SuperMini.
- GPIO8 e GPIO9 são pinos de configuração de boot. As bases dos NPN podem interferir na inicialização, principalmente no GPIO9; a inicialização das saídas pelo firmware não corrige os níveis durante o reset.
- TPIC6B595 alimentado em 5 V exige nível alto mínimo de 0,85 × VCC = 4,25 V segundo seu datasheet. Acionamento direto por GPIO de 3,3 V não é garantido; deve-se prever adequação dos níveis lógicos.
- Com resistores de coluna de 100 Ω e queda de LED de aproximadamente 3 V, a estimativa simplificada é 20 mA por LED e 320 mA para uma camada completa, mais a eletrônica. Medir a corrente real antes de dimensionar a alimentação.

Referências: [TPIC6B595 — TI](https://www.ti.com/lit/ds/symlink/tpic6b595.pdf), [Boot ESP32-C3 — Espressif](https://docs.espressif.com/projects/esptool/en/latest/esp32c3/advanced-topics/boot-mode-selection.html).
