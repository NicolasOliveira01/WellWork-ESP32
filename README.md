# 🏢 WellWork - Sistema Inteligente de Monitoramento Ambiental

## 📖 Sobre o Projeto

### Identificação do problema

#### 🎯 Desafio no Futuro do Trabalho

Com a ascensão do trabalho remoto e híbrido, profissionais enfrentam novos desafios ambientais que impactam diretamente sua produtividade, saúde e bem-estar:

 - ❌ Ambientes inadequados: Temperatura, umidade e iluminação fora dos padrões ideais
 - ❌ Sedentarismo digital: Longas horas em frente às telas sem pausas adequadas
 - ❌ Falta de ergonomia: Posturas inadequadas levando a LER/DORT
 - ❌ Isolamento: Perda da rotina estruturada do ambiente corporativo
 - ❌ Baixa produtividade: Ambiente inadequado reduz em até 30% a eficiência no trabalho

## 🚀 Funcionalidades
- ✅ **Monitoramento Ambiental**: Temperatura, umidade e luminosidade
- ✅ **Sistema de Scoring**: Avaliação da saúde ambiental (0-100 pontos)
- ✅ **Pausas Inteligentes**: Alertas para café, almoço e alongamentos
- ✅ **Alertas Visuais**: LEDs indicadores de condições inadequadas
- ✅ **Dashboard em Tempo Real**: Integração com ThingSpeak
- ✅ **Tomada de Decisão**: Recomendações automatizadas baseadas em dados

## 🛠️ Tecnologias Utilizadas
- **Microcontrolador**: ESP32
- **Sensores**: DHT22 (Temperatura/Umidade), LDR (Luminosidade)
- **Atuadores**: LEDs, Buzzer
- **Comunicação**: HTTP/REST API
- **Cloud**: ThingSpeak (Dashboard e gráficos)
- **Plataforma**: Wokwi Simulator

<img src="./img/wokwi.jpg" height="350" alt="sistema no wokwi">

## 📦 Hardware
| Componente | Função |
|------------|--------|
| ESP32 | Processamento principal |
| DHT22 | Sensor de temperatura e umidade |
| LDR | Sensor de luminosidade |
| LEDs | Alertas visuais (vermelho, azul, amarelo, verde) |
| Buzzer | Alertas sonoros para pausas |

## 📊 Dashboard ThingSpeak
O sistema envia dados para o ThingSpeak com 4 campos:
- 🌡️ Temperatura (°C)
- 💧 Umidade (%)
- 💡 Luminosidade (0-4095)
- 🏆 Score de Saúde Ambiental (0-100)

<img src="./img/thingspeak.jpg" height="350" alt="gráficos thingspeak">

## 🎯 Como Funciona
1. **Coleta de Dados**: Sensores monitoram ambiente a cada 2.5s
2. **Processamento**: Calcula score baseado em condições ideais
3. **Tomada de Decisão**: Emite alertas e recomendações
4. **Dashboard**: Envia médias a cada 15s para ThingSpeak
5. **Pausas Programadas**: Alertas sonoros e visuais conforme horário virtual

## 📈 Exemplo de Saída

<img src="./img/fazendo_conexao.jpg" height="450" alt="gráficos thingspeak">

<img src="./img/enviando_dados.jpg" height="500" alt="gráficos thingspeak">
