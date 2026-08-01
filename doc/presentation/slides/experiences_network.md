# Erfahrungen Netzwerk <span style="float: right">[⬅️](experiences_ik.md) [⬆️](../slides.md) [➡️](demo.md)</span>

Folgender Fehlerfall: <i style="color: orange">Kein Zugriff auf ’http&#58;//robot.local‘ aus der WSL</i> 😡
* Kaputte Demo reparieren!
* Ursache unbekannt!? Wahrscheinlich ein Windows- oder WSL-Update... 🤢
* `${ENDLOSE_KI_DISKUSSION}`  
  24h später...
* Lösung Netzwerkmodus von ’NAT‘ auf ’Mirrored‘
* Kollateralschaden VSCode startet nicht mehr
* `${ENDLOSE_KI_DISKUSSION}`  
  Noch einmal Stunden später Eintrag in `/etc/hosts`
  ```
  robot_ip=$(
  powershell.exe -NoProfile -Command \
    "[System.Net.Dns]::GetHostAddresses('robot.local') |
     Where-Object AddressFamily -eq InterNetwork |
     Select-Object -First 1 -ExpandProperty IPAddressToString" |
  tr -d '\r'
  )
  
  sudo sed -i '/# WSL robot.local$/d' /etc/hosts
  printf '%s robot.local # WSL robot.local\n' "$robot_ip" |
  sudo tee -a /etc/hosts
  ```

