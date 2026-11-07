# Publicació a GitHub Pages

El repositori inclou una web estàtica a `docs/` i un workflow a `.github/workflows/pages.yml`. No cal contractar allotjament ni configurar cap servidor per llegir la guia.

## Primera vegada

1. Puja aquests fitxers al repositori `jrevelldev/esp32-ultrasonic-touchdesigner`.
2. Al repositori, obre **Settings → Pages**.
3. A **Build and deployment → Source**, selecciona **GitHub Actions**.
4. A la pestanya **Actions**, obre **Publica la guia a GitHub Pages**. Si no s’ha iniciat amb la pujada, prem **Run workflow** sobre la branca principal.
5. Espera que `build` i `deploy` acabin en verd. A **Settings → Pages** apareixerà l’adreça publicada.

L’adreça prevista és **https://jrevelldev.github.io/esp32-ultrasonic-touchdesigner/**. Només queda activa després d’una publicació correcta. Si el repositori és privat, la disponibilitat de Pages depèn del pla de GitHub; no canviïs la seva visibilitat sense decidir-ho expressament.

## Actualitzacions

Una pujada a `main` o `master` torna a comprovar i publicar la guia. Les descàrregues es regeneren des de `firmware/`, `touchdesigner/` i `hardware/`; el paquet Arduino conserva la carpeta necessària perquè el .ino s’obri correctament.

No hi ha contrasenyes del Wi-Fi de casa al programa. Aquestes es desen a la memòria de cada ESP quan el configures. La clau inicial del Wi-Fi de configuració (`Configura32`) és un valor conegut del projecte i es pot canviar a `AP_PASSWORD`.

## Alternatives i errors

- Si Pages indica que no està activat, comprova primer **Source = GitHub Actions** i torna a executar el workflow.
- Si es bloqueja el desplegament per una regla de l’entorn `github-pages`, comprova a **Settings → Environments** que la branca principal estigui autoritzada.
- La web utilitza rutes relatives i funciona sota el subdirectori del repositori.
- També pots publicar directament des de `main` i `/docs` a **Settings → Pages → Deploy from a branch**. En aquest cas, desactiva el workflow `Publica la guia a GitHub Pages` a **Actions** perquè no intenti publicar per un segon mecanisme, i regenera i puja manualment les descàrregues quan les canviïs.

Referències: [configurar l’origen de Pages](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site) · [workflows de Pages](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages).
