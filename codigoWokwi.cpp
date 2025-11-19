#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define DHT_PIN 4
#define DHT_TYPE DHT22

// ======================= DEFINIÇÃO DOS PINOS ==============================
#define LED_VERMELHO 12             // Temperatura alta
#define LED_AZUL 14                 // Umidade alta
#define LED_ALERTA_ESCURO_DIA 16    // 🟡 Amarelo - Ambiente escuro durante expediente
#define LED_ALERTA_CLARO_NOITE 17   // 🟢 Verde - Luz intensa durante a noite
#define BUZZER_PIN 27               // Buzzer para pausas
#define LDR_AMBIENTE 33             // LDR para medir luminosidade

// ==================== CONFIGURAÇÃO THINGSPEAK ====================
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define THINGSPEAK_API_KEY "8SMHZFKBKRSXQRAF"
#define THINGSPEAK_URL "http://api.thingspeak.com/update"

// Campos do ThingSpeak
#define FIELD_TEMPERATURA 1
#define FIELD_UMIDADE 2
#define FIELD_LUMINOSIDADE 3
#define FIELD_SCORE_SAUDE 4

bool wifiConectado = false;
unsigned long ultimoEnvioThingSpeak = 0;
#define INTERVALO_ENVIO_THINGSPEAK 15000  // 15 segundos

// ==================== SISTEMA DE ACUMULAÇÃO PARA MÉDIAS ====================
float tempAcumulada = 0;
float umidadeAcumulada = 0;
int ldrAcumulado = 0;
int scoreAcumulado = 0;
int leituras = 0;

// ==================== SISTEMA DE HORÁRIO VIRTUAL ====================
#define SEGUNDOS_POR_HORA_VIRTUAL 5  // 5s real = 1h virtual
#define HORA_INICIAL 7               // Começa às 7:00
unsigned long inicioSimulacao = 0;

// ==================== CONTROLE DE PAUSAS ====================
bool pausaCafeTomada = false;
bool pausaAlmocoTomada = false;
bool pausaTardeTomada = false;
bool pausaAlongamentoTomada = false;

// ==================== CONTADORES PARA THINGSPEAK ====================
int totalPausasRealizadas = 0;
int statusPausaAtual = 0; // 0=Sem pausa, 1=Café, 2=Almoço, 3=Tarde, 4=Alongamento
int alertasAtivos = 0;

// Definições para status de pausa
#define SEM_PAUSA 0
#define PAUSA_CAFE 1
#define PAUSA_ALMOCO 2
#define PAUSA_TARDE 3
#define PAUSA_ALONGAMENTO 4

// ==================== CONTROLE DE ALERTAS VISUAIS ====================
unsigned long ultimoAlertaVisual = 0;
#define INTERVALO_ALERTA_VISUAL 30000  // 30 segundos entre alertas

DHT dht(DHT_PIN, DHT_TYPE);

// ==================== FUNÇÕES THINGSPEAK ====================
void conectarWiFi() {
  Serial.println("📡 INICIANDO CONEXÃO WiFi PERSSISTENTE");
  Serial.println("======================================");
  
  int tentativas = 0;
  unsigned long inicioGeral = millis();
  
  while (true) {
    tentativas++;
    Serial.println("\n🔄 TENTATIVA " + String(tentativas));
    
    // Reset da conexão
    WiFi.disconnect();
    delay(1000);
    WiFi.mode(WIFI_STA);
    
    Serial.print("   Conectando");
    unsigned long startTime = millis();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    bool conectado = false;
    while (millis() - startTime < 15000) { // 15s por tentativa
      if (WiFi.status() == WL_CONNECTED) {
        conectado = true;
        break;
      }
      delay(500);
      Serial.print(".");
    }
    
    if (conectado) {
      unsigned long tempoTotal = (millis() - inicioGeral) / 1000;
      wifiConectado = true;
      Serial.println("\n🎉 ✅ CONEXÃO ESTABELECIDA!");
      Serial.println("   📶 IP: " + WiFi.localIP().toString());
      Serial.println("   📡 RSSI: " + String(WiFi.RSSI()) + " dBm");
      Serial.println("   ⏱️  Tempo total: " + String(tempoTotal) + " segundos");
      Serial.println("   🔄 Tentativas: " + String(tentativas));
      Serial.println("======================================\n");
      break;
    } else {
      Serial.println("\n❌ Falha na tentativa " + String(tentativas));
      
      // Progresso a cada 5 tentativas
      if (tentativas % 5 == 0) {
        Serial.println("💡 Dica: Verifique se o Wokwi está respondendo");
      }
      
      Serial.println("🔄 Nova tentativa em 5 segundos...");
      delay(5000); // Espera 5 segundos entre tentativas
    }
  }
}

int contarAlertasAtivos(float temperatura, float umidade, int horaVirtual, bool escuro) {
  int count = 0;
  
  // Alertas de temperatura
  if (temperatura > 28.0 || temperatura < 18.0) count++;
  
  // Alertas de umidade
  if (umidade > 70.0 || umidade < 30.0) count++;
  
  // Alertas de iluminação
  if (escuro && horaVirtual >= 8 && horaVirtual <= 17) count++;
  if (!escuro && (horaVirtual >= 20 || horaVirtual < 6)) count++;
  
  return count;
}

void enviarParaThingSpeak(float temperatura, float umidade, int luminosidade, int score) {
  if (!wifiConectado || WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi não disponível para envio");
    return;
  }
  
  HTTPClient http;
  
  // 👇 VALIDAÇÃO DOS VALORES
  if (luminosidade == 0) {
    luminosidade = 1; // Substituir 0 por 1 para evitar erro
  }
  
  // Verificar se há valores inválidos
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("🚨 VALORES INVÁLIDOS DETECTADOS!");
    return;
  }
  
  String url = "http://api.thingspeak.com/update";
  url += "?api_key=" + String(THINGSPEAK_API_KEY);
  url += "&field" + String(FIELD_TEMPERATURA) + "=" + String(temperatura, 1);
  url += "&field" + String(FIELD_UMIDADE) + "=" + String(umidade, 1);
  url += "&field" + String(FIELD_LUMINOSIDADE) + "=" + String(luminosidade);
  url += "&field" + String(FIELD_SCORE_SAUDE) + "=" + String(score);
  
  Serial.println("🌐 Enviando dados para ThingSpeak...");
  Serial.println("   📊 Dados enviados (MÉDIAS):");
  Serial.println("   🌡️  Temperatura: " + String(temperatura, 1) + "°C");
  Serial.println("   💧 Umidade: " + String(umidade, 1) + "%");
  Serial.println("   💡 Luminosidade: " + String(luminosidade));
  Serial.println("   🏆 Score: " + String(score));
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String resposta = http.getString();
      Serial.println("✅ Dados enviados!);
    } else {
      Serial.println("❌ Erro HTTP: " + String(httpCode));
    }
  } else {
    Serial.println("❌ Falha na conexão");
  }
  
  http.end();
}

// ==================== SISTEMA DE SCORING INTELIGENTE ====================
int calcularScoreSaudeAmbiental(float temp, float umidade, int hora, bool escuro) {
  int score = 100; // Score perfeito
  
  // Penalidades por condições ruins
  if (temp > 28.0 || temp < 18.0) score -= 30;
  if (umidade > 70.0 || umidade < 30.0) score -= 25;
  
  // Condições de iluminação inadequadas
  if (escuro && hora >= 8 && hora <= 17) {
    score -= 20; // Ambiente escuro durante expediente
  }
  if (!escuro && (hora >= 20 || hora < 6)) {
    score -= 15; // Luz intensa durante a noite
  }

  return max(score, 0); // Não menor que 0
}

void tomarDecisaoAmbiental(int score, float temp, float umidade, int hora, bool escuro) {
  Serial.println("📊 SCORE AMBIENTAL: " + String(score) + "/100");
  
  if (score >= 85) {
    Serial.println("✅ Ambiente IDEAL para produtividade!");
  } 
  else if (score >= 60) {
    Serial.println("⚠️ Ambiente REGULAR - pequenos ajustes necessários");
    if (temp > 28.0) Serial.println("   🎯 Ação: Ventilar ambiente ou ajustar Ar Condicionado");
    if (umidade > 70.0) Serial.println("   🌬️ Ação: Ventilar para reduzir umidade");
    if (umidade < 30.0) Serial.println("   💧 Ação: Usar umidificador");

    if (escuro && hora >= 8 && hora <= 17) Serial.println("   💡 Ação: Deixe o lugar mais iluminado, de preferência para luz natural do dia");
    if (!escuro && (hora >= 20 || hora < 6)) Serial.println("   🌙 Ação: Reduzir iluminação para descanso");
  }
  else {
    Serial.println("🚨 Ambiente CRÍTICO - ajustes urgentes necessários!");
    if (temp > 28.0) Serial.println("   🔥 Ação Imediata: Resfriar ambiente");
    if (temp < 18.0) Serial.println("   ❄️ Ação Imediata: Aquecer ambiente");
    if (umidade > 70.0) Serial.println("   💦 Ação Imediata: Reduzir umidade");
    if (umidade < 30.0) Serial.println("   🏜️ Ação Imediata: Aumentar umidade");
    if (escuro && hora >= 8 && hora <= 17) Serial.println("   💡 Ação Imediata: ILUMINAÇÃO INADEQUADA - Acender luzes!");
    if (!escuro && (hora >= 20 || hora < 6)) Serial.println("   🌙 Ação Imediata: LUZ EXCESSIVA - Reduzir iluminação!");
  }
}

// ==================== FUNÇÕES DE TEMPO VIRTUAL ====================
int getHoraVirtual() {
  unsigned long segundosDesdeInicio = (millis() - inicioSimulacao) / 1000;
  return ((segundosDesdeInicio / SEGUNDOS_POR_HORA_VIRTUAL) + HORA_INICIAL) % 24;
}

int getMinutoVirtual() {
  unsigned long segundosDesdeInicio = (millis() - inicioSimulacao) / 1000;
  return (segundosDesdeInicio % 60);
}

String getHorarioFormatado() {
  int hora = getHoraVirtual();
  int minuto = getMinutoVirtual();
  return String(hora) + ":" + (minuto < 10 ? "0" : "") + String(minuto);
}

void tocarAlertaSuave() {
  tone(BUZZER_PIN, 1000, 200);
}

// ==================== FUNÇÕES LDR ============================

bool ambienteEstaEscuro() {
  int valorLDR = lerLDR(); // Usar a função com filtro
  
  // Threshold mais realista - ajuste conforme necessário
  return (valorLDR < 100); // Considera escuro se valor for menor que 100
}

int lerLDR() {
  // Fazer múltiplas leituras para evitar ruído do WiFi
  int leitura1 = analogRead(LDR_AMBIENTE);
  delay(5);
  int leitura2 = analogRead(LDR_AMBIENTE);
  delay(5);
  int leitura3 = analogRead(LDR_AMBIENTE);
  
  // Calcular média
  int media = (leitura1 + leitura2 + leitura3) / 3;
  
  // Se for 0 (provavelmente erro), retornar um valor mínimo
  if (media == 0) {
    return 1; // Retorna 1 em vez de 0
  }
  
  return media;
}

void verificarIluminacaoBinaria(int horaVirtual) {
  int valorLDR = lerLDR(); // Usar a função com filtro
  
  // Definir thresholds mais realistas
  bool estaEscuro = (valorLDR < 100);   // Ajuste conforme seu LDR
  bool estaClaro = (valorLDR > 2000);   // Ajuste conforme seu LDR
  
  if (estaClaro) {
    
    if (horaVirtual >= 20 || horaVirtual < 6) {
      digitalWrite(LED_ALERTA_CLARO_NOITE, HIGH);
    } else {
      digitalWrite(LED_ALERTA_CLARO_NOITE, LOW);
    }
  } 
  
  if (estaEscuro) {
    Serial.println("   → 🌙 ESCURO (valor baixo = pouca luz)");
    
    if (horaVirtual >= 8 && horaVirtual <= 17) {
      digitalWrite(LED_ALERTA_ESCURO_DIA, HIGH);
    } else {
      digitalWrite(LED_ALERTA_ESCURO_DIA, LOW);
    }
  }
  
  // Garantir que LEDs são apagados quando não há alerta
  if (!estaEscuro) {
    digitalWrite(LED_ALERTA_ESCURO_DIA, LOW);
  }
  if (!estaClaro) {
    digitalWrite(LED_ALERTA_CLARO_NOITE, LOW);
  }
}

// ==================== FUNÇÕES DE SENSORES ====================
void lerSensoresAmbiente() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("❌ Erro na leitura do DHT22!");
    return;
  }
  
  Serial.println("🌡️ Temperatura: " + String(temperature) + "°C");
  Serial.println("💧 Umidade: " + String(humidity) + "%");
  
  controlarLEDsAmbiente(temperature, humidity);
}

void controlarLEDsAmbiente(float temperature, float humidity) {
  // Controlar LED VERMELHO (Temperatura alta > 28°C OU baixa < 18°C)
  if (temperature > 28.0 || temperature < 18.0) {
    digitalWrite(LED_VERMELHO, HIGH);
    if (temperature > 28.0) {
      Serial.println("🔴 Temperatura ALTA - Verificar ambiente");
    } else {
      Serial.println("🔴 Temperatura BAIXA - Verificar ambiente");
    }
  } else {
    digitalWrite(LED_VERMELHO, LOW);
  }
  
  // Controlar LED AZUL (Umidade alta > 70% OU baixa < 30%)
  if (humidity > 70.0 || humidity < 30.0) {
    digitalWrite(LED_AZUL, HIGH);
    if (humidity > 70.0) {
      Serial.println("🔵 Umidade ALTA - Verificar ambiente");
    } else {
      Serial.println("🔵 Umidade BAIXA - Verificar ambiente");
    }
  } else {
    digitalWrite(LED_AZUL, LOW);
  }
}

// ==================== FUNÇÕES DE PAUSAS PROGRAMADAS ====================
void verificarPausaCafe(int horaVirtual) {
  if (horaVirtual == 9 && !pausaCafeTomada) {
    tocarAlertaSuave();
    Serial.println("☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕");
    Serial.println("🕘 PAUSA DO CAFÉ - 9:00");
    Serial.println("💡 Tome um café e alongue os pulsos");
    Serial.println("👋 Cumprimente os colegas");
    Serial.println("🌅 Aproveite para se hidratar");
    Serial.println("☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕☕");
    
    // ⭐ NOVO: Atualizar dados para ThingSpeak
    totalPausasRealizadas++;
    statusPausaAtual = PAUSA_CAFE;
    
    pausaCafeTomada = true;
  } else if (horaVirtual != 9 && pausaCafeTomada) {
    statusPausaAtual = SEM_PAUSA;
  }
}

void verificarPausaAlmoco(int horaVirtual) {
  if (horaVirtual == 12 && !pausaAlmocoTomada) {
    tocarAlertaSuave();
    Serial.println("🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️");
    Serial.println("🕛 HORA DO ALMOÇO - 12:00");
    Serial.println("💡 Afaste-se completamente da tela");
    Serial.println("🍎 Alimente-se de forma saudável");
    Serial.println("🚶‍♂️ Dê uma volta após comer");
    Serial.println("😴 Descanse a mente do trabalho");
    Serial.println("🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️ 🍽️");
    
    totalPausasRealizadas++;
    statusPausaAtual = PAUSA_ALMOCO;
    
    pausaAlmocoTomada = true;
  } else if (horaVirtual != 12 && pausaAlmocoTomada) {
    statusPausaAtual = SEM_PAUSA;
  }
}

void verificarPausaTarde(int horaVirtual) {
  if (horaVirtual == 15 && !pausaTardeTomada) {
    tocarAlertaSuave();
    Serial.println("🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞");
    Serial.println("🕒 PAUSA DA TARDE - 15:00");
    Serial.println("💡 Revitalize-se para o final do dia");
    Serial.println("👀 Descanse os olhos");
    Serial.println("💧 Beba água para manter a hidratação");
    Serial.println("🎵 Ouça uma música para relaxar");
    Serial.println("🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞🌞");
    
    totalPausasRealizadas++;
    statusPausaAtual = PAUSA_TARDE;
    
    pausaTardeTomada = true;
  } else if (horaVirtual != 15 && pausaTardeTomada) {
    statusPausaAtual = SEM_PAUSA;
  }
}

void verificarMicroPausaAlongamento(int horaVirtual, int minutoVirtual) {
  if ((horaVirtual == 10 && minutoVirtual >= 30) || 
      (horaVirtual == 14 && minutoVirtual >= 30)) {
    if (!pausaAlongamentoTomada) {
      tone(BUZZER_PIN, 600, 200);
      Serial.println("🧘‍♂️ MICRO-PAUSA: Alongue costas e pescoço");
      Serial.println("💫 2 minutos para prevenir LER/DORT");
      
      totalPausasRealizadas++;
      statusPausaAtual = PAUSA_ALONGAMENTO;
      
      pausaAlongamentoTomada = true;
    }
  } else {
    pausaAlongamentoTomada = false;
    statusPausaAtual = SEM_PAUSA;
  }
}

// ==================== FUNÇÕES DE MENSAGENS CONTEXTUAIS ====================
void verificarMensagensContextuais(int horaVirtual, int minutoVirtual) {
  // Início do expediente - 7:00
  if (horaVirtual == 7) {
    Serial.println("🌅 BOM DIA! - Tenha um dia produtivo!");
  }
  
  // Fim do expediente - 17:00
  if (horaVirtual == 17) {
    Serial.println("🏠 FIM DO EXPEDIENTE - Descanse e recarregue as energias!");
  }
  
  // Virada do dia - 7:00 do próximo dia
  verificarViradaDia(horaVirtual, minutoVirtual);
}

void verificarViradaDia(int horaVirtual, int minutoVirtual) {
  if (horaVirtual == 7 && minutoVirtual < 5) {
    static bool novoDiaProcessado = false;
    if (!novoDiaProcessado) {
      reiniciarSistemaPausas();
      novoDiaProcessado = true;
    }
  } else {
    static bool novoDiaProcessado = false;
    novoDiaProcessado = false;
  }
}

void reiniciarSistemaPausas() {
  pausaCafeTomada = false;
  pausaAlmocoTomada = false;
  pausaTardeTomada = false;
  pausaAlongamentoTomada = false;
  totalPausasRealizadas = 0;
  statusPausaAtual = SEM_PAUSA;
}

// ==================== FUNÇÃO PRINCIPAL DO SISTEMA ====================
void executarSistemaWellWork() {
  // Obter horário atual
  String horarioAtual = getHorarioFormatado();
  int horaVirtual = getHoraVirtual();
  int minutoVirtual = getMinutoVirtual();

  int valorLDR = lerLDR();
  bool escuro = ambienteEstaEscuro();

  Serial.println("🕐 HORÁRIO VIRTUAL: " + horarioAtual + "h");
  
  // Ler sensores de ambiente
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  
  if (!isnan(temperatura) && !isnan(umidade)) {
    Serial.println("🌡️ Temperatura: " + String(temperatura) + "°C");
    Serial.println("💧 Umidade: " + String(umidade) + "%");
    Serial.println("💡 LDR: " + String(valorLDR));
    
    // ========== SISTEMA DE SCORING INTELIGENTE ==========
    int score = calcularScoreSaudeAmbiental(temperatura, umidade, horaVirtual, escuro);
    tomarDecisaoAmbiental(score, temperatura, umidade, horaVirtual, escuro);
    
    controlarLEDsAmbiente(temperatura, umidade);
    
    // ⭐⭐ ACUMULAR VALORES PARA MÉDIA ==========
    tempAcumulada += temperatura;
    umidadeAcumulada += umidade;
    ldrAcumulado += valorLDR;
    scoreAcumulado += score;
    leituras++;
    
    Serial.println("📈 Acumulando dados para média (" + String(leituras) + " leituras)");
    
    // ⭐⭐ NOVO: ENVIAR MÉDIAS PARA THINGSPEAK ==========
    if (millis() - ultimoEnvioThingSpeak > INTERVALO_ENVIO_THINGSPEAK && leituras > 0) {
      // Calcular médias
      float tempMedia = tempAcumulada / leituras;
      float umidadeMedia = umidadeAcumulada / leituras;
      int ldrMedia = ldrAcumulado / leituras;
      int scoreMedia = scoreAcumulado / leituras;
      
      Serial.println("📊 Calculando médias de " + String(leituras) + " leituras:");
      Serial.println("   🌡️  Temp média: " + String(tempMedia, 1) + "°C");
      Serial.println("   💧 Umidade média: " + String(umidadeMedia, 1) + "%");
      Serial.println("   💡 LDR médio: " + String(ldrMedia));
      Serial.println("   🏆 Score médio: " + String(scoreMedia));
      
      enviarParaThingSpeak(tempMedia, umidadeMedia, ldrMedia, scoreMedia);
      
      // Resetar acumuladores
      tempAcumulada = 0;
      umidadeAcumulada = 0;
      ldrAcumulado = 0;
      scoreAcumulado = 0;
      leituras = 0;
      ultimoEnvioThingSpeak = millis();
    }
  }

  // Verificar iluminação binária
  verificarIluminacaoBinaria(horaVirtual);

  // Verificar todas as pausas programadas
  verificarPausaCafe(horaVirtual);
  verificarPausaAlmoco(horaVirtual);
  verificarPausaTarde(horaVirtual);
  verificarMicroPausaAlongamento(horaVirtual, minutoVirtual);
  
  // Verificar mensagens contextuais
  verificarMensagensContextuais(horaVirtual, minutoVirtual);
  
  // Separador
  Serial.println("--------------------------------------------");
}

// ==================== SETUP E LOOP ====================
void setup() {
  Serial.begin(115200);
  dht.begin(); 
  
  // Configurar os pinos
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_ALERTA_ESCURO_DIA, OUTPUT);
  pinMode(LED_ALERTA_CLARO_NOITE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Iniciar com LEDs apagados e buzzer desligado
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_ALERTA_ESCURO_DIA, LOW);
  digitalWrite(LED_ALERTA_CLARO_NOITE, LOW);
  noTone(BUZZER_PIN);
  
  // Conectar WiFi
  conectarWiFi();
  
  // Iniciar simulação de tempo
  inicioSimulacao = millis();
  
  Serial.println("🚀 Sistema WellWork - Pausas Inteligentes + Monitoramento Completo");
  Serial.println("📡 COM THINGSPEAK INTEGRATION (MÉDIAS)");
  Serial.println("⏰ 5 segundos reais = 1 hora virtual");
  Serial.println("🌅 Horário inicia às 7:00");
  Serial.println("📊 Dados enviados como MÉDIAS a cada 15 segundos");
  Serial.println("--------------------------------------------");
}

void loop() {
  executarSistemaWellWork();
  delay(2500);
}