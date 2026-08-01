#  Projekt Änderungen: Dilbert <span style="float: right">[⬅️](project_changes.md) [⬆️](../slides.md) [➡️](experiences_ki_pos.md)</span>


<div style="float: right; width: 255px; background-color: #888; border-radius: 50%;">
  <img src="../../../web/favicon.ico" style="width: 100%; display: block;">
</div>


#### Dann kam `<dilbert>` ins Spiel
  1. Inbetriebnahme der Servos  
  Kalibration: PWM → °, %
  2. Testen der Joint Positions  
  d[°], s[°], e[°], hp[°], hr[°], g[%]
  3. Caching von Joint Positions (Client Side Storage)
  4. Load & Save von Joint Positions
  5. Testen der IK Algorithmik  
  Test: Pose(x,y,z,hp,hr,g) → JointPosition(d,s,e,hp,hr,g)
  6. Welt-Koordinatensystem  
  x[mm], y[mm], z[mm], hp[°], hr[°], g[%]
  7. Load & Save von Posen
  8. Sequenzen (Run Engine)
     - Posen
     - LED Farben
     - Warten
  9. Client Side Storage
  10. Mehr in der Demo 🎁

#### Abgrenzung
  * CORS Preflight erlauben (<u>C</u>ross <u>O</u>rigin <u>R</u>essource <u>S</u>haring)  
  * Funktioniert! 👍
  * ETH über USB ausgelassen.
  * Mögliche Erweiterung: Web Serial API, Browser → UART 💡
