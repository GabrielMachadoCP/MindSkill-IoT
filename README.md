# MindSkill — Estação de Bem-Estar e Produtividade (ESP32 + MQTT + LCD + DHT22)

## Integrantes

- **Gabriel Machado Carrara Pimentel — RM 99880**
- **Lourenzo — RM 99951**
- **Vitor — RM 97758**

## 1. Sobre o Projeto

O **MindSkill IoT** é uma estação inteligente de bem-estar aplicada ao **Futuro do Trabalho**, capaz de:

- Monitorar temperatura, umidade e nível de stress (simulado com potenciômetro);
- Identificar estados **NORMAL / MODERADO / CRÍTICO** automaticamente;
- Exibir mensagens no LCD 16×2;
- Atuar com LEDs (verde, amarelo, vermelho);
- Sugerir pausas quando detectado risco;
- Receber **comandos remotos via MQTT** (ex.: “PAUSE” enviado pelo celular).

## 2. Problema Identificado

Ambientes de trabalho modernos sofrem com:

- Estresse elevado;
- Poucas pausas;
- Fadiga;
- Condições ambientais inadequadas.

Isso prejudica saúde mental, produtividade e desempenho.

## 3. Solução Proposta

O sistema monitora o ambiente e o estado do profissional e:

- Classifica automaticamente o estado de bem-estar;
- Exibe alertas e recomenda pausas;
- Aceita a pausa via botão físico ou app móvel via MQTT.

## 4. Componentes

- ESP32
- DHT22
- LCD 16x2 I2C
- Potenciômetro (nível de stress)
- LEDs (verde, amarelo, vermelho)
- Botão
- Protoboard e jumpers

## 5. Circuito (Wokwi)

[Wokwi](https://wokwi.com/projects/447636047050910721)

<img width="1367" height="1073" alt="image" src="https://github.com/user-attachments/assets/5425f96e-de61-42fa-a091-e7cadba7b107" />


## 6. Dependências

Bibliotecas necessárias:

- DHT sensor library
- Adafruit Unified Sensor
- LiquidCrystal I2C
- PubSubClient

## 7. Execução

1. Abra o projeto no Wokwi  
2. Inicie a simulação (até conectar no wifi e mqtt)
3. Ajuste o potenciômetro para simular stress  
4. Modifique a temperatura no DHT22 para testar estados  
5. Pressione o botão ou envie um comando MQTT para aplicar pausa  

## 8. Tópicos MQTT

### Publicações do ESP32:
- `/mindskill/temperatura`
- `/mindskill/umidade`
- `/mindskill/stress`
- `/mindskill/estado`
- `/mindskill/payload`

### Assinatura (comando remoto):
- `/mindskill/commands/pause`

Payloads aceitos:
- `PAUSE`
- `1`
- `PAUSA`

## 9. App Mobile (Cliente MQTT)

Use um app MQTT para simular o "App MindSkill".

### iPhone — MQTT Analyzer
- Host: `broker.hivemq.com`
- Port: `1883`
- Publish:
  - Topic: `/mindskill/commands/pause`
  - Payload: `PAUSE`

### Android — MQTT Dash ou MQTT Client
Mesma configuração.

O app permite que o usuário confirme uma pausa recomendada.

## 10. Conclusão

O projeto demonstra como IoT + MQTT criam estações inteligentes de bem-estar, alinhadas ao futuro do trabalho, promovendo saúde mental, pausas inteligentes e ambientes mais equilibrados.
