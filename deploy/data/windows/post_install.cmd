sc stop AmneziaWGTunnel$AmneziaVPN
sc delete AmneziaWGTunnel$AmneziaVPN
taskkill /IM "NimbusVPN-service.exe" /F
taskkill /IM "NimbusVPN.exe" /F
exit /b 0
