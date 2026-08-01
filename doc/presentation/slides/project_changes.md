 #  Projekt Änderungen<span style="float: right">[⬅️](project_implementation.md) [⬆️](../slides.md) [➡️](project_changes_dilbert.md)</span>
#### Ungeplantes
* Clang-format
* mDNS
* REST
* SBOM
* SPI Flash File System (SPIFFS) und Webserver (wozu nur!?)

#### REST Calls mit `curl`?

  ```
  > ping robot.local
  
  $ sudo nano /etc/hosts
  
  export BASE_URL="${BASE_URL:-http://robot.local}"

  curl --max-time 5 -i \
    -X POST "$BASE_URL/api/joint-motion" \
    -H 'Content-Type: application/json' \
    -d '{"d_deg":0, "s_deg":45, "e_deg":-45, "hp_deg":-45, "hr_deg":45, "g_pct":50}'
  ```

#### REST Calls mit Shell Skripten
* Demo:  
  * [init.sh](../../../scripts/init.sh): `./scripts/init.sh`
  * [rest_api_wsl_examples.sh](../../../scripts/rest_api_wsl_examples.sh): `./scripts/rest_api_wsl_examples.sh`

<p style="color: orange"><b><i>«Das isch alles chli müesam! Was jetzt!?»</i></b></p>