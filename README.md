# 🌾🐄 AgroTrack — Coleira Inteligente para Monitoramento de Gado

<p align="center">
  <b>Sistema IoT para monitoramento de gado utilizando ESP32, Wi-Fi, MQTT e HiveMQ Cloud.</b>
</p>

<p align="center">
  🌐 ESP32 • 📡 Wi-Fi • ☁️ HiveMQ • 🗺️ Localização • 🔋 Alimentação • 🌾 Dashboard Agro
</p>

---

## 📖 Sobre o projeto

O **AgroTrack** é um sistema de monitoramento desenvolvido para utilização em uma coleira inteligente instalada em bovinos.

A solução utiliza uma **ESP32** para realizar a comunicação com a Internet através de Wi-Fi e enviar dados em tempo real para um servidor MQTT hospedado no **HiveMQ Cloud**.

Os dados podem ser acompanhados através de um dashboard web desenvolvido inteiramente em:

- HTML
- CSS
- JavaScript

Tudo está concentrado em uma única página `index.html`.

O sistema permite acompanhar informações como:

- 🐄 Identificação do animal
- 📍 Latitude
- 📍 Longitude
- 🎯 Precisão da localização
- 📶 Intensidade do sinal Wi-Fi
- 🌐 Endereço IP da ESP32
- ⏱️ Tempo de funcionamento
- 🔋 Estado da alimentação
- 📡 Status MQTT
- 🟢 Coleira online/offline
- 🗺️ Localização em mapa
- 📋 Log das mensagens MQTT em tempo real

---

# 🏗️ Arquitetura do sistema

```text
                 🐄
             GADO / COLEIRA
                  │
                  ▼
             ┌─────────┐
             │  ESP32  │
             └────┬────┘
                  │
                  │ Wi-Fi
                  ▼
           📶 Rede sem fio
                  │
                  ▼
              INTERNET
                  │
                  ▼
        ☁️ HIVEMQ CLOUD MQTT
                  │
        ┌─────────┼──────────┐
        │         │          │
        ▼         ▼          ▼
   Localização  Bateria   Telemetria
        │         │          │
        └─────────┼──────────┘
                  │
                  ▼
          🌾 AGROTRACK WEB
                  │
          HTML + CSS + JS
                  │
                  ▼
            🗺️ DASHBOARD
